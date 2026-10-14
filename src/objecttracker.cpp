#include "objecttracker.h"
#include "floormapper.h"
#include "trackermodels.h"

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

// Reprise automatique : recherche dans toute l'image, une image sur trois, sur une image réduite
// de moitié ; un candidat doit ressembler à l'objet par son motif ET par l'indice de confiance.
constexpr int kSearchEveryFrames = 3;
constexpr double kSearchDownscale = 0.5;
constexpr double kSearchMinCorrelation = 0.6;
constexpr double kSearchMinConfidence = 0.55;

} // namespace

ObjectTracker::ObjectTracker() = default;
ObjectTracker::~ObjectTracker() = default;

QStringList ObjectTracker::algorithmNames()
{
    return {QObject::tr("CSRT (précis)"), QObject::tr("KCF (rapide)"), QObject::tr("Nano (réseau de neurones)"),
            QObject::tr("Vit (réseau de neurones)")};
}

bool ObjectTracker::isAvailable(Algorithm algorithm)
{
    if (algorithm == Nano)
        return TrackerModels::missing(TrackerModels::Nano).empty();
    if (algorithm == Vit)
        return TrackerModels::missing(TrackerModels::Vit).empty();
    return true;
}

namespace {

// Image Qt -> BGR d'au plus « maxWidth » px de large ; « scale » = image d'origine / image réduite.
cv::Mat toBgr(const QImage &image, int maxWidth, double &scale)
{
    const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
    const cv::Mat bgra(rgb.height(), rgb.width(), CV_8UC4, const_cast<uchar *>(rgb.constBits()),
                       size_t(rgb.bytesPerLine()));
    cv::Mat bgr;
    cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
    scale = 1.0;
    if (bgr.cols > maxWidth) {
        scale = double(bgr.cols) / maxWidth;
        cv::resize(bgr, bgr, cv::Size(), 1.0 / scale, 1.0 / scale, cv::INTER_AREA);
    }
    return bgr;
}

} // namespace

cv::Mat ObjectTracker::toWorkingImage(const QImage &image, double &scale) const
{
    return toBgr(image, kWorkingWidth, scale);
}

QRect ObjectTracker::delineateAt(const QImage &image, const QPoint &point, bool *uncertain)
{
    // GrabCut sur une fenêtre centrée sur le clic : on suppose l'objet dans la fenêtre et le fond
    // autour. La fenêtre fait 40 % du petit côté de l'image (image réduite à 480 px de large).
    double scale = 1.0;
    const cv::Mat img = toBgr(image, 480, scale);
    const cv::Point p(int(point.x() / scale), int(point.y() / scale));
    const int side = int(0.4 * std::min(img.cols, img.rows));
    const cv::Rect window = cv::Rect(p.x - side / 2, p.y - side / 2, side, side) & cv::Rect(0, 0, img.cols, img.rows);
    const cv::Rect fallback(int((p.x - side / 4) * scale), int((p.y - side / 4) * scale), int(side / 2 * scale),
                            int(side / 2 * scale));
    if (uncertain)
        *uncertain = true;
    if (window.width < 20 || window.height < 20 || !window.contains(p))
        return QRect(fallback.x, fallback.y, fallback.width, fallback.height);

    // Zone de travail : la fenêtre et une marge de fond autour.
    const int margin = side / 4;
    const cv::Rect area = cv::Rect(window.x - margin, window.y - margin, window.width + 2 * margin,
                                   window.height + 2 * margin) &
                          cv::Rect(0, 0, img.cols, img.rows);
    const cv::Mat crop = img(area).clone();
    cv::Mat mask(crop.size(), CV_8UC1, cv::Scalar(cv::GC_BGD)), bgdModel, fgdModel;
    const cv::Rect rect = window - area.tl();
    try {
        cv::grabCut(crop, mask, rect, bgdModel, fgdModel, 3, cv::GC_INIT_WITH_RECT);
    } catch (const cv::Exception &) {
        return QRect(fallback.x, fallback.y, fallback.width, fallback.height);
    }
    cv::Mat fg = (mask == cv::GC_FGD) | (mask == cv::GC_PR_FGD);
    cv::morphologyEx(fg, fg, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE, {5, 5}));

    // Zone de premier plan qui contient le point cliqué (ou la plus proche, à 10 px près).
    cv::Mat labels, stats, centroids;
    const int n = cv::connectedComponentsWithStats(fg, labels, stats, centroids);
    const cv::Point local = p - area.tl();
    int best = labels.at<int>(local);
    if (best == 0) {
        double bestDist = 10.0;
        for (int i = 1; i < n; ++i) {
            const cv::Rect r(stats.at<int>(i, cv::CC_STAT_LEFT), stats.at<int>(i, cv::CC_STAT_TOP),
                             stats.at<int>(i, cv::CC_STAT_WIDTH), stats.at<int>(i, cv::CC_STAT_HEIGHT));
            const double dx = std::max({r.x - local.x, 0, local.x - (r.x + r.width)});
            const double dy = std::max({r.y - local.y, 0, local.y - (r.y + r.height)});
            if (std::hypot(dx, dy) < bestDist) {
                bestDist = std::hypot(dx, dy);
                best = i;
            }
        }
    }
    if (best <= 0)
        return QRect(fallback.x, fallback.y, fallback.width, fallback.height);
    const cv::Rect found(stats.at<int>(best, cv::CC_STAT_LEFT) + area.x, stats.at<int>(best, cv::CC_STAT_TOP) + area.y,
                         stats.at<int>(best, cv::CC_STAT_WIDTH), stats.at<int>(best, cv::CC_STAT_HEIGHT));
    // Trop petit, ou remplissant presque toute la fenêtre (objet plus grand que la fenêtre) : douteux.
    const double fill = double(found.area()) / window.area();
    if (found.width < 10 || found.height < 10 || fill > 0.9)
        return QRect(fallback.x, fallback.y, fallback.width, fallback.height);
    if (uncertain)
        *uncertain = false;
    return QRect(int(found.x * scale), int(found.y * scale), int(found.width * scale), int(found.height * scale));
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

bool ObjectTracker::createTracker(const cv::Mat &work, const cv::Rect &roi)
{
    try {
        switch (m_algorithm) {
        case Kcf:
            m_tracker = cv::TrackerKCF::create();
            break;
        case Nano: {
            cv::TrackerNano::Params params;
            params.backbone = TrackerModels::find(QStringLiteral("nanotrack_backbone_sim.onnx")).toStdString();
            params.neckhead = TrackerModels::find(QStringLiteral("nanotrack_head_sim.onnx")).toStdString();
            m_tracker = cv::TrackerNano::create(params);
            break;
        }
        case Vit: {
            cv::TrackerVit::Params params;
            params.net = TrackerModels::find(QStringLiteral("object_tracking_vittrack_2023sep.onnx")).toStdString();
            m_tracker = cv::TrackerVit::create(params);
            break;
        }
        default:
            m_tracker = cv::TrackerCSRT::create();
            break;
        }
        m_tracker->init(work, roi);
    } catch (const cv::Exception &) {
        m_tracker.release();
        return false;
    }
    return true;
}

cv::Rect ObjectTracker::searchObject(const cv::Mat &work) const
{
    if (m_initGray.empty())
        return {};
    cv::Mat gray, small;
    cv::cvtColor(work, gray, cv::COLOR_BGR2GRAY);
    cv::resize(gray, small, cv::Size(), kSearchDownscale, kSearchDownscale, cv::INTER_AREA);

    // Échelles autour de la dernière taille connue de l'objet.
    const double base = m_lastBoxSize.width() / m_initGray.cols;
    double bestScore = -1.0;
    cv::Rect best;
    for (const cv::Mat *reference : {&m_initGray, &m_refGray}) {
        for (double factor : {0.85, 1.0, 1.2}) {
            const double s = base * factor * kSearchDownscale;
            const cv::Size size(int(reference->cols * s), int(reference->rows * s));
            if (size.width < 8 || size.height < 8 || size.width >= small.cols || size.height >= small.rows)
                continue;
            cv::Mat templ, result;
            cv::resize(*reference, templ, size, 0, 0, cv::INTER_AREA);
            cv::matchTemplate(small, templ, result, cv::TM_CCOEFF_NORMED);
            double maxVal = 0;
            cv::Point maxLoc;
            cv::minMaxLoc(result, nullptr, &maxVal, nullptr, &maxLoc);
            if (maxVal > bestScore) {
                bestScore = maxVal;
                best = cv::Rect(int(maxLoc.x / kSearchDownscale), int(maxLoc.y / kSearchDownscale),
                                int(size.width / kSearchDownscale), int(size.height / kSearchDownscale));
            }
        }
    }
    best &= cv::Rect(0, 0, work.cols, work.rows);
    if (bestScore < kSearchMinCorrelation || best.width < 8 || best.height < 8)
        return {};
    // Vérification complète (couleurs et motif) avant d'accepter le candidat.
    if (appearanceScore(work, best) < kSearchMinConfidence)
        return {};
    return best;
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

    if (!createTracker(work, roi))
        return false;

    m_refHist = colorHistogram(work(roi));
    cv::cvtColor(work(roi), m_initGray, cv::COLOR_BGR2GRAY);
    m_refGray = m_initGray.clone();
    m_lowConfidenceFrames = 0;
    m_lostFrames = 0;
    m_lastBoxSize = QSizeF(roi.width, roi.height);
    m_anchorValid = false;
    m_recentPositions.clear();
    m_lostReported = false;
    m_events.clear();

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

    // Reprise automatique : objet perdu (par le tracker ou par l'indice de confiance), on le
    // cherche dans toute l'image et on relance le tracker là où il est retrouvé.
    m_state.reacquired = false;
    if (found) {
        m_lostFrames = 0;
        m_lastBoxSize = QSizeF(roi.width, roi.height);
    } else if (++m_lostFrames % kSearchEveryFrames == 0) {
        const cv::Rect candidate = searchObject(work);
        if (!candidate.empty() && createTracker(work, candidate)) {
            roi = candidate;
            found = true;
            m_state.reacquired = true;
            m_state.drifted = false;
            ++m_state.reacquisitions;
            m_state.confidence = appearanceScore(work, roi);
            m_lowConfidenceFrames = 0;
            m_lostFrames = 0;
            m_lastBoxSize = QSizeF(roi.width, roi.height);
        }
    }

    // Filtre de Kalman : prédiction au temps de l'image, puis correction par la mesure.
    m_filter.setStrength(m_smoothing);
    m_filter.predict((timeMs - m_lastUpdateMs) / 1000.0);
    m_lastUpdateMs = timeMs;

    m_state.found = found;
    if (found) {
        m_state.box = QRectF(roi.x * scale, roi.y * scale, roi.width * scale, roi.height * scale);
        if (m_state.reacquired)
            m_filter.reset(m_state.box.center());  // l'objet a pu réapparaître loin : on repart de là
        else
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
        m_trail.push_back({timeMs, m_state.center, m_state.reacquired});
    while (!m_trail.empty() && timeMs - m_trail.front().timeMs > m_trailMs)
        m_trail.pop_front();

    computeMotion(floor);
    updateMotionState(timeMs);
    return m_state;
}

std::vector<MotionEvent> ObjectTracker::takeEvents()
{
    std::vector<MotionEvent> events;
    events.swap(m_events);
    return events;
}

void ObjectTracker::addEvent(MotionEvent::Type type, qint64 timeMs)
{
    MotionEvent e;
    e.type = type;
    e.timeMs = timeMs;
    e.onFloor = m_state.hasFloor;
    e.position = m_state.hasFloor ? m_state.floorPosition : m_state.center;
    m_events.push_back(e);
}

void ObjectTracker::updateMotionState(qint64 timeMs)
{
    // Perte : signalée une fois la prédiction terminée ; retour : dès que l'objet est retrouvé.
    if (!m_state.found) {
        if (!m_state.predicted && !m_lostReported) {
            m_state.moving = false;
            addEvent(MotionEvent::Lost, timeMs);
            m_lostReported = true;
        }
        return;
    }
    if (m_lostReported || m_state.reacquired) {
        m_lostReported = false;
        m_anchorValid = false;  // l'objet a pu réapparaître ailleurs : nouvelle position de repos
        addEvent(MotionEvent::Found, timeMs);
    }

    const bool onFloor = m_state.hasFloor;
    const QPointF pos = onFloor ? m_state.floorPosition : m_state.center;
    const double threshold = onFloor ? m_motionMm : m_motionPx;
    // Position de repos à (re)définir : début du suivi, reprise, ou changement d'unité.
    if (!m_anchorValid || m_anchorOnFloor != onFloor) {
        m_anchorValid = true;
        m_anchorOnFloor = onFloor;
        m_anchor = pos;
        m_state.moving = false;
        m_recentPositions.clear();
        return;
    }

    if (!m_state.moving) {
        // Mise en mouvement : l'objet s'est éloigné de sa position de repos de plus que le seuil.
        const QPointF d = pos - m_anchor;
        if (std::hypot(d.x(), d.y()) > threshold) {
            m_state.moving = true;
            m_moveStartPos = m_anchor;
            m_moveStartMs = timeMs;
            m_travelled = std::hypot(d.x(), d.y());
            m_lastMotionPos = pos;
            m_recentPositions.clear();
            addEvent(MotionEvent::MotionStart, timeMs);
            m_events.back().position = m_anchor;  // le mouvement est parti de la position de repos
        }
        return;
    }

    const QPointF step = pos - m_lastMotionPos;
    m_travelled += std::hypot(step.x(), step.y());
    m_lastMotionPos = pos;

    // Retour au repos : sur les 0,5 dernière seconde, l'objet est resté dans un rayon de la moitié
    // du seuil. Un critère de position résiste mieux au tremblement qu'un seuil de vitesse.
    m_recentPositions.emplace_back(timeMs, pos);
    while (m_recentPositions.size() > 2 && timeMs - m_recentPositions[1].first >= 500)
        m_recentPositions.pop_front();
    if (timeMs - m_recentPositions.front().first < 500)
        return;
    double spread = 0.0;
    for (const auto &[t, p] : m_recentPositions)
        spread = std::max(spread, std::hypot(p.x() - pos.x(), p.y() - pos.y()));
    if (spread < threshold / 2) {
        m_state.moving = false;
        m_anchor = pos;
        addEvent(MotionEvent::MotionStop, timeMs);
        MotionEvent &e = m_events.back();
        const QPointF net = pos - m_moveStartPos;
        e.distance = std::hypot(net.x(), net.y());
        e.travelled = m_travelled;
        // Sans la demi-seconde de confirmation de l'arrêt.
        e.duration = std::max<qint64>(0, timeMs - 500 - m_moveStartMs) / 1000.0;
    }
}

void ObjectTracker::computeMotion(const FloorMapper *floor)
{
    m_state.velocity = QPointF();
    m_state.speed = 0.0;
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
