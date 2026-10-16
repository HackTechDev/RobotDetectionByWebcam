#include "footballgame.h"

#include <QJsonArray>
#include <QPainter>

#include <cmath>

using namespace GameUtil;

namespace {

constexpr qint64 kGoalConfirmMs = 200;
constexpr qint64 kStillMs = 1000;
const QColor kTeam1(230, 60, 60);
const QColor kTeam2(60, 130, 240);

} // namespace

QString FootballGame::rules() const
{
    return QObject::tr("Poussez la balle dans le but adverse : une balle dans le but de l'équipe 1 marque pour "
                       "l'équipe 2, et inversement. Une balle qui sort de l'image est une touche. Après un but ou "
                       "une touche, replacez la balle : le jeu reprend quand elle est immobile depuis 1 s.");
}

std::vector<GameParam> FootballGame::params() const
{
    return {
        {QStringLiteral("ball"), QObject::tr("Balle"), GameParam::Tracked, {}, 1, {},
         QObject::tr("Objet suivi qui joue le rôle de la balle (onglet Suivi)")},
        {QStringLiteral("goal1"), QObject::tr("But de l'équipe 1 (rouge)"), GameParam::Zone, {}, 0, {},
         QObject::tr("But défendu par l'équipe 1 : une balle dedans marque pour l'équipe 2")},
        {QStringLiteral("goal2"), QObject::tr("But de l'équipe 2 (bleue)"), GameParam::Zone, {}, 1, {},
         QObject::tr("But défendu par l'équipe 2 : une balle dedans marque pour l'équipe 1")},
        {QStringLiteral("duration"), QObject::tr("Durée"), GameParam::Choice,
         {QObject::tr("2 minutes"), QObject::tr("3 minutes"), QObject::tr("5 minutes"), QObject::tr("10 minutes")}, 1, {}, {}},
    };
}

QString FootballGame::checkReady(const GameSettings &settings, const GameContext &context) const
{
    const QString base = Game::checkReady(settings, context);
    if (!base.isEmpty())
        return base;
    if (settings.value(QStringLiteral("goal1")) == settings.value(QStringLiteral("goal2")))
        return QObject::tr("Choisissez deux zones différentes pour les buts (tracez-en deux dans l'onglet Zones).");
    return QString();
}

void FootballGame::onStart(const GameContext &context)
{
    m_ball = m_settings.value(QStringLiteral("ball"));
    m_goal1 = m_settings.value(QStringLiteral("goal1"));
    m_goal2 = m_settings.value(QStringLiteral("goal2"));
    static const int minutes[] = {2, 3, 5, 10};
    m_durationMs = qint64(minutes[std::clamp(m_settings.value(QStringLiteral("duration")), 0, 3)]) * 60000;
    m_score1 = m_score2 = 0;
    m_state = Play;
    m_startMs = m_stateMs = context.timeMs;
    m_goalCandidate = 0;
    m_goalSinceMs = -1;
    m_stillSinceMs = -1;
    m_lastScorer = 0;
    m_outs = 0;
}

double FootballGame::remaining(qint64 timeMs) const
{
    return std::max<qint64>(0, m_startMs + m_durationMs - timeMs) / 1000.0;
}

std::vector<MotionEvent> FootballGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const qint64 t = context.timeMs;
    if (t >= m_startMs + m_durationMs) {
        m_state = Finished;
        events.push_back(event(context, QStringLiteral("football_finish"),
                               QObject::tr("Football : terminé, équipe 1 %1 — équipe 2 %2").arg(m_score1).arg(m_score2)));
        finish();
        return events;
    }
    const GameObject *ball = context.object(m_ball);
    if (!ball)
        return events;
    const TrackedObject &b = *ball->state;

    if (m_state == Play) {
        // Touche : balle sortie de l'image.
        if (b.outOfView) {
            m_state = Out;
            m_stateMs = t;
            ++m_outs;
            events.push_back(event(context, QStringLiteral("football_out"), QObject::tr("Football : touche"), m_ball, m_outs));
            return events;
        }
        if (!b.found)
            return events;
        int inGoal = 0;
        if (context.zoneRect(m_goal1).contains(b.center))
            inGoal = 1;
        else if (context.zoneRect(m_goal2).contains(b.center))
            inGoal = 2;
        if (inGoal != m_goalCandidate) {
            m_goalCandidate = inGoal;
            m_goalSinceMs = t;
        } else if (inGoal != 0 && t - m_goalSinceMs >= kGoalConfirmMs) {
            // Balle dans le but 1 : point pour l'équipe 2, et inversement.
            m_lastScorer = inGoal == 1 ? 2 : 1;
            (m_lastScorer == 1 ? m_score1 : m_score2)++;
            m_state = Goal;
            m_stateMs = t;
            m_goalCandidate = 0;
            events.push_back(event(context, QStringLiteral("football_goal"),
                                   QObject::tr("Football : BUT pour l'équipe %1 (%2 à %3)")
                                       .arg(m_lastScorer)
                                       .arg(m_score1)
                                       .arg(m_score2),
                                   m_ball, m_lastScorer));
        }
        return events;
    }

    // But ou touche : reprise quand la balle est replacée, hors des buts, et immobile depuis 1 s.
    if (!b.found || context.zoneRect(m_goal1).contains(b.center) || context.zoneRect(m_goal2).contains(b.center)) {
        m_stillSinceMs = -1;
        return events;
    }
    if (m_stillSinceMs < 0 || distance(b.center, m_stillPos) > context.motionPx) {
        m_stillPos = b.center;
        m_stillSinceMs = t;
    } else if (t - m_stillSinceMs >= kStillMs && t - m_stateMs >= 1500) {
        m_state = Play;
        m_goalCandidate = 0;
        events.push_back(event(context, QStringLiteral("football_kickoff"), QObject::tr("Football : reprise du jeu"), m_ball));
    }
    return events;
}

void FootballGame::draw(const GameDrawContext &draw, const GameContext &context) const
{
    QPainter *p = draw.painter;
    for (const auto &[zone, color, text] : {std::tuple{m_goal1, kTeam1, QObject::tr("But équipe 1")},
                                            std::tuple{m_goal2, kTeam2, QObject::tr("But équipe 2")}}) {
        const QRectF r = draw.rectToDisplay(context.zoneRect(zone));
        if (r.isEmpty())
            continue;
        QColor fill = color;
        fill.setAlpha(60);
        p->setBrush(fill);
        p->setPen(QPen(color, draw.penWidth * 2));
        p->drawRect(r);
        label(draw, r.topLeft(), text, color.lighter(140));
    }
    if (const GameObject *ball = context.object(m_ball); ball && ball->state->found) {
        const QRectF r = draw.rectToDisplay(ball->state->box);
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(Qt::white, draw.penWidth * 2));
        p->drawEllipse(r.center(), r.width() * 0.65, r.height() * 0.65);
    }
    p->setBrush(Qt::NoBrush);
}

QString FootballGame::bannerText(const GameContext &context) const
{
    const QString score = QObject::tr("%1 — %2").arg(m_score1).arg(m_score2);
    switch (m_state) {
    case Finished:
        if (m_score1 == m_score2)
            return QObject::tr("FIN DU MATCH ! Égalité %1").arg(score);
        return QObject::tr("FIN DU MATCH ! L'équipe %1 gagne %2").arg(m_score1 > m_score2 ? 1 : 2).arg(score);
    case Goal:
        return QObject::tr("BUT pour l'équipe %1 ! %2 — replacez la balle").arg(m_lastScorer).arg(score);
    case Out:
        return QObject::tr("TOUCHE ! %1 — replacez la balle").arg(score);
    case Play:
        break;
    }
    return QObject::tr("Équipe 1 %1 équipe 2 — %2").arg(score, clock(remaining(context.timeMs)));
}

QColor FootballGame::bannerColor(const GameContext &) const
{
    switch (m_state) {
    case Finished: return blue();
    case Goal: return m_lastScorer == 1 ? kTeam1.darker(120) : kTeam2.darker(120);
    case Out: return orange();
    case Play: break;
    }
    return green();
}

QString FootballGame::countersText(const GameContext &context) const
{
    return QObject::tr("Équipe 1 (rouge) : %1 — Équipe 2 (bleue) : %2 — Touches : %3 — Temps restant : %4")
        .arg(m_score1)
        .arg(m_score2)
        .arg(m_outs)
        .arg(clock(remaining(context.timeMs)));
}

QJsonObject FootballGame::toJson(const GameContext &context) const
{
    static const char *states[] = {"play", "goal", "out", "finished"};
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("ball_id"), m_ball);
    o.insert(QStringLiteral("goal1_zone_id"), m_goal1);
    o.insert(QStringLiteral("goal2_zone_id"), m_goal2);
    o.insert(QStringLiteral("scores"), QJsonArray{m_score1, m_score2});
    o.insert(QStringLiteral("state"), QString::fromLatin1(states[m_state]));
    o.insert(QStringLiteral("outs"), m_outs);
    o.insert(QStringLiteral("remaining_s"), std::round(remaining(context.timeMs) * 10) / 10);
    return o;
}
