#include "motionfilter.h"
#include "floormapper.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kRadToDeg = 180.0 / M_PI;
constexpr double kDegToRad = M_PI / 180.0;

// Variance de la mesure : environ 1,5 px d'écart-type pour la position d'un carré ou d'un marqueur.
constexpr double kMeasurementNoise = 2.25;

} // namespace

void KalmanFilter2D::setNoise(double processNoise, double measurementNoise)
{
    m_q = processNoise;
    m_r = measurementNoise;
}

void KalmanFilter2D::setStrength(int strength)
{
    // Force 0 -> bruit d'accélération très grand (la mesure est suivie presque telle quelle),
    // force 100 -> très faible (positions très stables mais en retard sur les mouvements rapides).
    setNoise(std::pow(10.0, 5.5 - 0.035 * std::clamp(strength, 0, 100)), kMeasurementNoise);
}

void KalmanFilter2D::reset(const QPointF &position)
{
    const double p[2] = {position.x(), position.y()};
    for (int i = 0; i < 2; ++i) {
        m_axes[i] = Axis();
        m_axes[i].p = p[i];
        m_axes[i].P00 = m_r;
        m_axes[i].P11 = 1e6;  // vitesse initiale inconnue
    }
}

void KalmanFilter2D::predict(double dt)
{
    if (dt <= 0)
        return;
    // Modèle à vitesse constante, accélération en bruit blanc.
    const double dt2 = dt * dt, dt3 = dt2 * dt;
    for (Axis &a : m_axes) {
        a.p += a.v * dt;
        const double P00 = a.P00 + 2 * dt * a.P01 + dt2 * a.P11 + m_q * dt3 / 3;
        const double P01 = a.P01 + dt * a.P11 + m_q * dt2 / 2;
        const double P11 = a.P11 + m_q * dt;
        a.P00 = P00;
        a.P01 = P01;
        a.P11 = P11;
    }
}

void KalmanFilter2D::update(const QPointF &measurement)
{
    const double z[2] = {measurement.x(), measurement.y()};
    for (int i = 0; i < 2; ++i) {
        Axis &a = m_axes[i];
        const double s = a.P00 + m_r;
        const double k0 = a.P00 / s, k1 = a.P01 / s;
        const double innovation = z[i] - a.p;
        a.p += k0 * innovation;
        a.v += k1 * innovation;
        const double P00 = (1 - k0) * a.P00;
        const double P01 = (1 - k0) * a.P01;
        const double P11 = a.P11 - k1 * a.P01;
        a.P00 = P00;
        a.P01 = P01;
        a.P11 = P11;
    }
}

void MotionSmoother::setStrength(int strength)
{
    m_strength = std::clamp(strength, 0, 100);
}

void MotionSmoother::clear()
{
    m_hasSquare = false;
    m_square = Track();
    m_markers.clear();
}

void MotionSmoother::step(Track &track, qint64 timeMs, const QPointF *measurement, double angle,
                          double angleSymmetry, double resetDistance)
{
    track.filter.setStrength(m_strength);

    const double dt = (timeMs - track.lastUpdateMs) / 1000.0;
    track.lastUpdateMs = timeMs;
    track.filter.predict(dt);

    if (measurement) {
        // Saut trop grand (autre objet, détection aberrante) : on repart de la mesure.
        const QPointF d = *measurement - track.filter.position();
        if (std::hypot(d.x(), d.y()) > resetDistance) {
            track.filter.reset(*measurement);
            track.out.trail.clear();
            track.trailTimes.clear();
            track.angleInit = false;
        } else {
            track.filter.update(*measurement);
        }
        track.lastSeenMs = timeMs;

        // Angle lissé par moyenne exponentielle d'un vecteur unitaire (évite le saut de ±180°).
        // Pour un carré, l'angle est défini à 90° près : on lisse 4 × angle.
        const double a = angle * angleSymmetry * kDegToRad;
        const double alpha = 1.0 - 0.009 * m_strength;  // 1 (brut) à 0,1 (très lissé)
        if (!track.angleInit) {
            track.angleX = std::cos(a);
            track.angleY = std::sin(a);
            track.angleInit = true;
        } else {
            track.angleX += alpha * (std::cos(a) - track.angleX);
            track.angleY += alpha * (std::sin(a) - track.angleY);
        }
    }
    track.out.predicted = !measurement;
}

void MotionSmoother::finish(Track &track, qint64 timeMs, const FloorMapper *floor, double angleSymmetry)
{
    SmoothedTrack &o = track.out;
    o.position = track.filter.position();
    o.velocity = track.filter.velocity();
    o.speed = std::hypot(o.velocity.x(), o.velocity.y());
    double angle = std::atan2(track.angleY, track.angleX) * kRadToDeg / angleSymmetry;
    if (angleSymmetry > 1)
        angle = std::fmod(angle + 90.0, 90.0);
    o.angle = angle;

    o.trail.push_back(o.position);
    track.trailTimes.push_back(timeMs);
    while (!track.trailTimes.empty() && timeMs - track.trailTimes.front() > m_trailMs) {
        track.trailTimes.pop_front();
        o.trail.pop_front();
    }

    o.hasFloor = floor && floor->isReady();
    if (o.hasFloor) {
        o.floorPosition = floor->toFloor(o.position, floor->objectHeight());
        // La projection sur le sol n'est pas linéaire : vitesse calculée sur 0,1 s.
        constexpr double h = 0.1;
        o.floorVelocity = (floor->toFloor(o.position + o.velocity * h, floor->objectHeight()) - o.floorPosition) / h;
        o.floorSpeed = std::hypot(o.floorVelocity.x(), o.floorVelocity.y());
        o.floorDirection = std::atan2(o.floorVelocity.y(), o.floorVelocity.x()) * kRadToDeg;
        // Cap sur le sol : direction du haut du marqueur (angle du bord supérieur moins 90° dans
        // l'image), comme FloorMapper::apply(). Pour un carré, défini à 90° près.
        const double a = (o.angle - 90.0) * kDegToRad;
        const QPointF front = o.position + QPointF(std::cos(a), std::sin(a)) * 20.0;
        double heading = floor->heading(o.position, front, floor->objectHeight());
        if (o.kind == SmoothedTrack::Square)
            heading = std::fmod(std::fmod(heading, 90.0) + 90.0, 90.0);
        o.floorHeading = heading;
    }
}

void MotionSmoother::update(qint64 timeMs, const std::vector<DetectedSquare> &squares,
                            const std::vector<DetectedMarker> &markers, const FloorMapper *floor)
{
    // Carré principal (le plus grand).
    if (!squares.empty()) {
        const DetectedSquare &s = squares.front();
        if (!m_hasSquare) {
            m_square = Track();
            m_square.out.kind = SmoothedTrack::Square;
            m_square.filter.setNoise(1, kMeasurementNoise);
            m_square.filter.reset(s.center);
            m_square.lastUpdateMs = timeMs;
            m_hasSquare = true;
        }
        step(m_square, timeMs, &s.center, s.angle, 4.0, std::max(50.0, 2.0 * s.side));
        finish(m_square, timeMs, floor, 4.0);
    } else if (m_hasSquare) {
        if (timeMs - m_square.lastSeenMs > m_holdMs) {
            m_hasSquare = false;
        } else {
            step(m_square, timeMs, nullptr, 0, 4.0, 0);
            finish(m_square, timeMs, floor, 4.0);
        }
    }

    // Marqueurs, identifiés par leur ID.
    for (const DetectedMarker &m : markers) {
        auto it = m_markers.find(m.id);
        if (it == m_markers.end()) {
            Track t;
            t.out.kind = SmoothedTrack::Marker;
            t.out.id = m.id;
            t.filter.setNoise(1, kMeasurementNoise);
            t.filter.reset(m.center);
            t.lastUpdateMs = timeMs;
            it = m_markers.emplace(m.id, t).first;
        }
        // Angle du marqueur = direction de son bord supérieur ; on lisse l'angle de l'image.
        step(it->second, timeMs, &m.center, m.angle, 1.0, std::max(50.0, 2.0 * m.side));
        finish(it->second, timeMs, floor, 1.0);
    }
    for (auto it = m_markers.begin(); it != m_markers.end();) {
        Track &t = it->second;
        if (t.lastUpdateMs == timeMs) {
            ++it;
            continue;
        }
        if (timeMs - t.lastSeenMs > m_holdMs) {
            it = m_markers.erase(it);
            continue;
        }
        step(t, timeMs, nullptr, 0, 1.0, 0);
        finish(t, timeMs, floor, 1.0);
        ++it;
    }
}

std::vector<const SmoothedTrack *> MotionSmoother::tracks() const
{
    std::vector<const SmoothedTrack *> out;
    if (m_hasSquare)
        out.push_back(&m_square.out);
    for (const auto &[id, t] : m_markers)
        out.push_back(&t.out);
    return out;
}
