#include "objecttracker.h"
#include "floormapper.h"

#include <QObject>

#include <opencv2/imgproc.hpp>
#include <opencv2/tracking.hpp>

#include <algorithm>
#include <cmath>

namespace {

// Le suivi travaille sur une image d'au plus 640 px de large : CSRT prend environ 23 ms par
// image à cette taille, bien plus en haute résolution.
constexpr int kWorkingWidth = 640;
constexpr double kRadToDeg = 180.0 / M_PI;

// Indice de confiance : objet déclaré perdu sous ce seuil pendant quelques images d'affilée.
constexpr double kLostConfidence = 0.45;
constexpr int kLostConfidenceFrames = 3;
// Au-dessus de ce seuil, l'apparence de référence suit lentement l'objet (éclairage, rotation
// progressive) ; un intrus qui surgit fait au contraire chuter la confiance d'un coup.
constexpr double kUpdateConfidence = 0.7;
constexpr double kReferenceUpdateRate = 0.05;

} // namespace

ObjectTracker::ObjectTracker() = default;
ObjectTracker::~ObjectTracker() = default;

QStringList ObjectTracker::algorithmNames()
{
    return {QObject::tr("CSRT (précis)"), QObject::tr("KCF (rapide)")};
}

cv::Mat ObjectTracker::toWorkingImage(const QImage &image, double &scale) const
{
    const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
    const cv::Mat bgra(rgb.height(), rgb.width(), CV_8UC4, const_cast<uchar *>(rgb.constBits()),
                       size_t(rgb.bytesPerLine()));
    cv::Mat bgr;
    cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
    scale = 1.0;
    if (bgr.cols > kWorkingWidth) {
        scale = double(bgr.cols) / kWorkingWidth;
        cv::resize(bgr, bgr, cv::Size(), 1.0 / scale, 1.0 / scale, cv::INTER_AREA);
    }
    return bgr;
}

cv::Mat ObjectTracker::colorHistogram(const cv::Mat &bgrPatch)
{
    cv::Mat hsv;
    cv::cvtColor(bgrPatch, hsv, cv::COLOR_BGR2HSV);
    const int channels[] = {0, 1};
    const int bins[] = {30, 32};
    const float hueRange[] = {0, 180}, satRange[] = {0, 256};
    const float *ranges[] = {hueRange, satRange};
    cv::Mat hist;
    cv::calcHist(&hsv, 1, channels, cv::Mat(), hist, 2, bins, ranges);
    cv::normalize(hist, hist, 1.0, 0.0, cv::NORM_L1);
    return hist;
}

double ObjectTracker::appearanceScore(const cv::Mat &work, const cv::Rect &roi) const
{
    const cv::Rect r = roi & cv::Rect(0, 0, work.cols, work.rows);
    if (r.width < 4 || r.height < 4 || m_refHist.empty())
        return 0.0;
    const cv::Mat patch = work(r);

    // Couleurs : peu sensibles à la rotation et aux déformations de l'objet.
    const double colorScore = 1.0 - cv::compareHist(m_refHist, colorHistogram(patch), cv::HISTCMP_BHATTACHARYYA);

    // Motif en niveaux de gris : distingue l'objet d'un fond ou d'un intrus aux couleurs proches.
    cv::Mat gray, resized, ncc;
    cv::cvtColor(patch, gray, cv::COLOR_BGR2GRAY);
    cv::resize(gray, resized, m_refGray.size(), 0, 0, cv::INTER_AREA);
    cv::matchTemplate(resized, m_refGray, ncc, cv::TM_CCOEFF_NORMED);
    const double patternScore = std::max(0.0, double(ncc.at<float>(0, 0)));

    return 0.5 * colorScore + 0.5 * patternScore;
}

bool ObjectTracker::start(const QImage &image, const QRectF &box, qint64 timeMs)
{
    stop();
    double scale = 1.0;
    const cv::Mat work = toWorkingImage(image, scale);
    const cv::Rect roi = cv::Rect(int(box.x() / scale), int(box.y() / scale), int(box.width() / scale),
                                  int(box.height() / scale)) &
                         cv::Rect(0, 0, work.cols, work.rows);
    if (roi.width < 8 || roi.height < 8)
        return false;

    if (m_algorithm == Kcf)
        m_tracker = cv::TrackerKCF::create();
    else
        m_tracker = cv::TrackerCSRT::create();
    try {
        m_tracker->init(work, roi);
    } catch (const cv::Exception &) {
        m_tracker.release();
        return false;
    }

    m_refHist = colorHistogram(work(roi));
    cv::cvtColor(work(roi), m_initGray, cv::COLOR_BGR2GRAY);
    m_refGray = m_initGray.clone();
    m_lowConfidenceFrames = 0;

    m_state = TrackedObject();
    m_state.active = true;
    m_state.found = true;
    m_state.confidence = 1.0;
    m_state.box = QRectF(roi.x * scale, roi.y * scale, roi.width * scale, roi.height * scale);
    m_state.center = m_state.box.center();
    m_filter.setStrength(m_smoothing);
    m_filter.reset(m_state.center);
    m_lastUpdateMs = m_lastSeenMs = timeMs;
    m_trail.push_back({timeMs, m_state.center});
    return true;
}

void ObjectTracker::stop()
{
    m_tracker.release();
    m_refHist.release();
    m_refGray.release();
    m_initGray.release();
    m_state = TrackedObject();
    m_trail.clear();
}

const TrackedObject &ObjectTracker::update(const QImage &image, qint64 timeMs, const FloorMapper *floor)
{
    if (!m_tracker)
        return m_state;

    double scale = 1.0;
    const cv::Mat work = toWorkingImage(image, scale);
    cv::Rect roi;
    bool found = false;
    try {
        found = m_tracker->update(work, roi);
    } catch (const cv::Exception &) {
        found = false;
    }

    // Indice de confiance : le tracker peut affirmer avoir trouvé l'objet alors que son cadre a
    // glissé sur le fond ou s'est accroché à un autre objet passé devant. On compare le contenu du
    // cadre à l'apparence mémorisée ; une confiance trop basse plusieurs images d'affilée vaut perte.
    m_state.drifted = false;
    if (found) {
        const double score = appearanceScore(work, roi);
        m_state.confidence = 0.6 * m_state.confidence + 0.4 * score;
        m_lowConfidenceFrames = m_state.confidence < kLostConfidence ? m_lowConfidenceFrames + 1 : 0;
        if (m_lowConfidenceFrames >= kLostConfidenceFrames) {
            found = false;
            m_state.drifted = true;
        } else if (m_state.confidence > kUpdateConfidence) {
            const cv::Mat patch = work(roi & cv::Rect(0, 0, work.cols, work.rows));
            cv::addWeighted(m_refHist, 1.0 - kReferenceUpdateRate, colorHistogram(patch), kReferenceUpdateRate,
                            0.0, m_refHist);
            cv::Mat gray, resized;
            cv::cvtColor(patch, gray, cv::COLOR_BGR2GRAY);
            cv::resize(gray, resized, m_refGray.size(), 0, 0, cv::INTER_AREA);
            cv::addWeighted(m_refGray, 1.0 - kReferenceUpdateRate, resized, kReferenceUpdateRate, 0.0, m_refGray);
        }
    } else {
        m_state.confidence = 0.6 * m_state.confidence;
    }

    // Filtre de Kalman : prédiction au temps de l'image, puis correction par la mesure.
    m_filter.setStrength(m_smoothing);
    m_filter.predict((timeMs - m_lastUpdateMs) / 1000.0);
    m_lastUpdateMs = timeMs;

    m_state.found = found;
    if (found) {
        m_state.box = QRectF(roi.x * scale, roi.y * scale, roi.width * scale, roi.height * scale);
        m_filter.update(m_state.box.center());
        m_lastSeenMs = timeMs;
    }
    // Objet perdu : position prédite pendant le délai de maintien, puis figée à la dernière
    // position connue. Le tracker continue de chercher aux images suivantes.
    m_state.predicted = !found && timeMs - m_lastSeenMs <= m_holdMs;
    if (m_state.predicted) {
        m_state.center = m_filter.position();
        m_state.box.moveCenter(m_state.center);
    } else if (found) {
        m_state.center = m_smoothPosition ? m_filter.position() : m_state.box.center();
    }
    if (found)
        m_trail.push_back({timeMs, m_state.center});
    while (!m_trail.empty() && timeMs - m_trail.front().timeMs > m_trailMs)
        m_trail.pop_front();

    computeMotion(floor);
    return m_state;
}

void ObjectTracker::computeMotion(const FloorMapper *floor)
{
    m_state.velocity = QPointF();
    m_state.speed = 0.0;
    m_state.moving = false;
    m_state.floorVelocity = QPointF();
    m_state.floorSpeed = 0.0;

    const bool floorReady = floor && floor->isReady();
    m_state.hasFloor = floorReady;
    if (floorReady)
        m_state.floorPosition = floor->toFloor(m_state.center);

    if (!m_state.found && !m_state.predicted)
        return;

    m_state.velocity = m_filter.velocity();
    m_state.speed = std::hypot(m_state.velocity.x(), m_state.velocity.y());
    m_state.direction = std::atan2(m_state.velocity.y(), m_state.velocity.x()) * kRadToDeg;

    // En mouvement : plus de 10 % de la taille de l'objet par seconde (au moins 5 px/s), pour
    // ne pas réagir au léger tremblement du cadre.
    const double size = std::hypot(m_state.box.width(), m_state.box.height());
    m_state.moving = m_state.speed > std::max(5.0, 0.1 * size);

    if (floorReady) {
        // La projection sur le sol n'est pas linéaire : vitesse calculée sur 0,1 s.
        constexpr double h = 0.1;
        m_state.floorVelocity =
            (floor->toFloor(m_state.center + m_state.velocity * h) - m_state.floorPosition) / h;
        m_state.floorSpeed = std::hypot(m_state.floorVelocity.x(), m_state.floorVelocity.y());
        m_state.floorDirection =
            std::atan2(m_state.floorVelocity.y(), m_state.floorVelocity.x()) * kRadToDeg;
    }
}
