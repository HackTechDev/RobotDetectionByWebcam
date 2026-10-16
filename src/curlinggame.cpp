#include "curlinggame.h"

#include <QJsonArray>
#include <QPainter>

#include <cmath>

using namespace GameUtil;

namespace {

constexpr qint64 kStillMs = 1000;   // immobilité pour « prêt » et pour l'arrêt d'un lancer
constexpr qint64 kResultMs = 2500;  // affichage du résultat d'un lancer
constexpr qint64 kLostMs = 1000;    // robot perdu pendant un lancer : 0 point

QColor playerColor(int player)
{
    static const QColor colors[] = {QColor(255, 80, 80), QColor(80, 160, 255), QColor(255, 220, 60),
                                    QColor(120, 230, 120)};
    return colors[player % 4];
}

} // namespace

QString CurlingGame::rules() const
{
    return QObject::tr("Chacun son tour, lancez le robot vers la cible et laissez-le s'arrêter au plus près du "
                       "centre : 10, 5 ou 2 points selon l'anneau, 0 en dehors. Replacez le robot entre deux "
                       "lancers : le lancer suivant est accepté après 1 s d'immobilité.");
}

std::vector<GameParam> CurlingGame::params() const
{
    return {
        {QStringLiteral("robot"), QObject::tr("Robot"), GameParam::Tracked, {}, 0, {},
         QObject::tr("Objet suivi qui joue le rôle de la pierre (onglet Suivi)")},
        {QStringLiteral("target"), QObject::tr("Cible"), GameParam::Zone, {}, 0, {},
         QObject::tr("Zone dont le centre est le centre de la cible (onglet Zones)")},
        {QStringLiteral("players"), QObject::tr("Joueurs"), GameParam::Choice,
         {QObject::tr("1 joueur"), QObject::tr("2 joueurs"), QObject::tr("3 joueurs"), QObject::tr("4 joueurs")}, 1, {}, {}},
        {QStringLiteral("rounds"), QObject::tr("Manches"), GameParam::Choice,
         {QObject::tr("1 manche"), QObject::tr("3 manches"), QObject::tr("5 manches")}, 1, {},
         QObject::tr("Un lancer par joueur et par manche")},
    };
}

void CurlingGame::onStart(const GameContext &context)
{
    m_players = m_settings.value(QStringLiteral("players")) + 1;
    static const int rounds[] = {1, 3, 5};
    m_rounds = rounds[std::clamp(m_settings.value(QStringLiteral("rounds")), 0, 2)];
    m_throw = 0;
    m_state = Placing;
    m_stillSinceMs = -1;
    m_stateMs = context.timeMs;
    m_lostSinceMs = -1;
    m_scores = QVector<int>(m_players, 0);
    m_lastPoints = 0;
    m_stones.clear();
}

QString CurlingGame::playerName(int player) const
{
    return QObject::tr("Joueur %1").arg(player + 1);
}

int CurlingGame::pointsAt(const QPointF &p, const GameContext &context) const
{
    const QRectF zone = context.zoneRect(m_settings.value(QStringLiteral("target")));
    const double radius = std::min(zone.width(), zone.height()) / 2;
    const double d = distance(p, zone.center());
    if (d < radius / 3)
        return 10;
    if (d < 2 * radius / 3)
        return 5;
    if (d < radius)
        return 2;
    return 0;
}

QString CurlingGame::ranking() const
{
    QVector<int> order(m_players);
    for (int i = 0; i < m_players; ++i)
        order[i] = i;
    std::stable_sort(order.begin(), order.end(), [this](int a, int b) { return m_scores[a] > m_scores[b]; });
    QStringList parts;
    for (int i : order)
        parts << QObject::tr("%1 : %2").arg(playerName(i)).arg(m_scores[i]);
    return parts.join(QStringLiteral(" — "));
}

std::vector<MotionEvent> CurlingGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const qint64 t = context.timeMs;
    const int robotId = m_settings.value(QStringLiteral("robot"));
    const GameObject *robot = context.object(robotId);
    m_seen = robot && robot->state->found;
    const int player = m_throw % m_players;

    auto scoreThrow = [&](int points, const QString &how) {
        m_lastPoints = points;
        m_scores[player] += points;
        if (m_seen)
            m_stones.push_back({robot->state->center, player});
        events.push_back(event(context, QStringLiteral("curling_throw"),
                               QObject::tr("Curling : %1, manche %2, %3 point(s)%4")
                                   .arg(playerName(player))
                                   .arg(m_throw / m_players + 1)
                                   .arg(points)
                                   .arg(how),
                               robotId, points));
        ++m_throw;
        m_state = Result;
        m_stateMs = t;
    };

    // Lancer en cours et robot perdu (sorti de l'image...) : 0 point.
    if (m_state == Moving && !m_seen) {
        if (m_lostSinceMs < 0)
            m_lostSinceMs = t;
        else if (t - m_lostSinceMs >= kLostMs)
            scoreThrow(0, QObject::tr(" (robot perdu)"));
        return events;
    }
    m_lostSinceMs = -1;
    if (!m_seen)
        return events;
    const QPointF c = robot->state->center;

    // Immobilité : position stable depuis kStillMs.
    if (m_stillSinceMs < 0 || distance(c, m_stillPos) > context.motionPx) {
        m_stillPos = c;
        m_stillSinceMs = t;
    }
    const bool still = t - m_stillSinceMs >= kStillMs;

    switch (m_state) {
    case Result:
        if (t - m_stateMs < kResultMs)
            break;
        if (m_throw == m_players * m_rounds) {
            m_state = Finished;
            events.push_back(event(context, QStringLiteral("curling_finish"),
                                   QObject::tr("Curling : terminé — %1").arg(ranking()), robotId));
            finish();
            break;
        }
        if (m_throw % m_players == 0)
            m_stones.clear();  // nouvelle manche
        m_state = Placing;
        m_stillSinceMs = -1;
        break;
    case Placing:
        if (still) {
            m_state = Ready;
            m_anchor = c;
        }
        break;
    case Ready:
        if (distance(c, m_anchor) > 2 * context.motionPx) {
            m_state = Moving;
            m_stateMs = t;
        }
        break;
    case Moving:
        if (still && t - m_stateMs > kStillMs)
            scoreThrow(pointsAt(c, context), QString());
        break;
    case Finished:
        break;
    }
    return events;
}

void CurlingGame::draw(const GameDrawContext &draw, const GameContext &context) const
{
    QPainter *p = draw.painter;
    const QRectF zone = context.zoneRect(m_settings.value(QStringLiteral("target")));
    const double radius = std::min(zone.width(), zone.height()) / 2;
    const QPointF c = draw.toDisplay(zone.center());
    const QColor rings[] = {QColor(40, 90, 220, 90), QColor(255, 255, 255, 90), QColor(220, 40, 40, 110)};
    for (int i = 0; i < 3; ++i) {
        const double r = radius * (3 - i) / 3;
        p->setPen(QPen(Qt::white, std::max(1, draw.penWidth / 2)));
        p->setBrush(rings[i]);
        p->drawEllipse(c, r, r);
    }
    p->setFont(draw.font);
    for (int i = 0; i < 3; ++i) {
        const int pts[] = {2, 5, 10};
        p->setPen(Qt::white);
        p->drawText(c + QPointF(radius * (5 - 2 * i) / 6 - draw.penWidth * 3, draw.font.pixelSize() / 3.0),
                    QString::number(pts[i]));
    }
    // Pierres de la manche : position d'arrêt de chaque lancer.
    for (const Stone &s : m_stones) {
        const QPointF d = draw.toDisplay(s.pos);
        p->setPen(QPen(Qt::black, std::max(1, draw.penWidth / 2)));
        p->setBrush(playerColor(s.player));
        p->drawEllipse(d, draw.penWidth * 4.0, draw.penWidth * 4.0);
        p->setPen(Qt::black);
        p->drawText(QRectF(d.x() - 20, d.y() - 20, 40, 40), Qt::AlignCenter, QString::number(s.player + 1));
    }
    p->setBrush(Qt::NoBrush);
}

QString CurlingGame::bannerText(const GameContext &) const
{
    const int player = m_throw % m_players;
    const int round = m_throw / m_players + 1;
    switch (m_state) {
    case Finished: {
        int best = 0;
        for (int i = 1; i < m_players; ++i)
            best = m_scores[i] > m_scores[best] ? i : best;
        return m_players > 1 ? QObject::tr("%1 GAGNE ! %2").arg(playerName(best), ranking())
                             : QObject::tr("TERMINÉ ! %1 points").arg(m_scores[0]);
    }
    case Result: {
        const int previous = (m_throw - 1) % m_players;
        return QObject::tr("%1 : %2 point(s)").arg(playerName(previous)).arg(m_lastPoints);
    }
    case Moving:
        return QObject::tr("%1 : lancer en cours...").arg(playerName(player));
    case Ready:
        return QObject::tr("Manche %1/%2 — %3 : lancez !").arg(round).arg(m_rounds).arg(playerName(player));
    case Placing:
        break;
    }
    if (!m_seen)
        return QObject::tr("Robot non trouvé");
    return QObject::tr("Manche %1/%2 — %3 : placez le robot au départ").arg(round).arg(m_rounds).arg(playerName(player));
}

QColor CurlingGame::bannerColor(const GameContext &) const
{
    switch (m_state) {
    case Finished: return blue();
    case Result: return m_lastPoints >= 5 ? green() : m_lastPoints > 0 ? gold() : red();
    case Moving: return orange();
    case Ready: return playerColor(m_throw % m_players).darker(130);
    case Placing: break;
    }
    return m_seen ? grey() : orange();
}

QString CurlingGame::countersText(const GameContext &) const
{
    return QObject::tr("Lancer %1/%2 — %3")
        .arg(std::min(m_throw + 1, m_players * m_rounds))
        .arg(m_players * m_rounds)
        .arg(ranking());
}

QJsonObject CurlingGame::toJson(const GameContext &context) const
{
    static const char *states[] = {"placing", "ready", "moving", "result", "finished"};
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("robot_id"), m_settings.value(QStringLiteral("robot")));
    o.insert(QStringLiteral("state"), QString::fromLatin1(states[m_state]));
    o.insert(QStringLiteral("players"), m_players);
    o.insert(QStringLiteral("rounds"), m_rounds);
    o.insert(QStringLiteral("round"), std::min(m_throw / m_players + 1, m_rounds));
    o.insert(QStringLiteral("player"), m_throw % m_players + 1);
    QJsonArray scores;
    for (int s : m_scores)
        scores.append(s);
    o.insert(QStringLiteral("scores"), scores);
    o.insert(QStringLiteral("last_points"), m_lastPoints);
    return o;
}
