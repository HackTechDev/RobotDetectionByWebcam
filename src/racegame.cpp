#include "racegame.h"

#include <QPainter>

#include <cmath>

using namespace GameUtil;

QString RaceGame::rules() const
{
    return QObject::tr("Franchissez les portes cochées dans l'ordre de la liste (ordre de l'onglet Zones), sur le "
                       "nombre de tours choisi. Le chrono démarre au premier mouvement du robot ; une mauvaise "
                       "porte coûte %1 s.")
        .arg(int(kPenaltySeconds));
}

std::vector<GameParam> RaceGame::params() const
{
    return {
        {QStringLiteral("robot"), QObject::tr("Robot"), GameParam::Tracked, {}, 0, {},
         QObject::tr("Objet suivi qui joue le rôle du robot (onglet Suivi)")},
        {QStringLiteral("gates"), QObject::tr("Portes (dans l'ordre)"), GameParam::ZoneList, {}, 0, {},
         QObject::tr("Zones à franchir, dans l'ordre de l'onglet Zones")},
        {QStringLiteral("laps"), QObject::tr("Tours"), GameParam::Choice,
         {QObject::tr("1 tour"), QObject::tr("2 tours"), QObject::tr("3 tours")}, 0, {}, {}},
    };
}

QString RaceGame::checkReady(const GameSettings &settings, const GameContext &context) const
{
    const QString base = Game::checkReady(settings, context);
    if (!base.isEmpty())
        return base;
    if (settings.list(QStringLiteral("gates")).size() < 2)
        return QObject::tr("Cochez au moins 2 portes (tracez-les dans l'onglet Zones).");
    return QString();
}

void RaceGame::onStart(const GameContext &)
{
    m_gates = m_settings.list(QStringLiteral("gates"));
    m_laps = m_settings.value(QStringLiteral("laps")) + 1;
    m_next = 0;
    m_lap = 0;
    m_insideGate = -1;
    m_started = false;
    m_hasAnchor = false;
    m_finished = false;
    m_penalties = 0;
    m_lastPenaltyMs = -10000;
    m_newRecord = false;
    m_trail.clear();
    // Le record dépend du parcours : portes, dans l'ordre, et nombre de tours.
    QStringList ids;
    for (int id : m_gates)
        ids << QString::number(id);
    m_course = ids.join(QLatin1Char('-')) + QStringLiteral("x") + QString::number(m_laps);
    m_best = m_bests.value(m_course, 0.0);
}

double RaceGame::elapsed(qint64 timeMs) const
{
    if (!m_started)
        return 0.0;
    return ((m_finished ? m_finishMs : timeMs) - m_startMs) / 1000.0 + m_penalties * kPenaltySeconds;
}

std::vector<MotionEvent> RaceGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    if (!isRunning())
        return events;
    const int robotId = m_settings.value(QStringLiteral("robot"));
    const GameObject *robot = context.object(robotId);
    if (!robot || !robot->state->found)
        return events;
    const QPointF c = robot->state->center;
    const qint64 t = context.timeMs;

    // Départ du chrono au premier mouvement.
    auto startClock = [&] {
        if (m_started)
            return;
        m_started = true;
        m_startMs = t;
        events.push_back(event(context, QStringLiteral("race_start"), QObject::tr("Course : départ"), robotId));
    };
    if (!m_hasAnchor) {
        m_anchor = c;
        m_hasAnchor = true;
    } else if (distance(c, m_anchor) > context.motionPx) {
        startClock();
    }
    if (m_started)
        m_trail << c;

    // Porte où se trouve le robot (centre dans la zone).
    int gateNow = -1;
    for (int i = 0; i < m_gates.size(); ++i) {
        if (context.zoneRect(m_gates[i]).contains(c)) {
            gateNow = i;
            break;
        }
    }
    if (gateNow != m_insideGate && gateNow >= 0) {
        const Zone *zone = context.zone(m_gates[gateNow]);
        const QString name = zone ? zone->name : QString::number(gateNow + 1);
        if (gateNow == m_next) {
            startClock();
            ++m_next;
            events.push_back(event(context, QStringLiteral("race_gate"),
                                   QObject::tr("Course : porte %1/%2 « %3 », tour %4/%5")
                                       .arg(m_next)
                                       .arg(m_gates.size())
                                       .arg(name)
                                       .arg(m_lap + 1)
                                       .arg(m_laps),
                                   robotId, m_next));
            if (m_next == m_gates.size()) {
                m_next = 0;
                ++m_lap;
                if (m_lap == m_laps) {
                    m_finished = true;
                    m_finishMs = t;
                    const double time = elapsed(t);
                    m_newRecord = m_best <= 0 || time < m_best;
                    if (m_newRecord) {
                        m_best = time;
                        m_bests.insert(m_course, time);
                    }
                    events.push_back(event(context, QStringLiteral("race_finish"),
                                           QObject::tr("Course : arrivée en %1 s (%2 pénalité(s))%3")
                                               .arg(seconds(time))
                                               .arg(m_penalties)
                                               .arg(m_newRecord ? QObject::tr(", nouveau record") : QString()),
                                           robotId, m_penalties));
                    finish();
                }
            }
        } else if (m_started) {
            // Mauvaise porte (pas celle attendue) : pénalité.
            ++m_penalties;
            m_lastPenaltyMs = t;
            events.push_back(event(context, QStringLiteral("race_penalty"),
                                   QObject::tr("Course : mauvaise porte « %1 », +%2 s").arg(name).arg(int(kPenaltySeconds)),
                                   robotId, m_penalties));
        }
    }
    m_insideGate = gateNow;
    return events;
}

void RaceGame::draw(const GameDrawContext &draw, const GameContext &context) const
{
    QPainter *p = draw.painter;
    // Tracé de la course.
    if (m_trail.size() > 1) {
        QPolygonF poly;
        for (const QPointF &pt : m_trail)
            poly << draw.toDisplay(pt);
        p->setPen(QPen(QColor(255, 255, 255, 180), draw.penWidth));
        p->drawPolyline(poly);
    }
    for (int i = 0; i < m_gates.size(); ++i) {
        const QRectF r = draw.rectToDisplay(context.zoneRect(m_gates[i]));
        if (r.isEmpty())
            continue;
        const bool next = !m_finished && i == m_next;
        const bool passed = m_finished || i < m_next;
        p->setPen(QPen(next ? QColor(255, 230, 0) : passed ? QColor(40, 220, 80) : QColor(200, 200, 200),
                       draw.penWidth * (next ? 3 : 1), next ? Qt::SolidLine : Qt::DashLine));
        p->setBrush(Qt::NoBrush);
        p->drawRect(r);
        label(draw, r.topLeft() + QPointF(0, -draw.penWidth), QString::number(i + 1),
              next ? QColor(255, 230, 0) : Qt::white);
    }
}

QString RaceGame::bannerText(const GameContext &context) const
{
    const qint64 t = context.timeMs;
    if (m_finished) {
        return m_newRecord ? QObject::tr("ARRIVÉE ! %1 s — NOUVEAU RECORD").arg(seconds(elapsed(t)))
                           : QObject::tr("ARRIVÉE ! %1 s (record %2 s)").arg(seconds(elapsed(t)), seconds(m_best));
    }
    if (t - m_lastPenaltyMs < 1500)
        return QObject::tr("MAUVAISE PORTE ! +%1 s").arg(int(kPenaltySeconds));
    if (!m_started)
        return QObject::tr("Course : partez quand vous voulez");
    return QObject::tr("Porte %1/%2 — tour %3/%4 — %5 s")
        .arg(m_next + 1)
        .arg(m_gates.size())
        .arg(m_lap + 1)
        .arg(m_laps)
        .arg(seconds(elapsed(t)));
}

QColor RaceGame::bannerColor(const GameContext &context) const
{
    if (m_finished)
        return blue();
    if (context.timeMs - m_lastPenaltyMs < 1500)
        return purple();
    return m_started ? green() : grey();
}

QString RaceGame::countersText(const GameContext &context) const
{
    return QObject::tr("Chrono : %1 s — Pénalités : %2 — Record : %3")
        .arg(seconds(elapsed(context.timeMs)))
        .arg(m_penalties)
        .arg(m_best > 0 ? seconds(m_best) + QObject::tr(" s") : QObject::tr("aucun"));
}

QJsonObject RaceGame::toJson(const GameContext &context) const
{
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("robot_id"), m_settings.value(QStringLiteral("robot")));
    o.insert(QStringLiteral("gates"), int(m_gates.size()));
    o.insert(QStringLiteral("next_gate"), m_finished ? 0 : m_next + 1);
    if (!m_finished && m_next < m_gates.size())
        o.insert(QStringLiteral("next_zone_id"), m_gates[m_next]);
    o.insert(QStringLiteral("lap"), std::min(m_lap + 1, m_laps));
    o.insert(QStringLiteral("laps"), m_laps);
    o.insert(QStringLiteral("started"), m_started);
    o.insert(QStringLiteral("finished"), m_finished);
    o.insert(QStringLiteral("elapsed_s"), std::round(elapsed(context.timeMs) * 10) / 10);
    o.insert(QStringLiteral("penalties"), m_penalties);
    o.insert(QStringLiteral("best_s"), std::round(m_best * 10) / 10);
    return o;
}
