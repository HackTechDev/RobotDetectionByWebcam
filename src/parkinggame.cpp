#include "parkinggame.h"

#include <QPainter>

#include <cmath>

using namespace GameUtil;

namespace {

constexpr qint64 kStillMs = 1000;       // immobile pendant 1 s : stationnement jugé
constexpr double kStillAngle = 3.0;     // variation de cap tolérée à l'arrêt, en degrés
constexpr qint64 kMessageMs = 2500;

} // namespace

QString ParkingGame::rules() const
{
    return QObject::tr("Garez le robot (marqueur ArUco) dans la case jaune, tourné dans la direction demandée, "
                       "puis arrêtez-le : le stationnement est jugé après 1 s d'immobilité dans la place (un arrêt ailleurs n'est pas jugé). À chaque réussite, la "
                       "case rétrécit et la tolérance de cap se resserre (5 niveaux).");
}

std::vector<GameParam> ParkingGame::params() const
{
    return {
        {QStringLiteral("marker"), QObject::tr("Marqueur du robot"), GameParam::Marker, {}, 0, {},
         QObject::tr("Numéro du marqueur ArUco collé sur le robot, bord supérieur vers l'avant")},
        {QStringLiteral("zone"), QObject::tr("Place de parking"), GameParam::Zone, {}, 0, {},
         QObject::tr("Zone dans laquelle se garer (onglet Zones)")},
        {QStringLiteral("direction"), QObject::tr("Cap demandé"), GameParam::Choice,
         {QObject::tr("Vers le haut de l'écran"), QObject::tr("Vers la droite de l'écran"),
          QObject::tr("Vers le bas de l'écran"), QObject::tr("Vers la gauche de l'écran")},
         0, {}, QObject::tr("Direction vers laquelle l'avant du robot doit pointer")},
    };
}

void ParkingGame::onStart(const GameContext &)
{
    m_level = 1;
    m_score = 0;
    m_attempts = 0;
    m_finished = false;
    m_armed = true;
    m_stillSinceMs = -1;
    m_seen = false;
    m_message.clear();
    m_messageMs = -10000;
}

double ParkingGame::targetHeading() const
{
    // Angle du marqueur : 0 = bord supérieur vers le haut, sens horaire positif.
    static const double headings[] = {0.0, 90.0, 180.0, -90.0};
    return headings[std::clamp(m_settings.value(QStringLiteral("direction")), 0, 3)];
}

double ParkingGame::tolerance() const
{
    return 30.0 - 5.0 * (m_level - 1);
}

double ParkingGame::angleDiff(double a, double b)
{
    double d = std::fmod(std::abs(a - b), 360.0);
    return d > 180.0 ? 360.0 - d : d;
}

QRectF ParkingGame::target(const GameContext &context) const
{
    const QRectF zone = context.zoneRect(m_settings.value(QStringLiteral("zone")));
    const double f = 1.0 - 0.15 * (std::min(m_level, kLevels) - 1);
    QRectF r(0, 0, zone.width() * f, zone.height() * f);
    r.moveCenter(zone.center());
    return r;
}

std::vector<MotionEvent> ParkingGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const qint64 t = context.timeMs;
    const DetectedMarker *m = context.marker(m_settings.value(QStringLiteral("marker")));
    m_seen = m != nullptr;
    if (!m) {
        m_stillSinceMs = -1;
        return events;
    }
    const QRectF box = target(context);
    m_inside = true;
    for (const QPointF &p : m->corners)
        m_inside = m_inside && box.contains(p);
    m_headingError = angleDiff(m->angle, targetHeading());

    // Le robot a bougé ou tourné depuis le dernier jugement : un nouveau stationnement sera jugé.
    if (!m_armed && (distance(m->center, m_judgedAt) > 3 * context.motionPx || angleDiff(m->angle, m_judgedAngle) > 20.0))
        m_armed = true;
    // Immobilité : position et cap stables depuis kStillMs.
    if (m_stillSinceMs < 0 || distance(m->center, m_stillPos) > context.motionPx
        || angleDiff(m->angle, m_stillAngle) > kStillAngle) {
        m_stillPos = m->center;
        m_stillAngle = m->angle;
        m_stillSinceMs = t;
        return events;
    }
    if (!m_armed || t - m_stillSinceMs < kStillMs)
        return events;
    // Arrêt hors de la place (manœuvre ailleurs) : pas jugé.
    if (!context.zoneRect(m_settings.value(QStringLiteral("zone"))).contains(m->center))
        return events;

    // Jugement du stationnement.
    m_armed = false;
    m_judgedAt = m->center;
    m_judgedAngle = m->angle;
    m_messageMs = t;
    const double halfDiagonal = std::hypot(box.width(), box.height()) / 2;
    const double positionError = std::min(1.0, distance(m->center, box.center()) / std::max(1.0, halfDiagonal));
    if (m_inside && m_headingError <= tolerance()) {
        const int points = std::max(10, int(std::lround(100 - 50 * positionError - 50 * m_headingError / tolerance())));
        m_score += points;
        m_messageGood = true;
        m_message = QObject::tr("Bien garé ! +%1 points").arg(points);
        events.push_back(event(context, QStringLiteral("parking_success"),
                               QObject::tr("Parking : niveau %1 réussi, %2 points (écart de cap %3°)")
                                   .arg(m_level)
                                   .arg(points)
                                   .arg(int(std::lround(m_headingError))),
                               0, points));
        if (m_level == kLevels) {
            m_finished = true;
            events.push_back(event(context, QStringLiteral("parking_finish"),
                                   QObject::tr("Parking : terminé, %1 points, %2 essai(s) raté(s)")
                                       .arg(m_score)
                                       .arg(m_attempts),
                                   0, m_score));
            finish();
        } else {
            ++m_level;
        }
    } else {
        ++m_attempts;
        m_messageGood = false;
        m_message = !m_inside ? QObject::tr("Raté : pas entièrement dans la case")
                              : QObject::tr("Raté : cap décalé de %1°").arg(int(std::lround(m_headingError)));
        events.push_back(event(context, QStringLiteral("parking_miss"), QObject::tr("Parking : %1").arg(m_message), 0,
                               m_attempts));
    }
    return events;
}

void ParkingGame::draw(const GameDrawContext &draw, const GameContext &context) const
{
    QPainter *p = draw.painter;
    const QRectF zone = draw.rectToDisplay(context.zoneRect(m_settings.value(QStringLiteral("zone"))));
    const QRectF box = draw.rectToDisplay(target(context));
    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(QColor(200, 200, 200), draw.penWidth, Qt::DashLine));
    p->drawRect(zone);
    p->setPen(QPen(m_inside && m_headingError <= tolerance() ? QColor(40, 220, 80) : QColor(255, 230, 0),
                   draw.penWidth * 3));
    p->drawRect(box);
    // Flèche du cap demandé, au centre de la case (miroir pris en compte par toDisplay).
    const double a = qDegreesToRadians(targetHeading());
    const QPointF c = context.zoneRect(m_settings.value(QStringLiteral("zone"))).center();
    const double len = std::min(box.width(), box.height()) * 0.35;
    const QPointF tip = draw.toDisplay(c + QPointF(std::sin(a) * len, -std::cos(a) * len));
    const QPointF base = draw.toDisplay(c);
    p->setPen(QPen(QColor(255, 230, 0), draw.penWidth * 2));
    p->drawLine(base, tip);
    const QPointF dir = (tip - base) / std::max(1.0, std::hypot(tip.x() - base.x(), tip.y() - base.y()));
    const QPointF side(-dir.y(), dir.x());
    p->drawLine(tip, tip - dir * draw.penWidth * 5 + side * draw.penWidth * 4);
    p->drawLine(tip, tip - dir * draw.penWidth * 5 - side * draw.penWidth * 4);
}

QString ParkingGame::bannerText(const GameContext &context) const
{
    if (m_finished)
        return QObject::tr("PARKING TERMINÉ ! %1 points").arg(m_score);
    if (context.timeMs - m_messageMs < kMessageMs)
        return m_message;
    if (!m_seen)
        return QObject::tr("Marqueur n° %1 non vu").arg(m_settings.value(QStringLiteral("marker")));
    return QObject::tr("Niveau %1/%2 — garez-vous — cap : %3° d'écart (max %4°)")
        .arg(m_level)
        .arg(kLevels)
        .arg(int(std::lround(m_headingError)))
        .arg(int(tolerance()));
}

QColor ParkingGame::bannerColor(const GameContext &context) const
{
    if (m_finished)
        return blue();
    if (context.timeMs - m_messageMs < kMessageMs)
        return m_messageGood ? green() : red();
    if (!m_seen)
        return orange();
    return grey();
}

QString ParkingGame::countersText(const GameContext &) const
{
    return QObject::tr("Niveau %1/%2 — Score : %3 — Essais ratés : %4 — Tolérance de cap : %5°")
        .arg(std::min(m_level, kLevels))
        .arg(kLevels)
        .arg(m_score)
        .arg(m_attempts)
        .arg(int(tolerance()));
}

QJsonObject ParkingGame::toJson(const GameContext &context) const
{
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("marker_id"), m_settings.value(QStringLiteral("marker")));
    o.insert(QStringLiteral("zone_id"), m_settings.value(QStringLiteral("zone")));
    o.insert(QStringLiteral("level"), m_level);
    o.insert(QStringLiteral("levels"), kLevels);
    o.insert(QStringLiteral("score"), m_score);
    o.insert(QStringLiteral("misses"), m_attempts);
    o.insert(QStringLiteral("target_heading_deg"), targetHeading());
    o.insert(QStringLiteral("tolerance_deg"), tolerance());
    o.insert(QStringLiteral("seen"), m_seen);
    o.insert(QStringLiteral("heading_error_deg"), std::round(m_headingError));
    o.insert(QStringLiteral("inside"), m_inside);
    o.insert(QStringLiteral("finished"), m_finished);
    return o;
}
