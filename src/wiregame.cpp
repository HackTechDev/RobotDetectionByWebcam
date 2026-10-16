#include "wiregame.h"

#include <QPainter>

#include <cmath>

using namespace GameUtil;

namespace {

constexpr qint64 kConfirmMs = 200;  // sortie ou arrivée confirmée après 0,2 s

} // namespace

QString WireGame::rules() const
{
    return QObject::tr("Suivez le couloir formé par les zones cochées, de la première (départ) à la dernière "
                       "(arrivée), sans que le robot en dépasse : chaque sortie est une touche (+%1 s). Avec une "
                       "vitesse maximale, chaque excès coûte %2 s.")
        .arg(int(kTouchPenalty))
        .arg(int(kSpeedPenalty));
}

std::vector<GameParam> WireGame::params() const
{
    return {
        {QStringLiteral("robot"), QObject::tr("Robot"), GameParam::Tracked, {}, 0, {},
         QObject::tr("Objet suivi qui joue le rôle du robot (onglet Suivi)")},
        {QStringLiteral("corridor"), QObject::tr("Couloir (départ → arrivée)"), GameParam::ZoneList, {}, 0, {},
         QObject::tr("Zones qui forment le couloir, dans l'ordre de l'onglet Zones : la première est le départ, la "
                     "dernière l'arrivée ; elles doivent se toucher ou se chevaucher")},
        {QStringLiteral("speed"), QObject::tr("Vitesse maximale"), GameParam::Choice,
         {QObject::tr("Pas de limite"), QObject::tr("Moyenne (12 % de la largeur de l'image par seconde)"),
          QObject::tr("Lente (6 % de la largeur de l'image par seconde)")},
         0, {}, QObject::tr("Variante « main tremblante »")},
    };
}

QString WireGame::checkReady(const GameSettings &settings, const GameContext &context) const
{
    const QString base = Game::checkReady(settings, context);
    if (!base.isEmpty())
        return base;
    if (settings.list(QStringLiteral("corridor")).size() < 2)
        return QObject::tr("Cochez au moins 2 zones pour le couloir (un départ et une arrivée).");
    return QString();
}

void WireGame::onStart(const GameContext &)
{
    m_zones = m_settings.list(QStringLiteral("corridor"));
    m_phase = Waiting;
    m_touches = 0;
    m_overspeeds = 0;
    m_outside = false;
    m_outsideSinceMs = -1;
    m_tooFast = false;
    m_lastPenaltyMs = -10000;
    m_finishSinceMs = -1;
}

double WireGame::speedLimit(const GameContext &context) const
{
    switch (m_settings.value(QStringLiteral("speed"))) {
    case 1: return 0.12 * context.imageSize.width();
    case 2: return 0.06 * context.imageSize.width();
    default: return 0.0;
    }
}

double WireGame::elapsed(qint64 timeMs) const
{
    if (m_phase == Waiting || m_phase == Ready)
        return 0.0;
    return ((m_phase == Finished ? m_finishMs : timeMs) - m_startMs) / 1000.0;
}

std::vector<MotionEvent> WireGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const int robotId = m_settings.value(QStringLiteral("robot"));
    const GameObject *robot = context.object(robotId);
    m_seen = robot && robot->state->found;
    if (!m_seen)
        return events;
    const qint64 t = context.timeMs;
    const QRectF box = robot->state->box;
    m_speed = robot->state->speed;
    const QRectF start = context.zoneRect(m_zones.first());
    const QRectF finishZone = context.zoneRect(m_zones.last());

    if (m_phase == Waiting) {
        // Le robot doit d'abord être entièrement dans le départ.
        if (start.contains(box))
            m_phase = Ready;
        return events;
    }
    if (m_phase == Ready) {
        if (start.contains(box))
            return events;
        m_phase = Running;
        m_startMs = t;
        events.push_back(event(context, QStringLiteral("wire_start"), QObject::tr("Fil électrique : départ"), robotId));
    }

    // Sortie du couloir : cadre qui dépasse de l'union des zones, confirmée après 0,2 s.
    std::vector<QRectF> rects;
    for (int id : m_zones)
        rects.push_back(context.zoneRect(id));
    const bool inside = insideUnion(box, rects);
    if (inside) {
        m_outside = false;
        m_outsideSinceMs = -1;
    } else if (!m_outside) {
        if (m_outsideSinceMs < 0) {
            m_outsideSinceMs = t;
        } else if (t - m_outsideSinceMs >= kConfirmMs) {
            m_outside = true;
            ++m_touches;
            m_lastPenaltyMs = t;
            m_lastPenalty = QObject::tr("TOUCHÉ ! +%1 s").arg(int(kTouchPenalty));
            events.push_back(event(context, QStringLiteral("wire_touch"),
                                   QObject::tr("Fil électrique : touche n° %1 (+%2 s)").arg(m_touches).arg(int(kTouchPenalty)),
                                   robotId, m_touches));
        }
    }

    // Variante « main tremblante » : un excès compte une fois, jusqu'au retour sous la limite.
    const double limit = speedLimit(context);
    if (limit > 0) {
        if (!m_tooFast && m_speed > limit) {
            m_tooFast = true;
            ++m_overspeeds;
            m_lastPenaltyMs = t;
            m_lastPenalty = QObject::tr("TROP VITE ! +%1 s").arg(int(kSpeedPenalty));
            events.push_back(event(context, QStringLiteral("wire_speed"),
                                   QObject::tr("Fil électrique : excès de vitesse n° %1 (+%2 s)")
                                       .arg(m_overspeeds)
                                       .arg(int(kSpeedPenalty)),
                                   robotId, m_overspeeds));
        } else if (m_tooFast && m_speed < 0.8 * limit) {
            m_tooFast = false;
        }
    }

    // Arrivée : robot entièrement dans la dernière zone, confirmé après 0,2 s.
    if (!finishZone.contains(box)) {
        m_finishSinceMs = -1;
    } else if (m_finishSinceMs < 0) {
        m_finishSinceMs = t;
    } else if (t - m_finishSinceMs >= kConfirmMs) {
        m_phase = Finished;
        m_finishMs = t;
        events.push_back(event(context, QStringLiteral("wire_finish"),
                               QObject::tr("Fil électrique : arrivée en %1 s, %2 touche(s), %3 excès, score %4 s")
                                   .arg(seconds(elapsed(t)))
                                   .arg(m_touches)
                                   .arg(m_overspeeds)
                                   .arg(seconds(score(t))),
                               robotId, m_touches));
        finish();
    }
    return events;
}

void WireGame::draw(const GameDrawContext &draw, const GameContext &context) const
{
    QPainter *p = draw.painter;
    const bool alarm = m_outside && m_phase == Running;
    for (int i = 0; i < m_zones.size(); ++i) {
        const QRectF r = draw.rectToDisplay(context.zoneRect(m_zones[i]));
        if (r.isEmpty())
            continue;
        p->setPen(Qt::NoPen);
        p->setBrush(alarm ? QColor(255, 40, 40, 70) : QColor(80, 200, 255, 45));
        p->drawRect(r);
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(alarm ? QColor(255, 60, 60) : QColor(80, 200, 255), draw.penWidth));
        p->drawRect(r);
    }
    const QRectF start = draw.rectToDisplay(context.zoneRect(m_zones.value(0)));
    const QRectF end = draw.rectToDisplay(context.zoneRect(m_zones.value(m_zones.size() - 1)));
    label(draw, start.topLeft(), QObject::tr("Départ"), QColor(120, 255, 120));
    label(draw, end.topLeft(), QObject::tr("Arrivée"), QColor(255, 230, 0));
}

QString WireGame::bannerText(const GameContext &context) const
{
    const qint64 t = context.timeMs;
    if (m_phase == Finished)
        return QObject::tr("ARRIVÉE ! %1 s — %2 touche(s) — score %3 s")
            .arg(seconds(elapsed(t)))
            .arg(m_touches)
            .arg(seconds(score(t)));
    if (!m_seen)
        return QObject::tr("Robot non trouvé");
    if (m_phase == Waiting)
        return QObject::tr("Placez le robot entièrement dans le départ");
    if (m_phase == Ready)
        return QObject::tr("Prêt : partez quand vous voulez");
    if (m_outside)
        return QObject::tr("TOUCHÉ ! Revenez dans le couloir");
    if (t - m_lastPenaltyMs < 1500)
        return m_lastPenalty;
    return QObject::tr("%1 s — touches : %2").arg(seconds(elapsed(t))).arg(m_touches);
}

QColor WireGame::bannerColor(const GameContext &context) const
{
    if (m_phase == Finished)
        return blue();
    if (!m_seen)
        return orange();
    if (m_phase != Running)
        return grey();
    if (m_outside || context.timeMs - m_lastPenaltyMs < 1500)
        return red();
    return green();
}

QString WireGame::countersText(const GameContext &context) const
{
    QString text = QObject::tr("Chrono : %1 s — Touches : %2 — Excès de vitesse : %3 — Score : %4 s")
                       .arg(seconds(elapsed(context.timeMs)))
                       .arg(m_touches)
                       .arg(m_overspeeds)
                       .arg(seconds(score(context.timeMs)));
    if (speedLimit(context) > 0)
        text += QObject::tr("\nVitesse : %1 px/s (max %2)").arg(int(m_speed)).arg(int(speedLimit(context)));
    return text;
}

QJsonObject WireGame::toJson(const GameContext &context) const
{
    static const char *phases[] = {"waiting", "ready", "running", "finished"};
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("robot_id"), m_settings.value(QStringLiteral("robot")));
    o.insert(QStringLiteral("phase"), QString::fromLatin1(phases[m_phase]));
    o.insert(QStringLiteral("inside"), !m_outside);
    o.insert(QStringLiteral("touches"), m_touches);
    o.insert(QStringLiteral("overspeeds"), m_overspeeds);
    o.insert(QStringLiteral("speed_limit_px_s"), std::round(speedLimit(context)));
    o.insert(QStringLiteral("elapsed_s"), std::round(elapsed(context.timeMs) * 10) / 10);
    o.insert(QStringLiteral("score_s"), std::round(score(context.timeMs) * 10) / 10);
    return o;
}
