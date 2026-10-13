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
// Fenêtre de calcul de la vitesse.
constexpr qint64 kVelocityWindowMs = 500;
constexpr qint64 kMinVelocityWindowMs = 150;
constexpr double kRadToDeg = 180.0 / M_PI;

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

    m_state = TrackedObject();
    m_state.active = true;
    m_state.found = true;
    m_state.box = QRectF(roi.x * scale, roi.y * scale, roi.width * scale, roi.height * scale);
    m_state.center = m_state.box.center();
    m_trail.push_back({timeMs, m_state.center});
    return true;
}

void ObjectTracker::stop()
{
    m_tracker.release();
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

    // Objet perdu : on garde sa dernière position et on continue d'essayer aux images suivantes.
    m_state.found = found;
    if (found) {
        m_state.box = QRectF(roi.x * scale, roi.y * scale, roi.width * scale, roi.height * scale);
        m_state.center = m_state.box.center();
        m_trail.push_back({timeMs, m_state.center});
    }
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

    if (!m_state.found || m_trail.size() < 2)
        return;

    // Vitesse entre la position actuelle et la plus ancienne position de la fenêtre.
    const TrailPoint &last = m_trail.back();
    const TrailPoint *first = &last;
    for (auto it = m_trail.rbegin(); it != m_trail.rend(); ++it) {
        if (last.timeMs - it->timeMs > kVelocityWindowMs)
            break;
        first = &*it;
    }
    const double dt = (last.timeMs - first->timeMs) / 1000.0;
    if (dt * 1000 < kMinVelocityWindowMs)
        return;

    m_state.velocity = (last.position - first->position) / dt;
    m_state.speed = std::hypot(m_state.velocity.x(), m_state.velocity.y());
    m_state.direction = std::atan2(m_state.velocity.y(), m_state.velocity.x()) * kRadToDeg;

    // En mouvement : plus de 10 % de la taille de l'objet par seconde (au moins 5 px/s), pour
    // ne pas réagir au léger tremblement du cadre.
    const double size = std::hypot(m_state.box.width(), m_state.box.height());
    m_state.moving = m_state.speed > std::max(5.0, 0.1 * size);

    if (floorReady) {
        const QPointF d = floor->toFloor(last.position) - floor->toFloor(first->position);
        m_state.floorVelocity = d / dt;
        m_state.floorSpeed = std::hypot(m_state.floorVelocity.x(), m_state.floorVelocity.y());
        m_state.floorDirection = std::atan2(d.y(), d.x()) * kRadToDeg;
    }
}
