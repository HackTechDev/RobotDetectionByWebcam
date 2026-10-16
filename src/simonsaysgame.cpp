#include "simonsaysgame.h"

#include <QLocale>
#include <QRandomGenerator>
#include <QtMath>

#include <cmath>

using namespace GameUtil;

namespace {

constexpr qint64 kPauseMs = 1500;
constexpr qint64 kResultMs = 2200;
constexpr double kSimonProbability = 0.7;
constexpr double kMoveSides = 1.5;    // avancer ou reculer : au moins 1,5 côté de marqueur
constexpr double kStillSides = 0.5;   // ne pas bouger : moins d'un demi-côté
constexpr double kStillTurn = 20.0;   // ne pas bouger : moins de 20° de rotation

double angleDelta(double from, double to)
{
    // Écart signé dans ]-180, 180] (positif : sens horaire à l'écran).
    double d = std::fmod(to - from, 360.0);
    if (d > 180)
        d -= 360;
    if (d <= -180)
        d += 360;
    return d;
}

} // namespace

QString SimonSaysGame::rules() const
{
    return QObject::tr("Exécutez l'ordre affiché dans le temps imparti, seulement s'il commence par « Jacques a dit ». "
                       "Sinon, c'est un piège : le robot ne doit pas bouger. Avancer ou reculer : au moins une fois et "
                       "demie la taille du marqueur ; tourner : un quart de tour (de 60° à 120°).");
}

std::vector<GameParam> SimonSaysGame::params() const
{
    return {
        {QStringLiteral("marker"), QObject::tr("Marqueur du robot"), GameParam::Marker, {}, 0, {},
         QObject::tr("Numéro du marqueur ArUco collé sur le robot, bord supérieur vers l'avant")},
        {QStringLiteral("orders"), QObject::tr("Nombre d'ordres"), GameParam::Choice,
         {QObject::tr("5 ordres"), QObject::tr("10 ordres"), QObject::tr("15 ordres")}, 1, {}, {}},
        {QStringLiteral("difficulty"), QObject::tr("Difficulté"), GameParam::Choice,
         {QObject::tr("Facile (6 s par ordre)"), QObject::tr("Normal (4 s par ordre)"), QObject::tr("Difficile (3 s par ordre)")},
         1, {}, {}},
    };
}

void SimonSaysGame::onStart(const GameContext &context)
{
    m_marker = m_settings.value(QStringLiteral("marker"));
    static const int orders[] = {5, 10, 15};
    static const int windows[] = {6000, 4000, 3000};
    m_ordersTotal = orders[std::clamp(m_settings.value(QStringLiteral("orders")), 0, 2)];
    m_windowMs = windows[std::clamp(m_settings.value(QStringLiteral("difficulty")), 0, 2)];
    m_done = 0;
    m_score = 0;
    m_phase = Pause;
    m_phaseMs = context.timeMs;
}

QString SimonSaysGame::orderText(Order order)
{
    switch (order) {
    case Forward: return QObject::tr("AVANCE");
    case Backward: return QObject::tr("RECULE");
    case TurnLeft: return QObject::tr("TOURNE À GAUCHE");
    case TurnRight: return QObject::tr("TOURNE À DROITE");
    }
    return QString();
}

void SimonSaysGame::announce(qint64 timeMs)
{
    QRandomGenerator *rng = QRandomGenerator::global();
    m_order = Order(rng->bounded(4));
    m_simon = rng->generateDouble() < kSimonProbability;
    m_phase = Announce;
    m_phaseMs = timeMs;
    m_hasReference = false;
}

std::vector<MotionEvent> SimonSaysGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const qint64 t = context.timeMs;
    const DetectedMarker *m = context.marker(m_marker);
    m_seen = m != nullptr;

    switch (m_phase) {
    case Pause:
        if (t - m_phaseMs >= kPauseMs)
            announce(t);
        break;
    case Announce:
        // Pose de référence : la première où le marqueur est vu après l'annonce.
        if (m && !m_hasReference) {
            m_refPos = m->center;
            m_refAngle = m->angle;
            m_refSide = std::max(10.0, m->side);
            m_hasReference = true;
        }
        if (t - m_phaseMs >= m_windowMs) {
            // Jugement.
            bool good = false;
            if (!m || !m_hasReference) {
                m_result = QObject::tr("Raté : marqueur n° %1 non vu").arg(m_marker);
            } else {
                const double a = qDegreesToRadians(m_refAngle);
                const QPointF front(std::sin(a), -std::cos(a));  // avant du robot dans l'image
                const QPointF d = m->center - m_refPos;
                const double along = d.x() * front.x() + d.y() * front.y();
                const double moved = std::hypot(d.x(), d.y());
                const double turn = angleDelta(m_refAngle, m->angle);
                const bool still = moved < kStillSides * m_refSide && std::abs(turn) < kStillTurn;
                if (!m_simon) {
                    good = still;
                    m_result = good ? QObject::tr("Bien ! Jacques n'avait rien dit")
                                    : QObject::tr("Piège ! Jacques n'avait rien dit");
                } else {
                    switch (m_order) {
                    case Forward: good = along >= kMoveSides * m_refSide; break;
                    case Backward: good = along <= -kMoveSides * m_refSide; break;
                    case TurnLeft: good = turn <= -60 && turn >= -120; break;
                    case TurnRight: good = turn >= 60 && turn <= 120; break;
                    }
                    if (good)
                        m_result = QObject::tr("Bravo !");
                    else if (m_order == TurnLeft || m_order == TurnRight)
                        m_result = QObject::tr("Raté : tourné de %1°").arg(int(std::lround(turn)));
                    else
                        m_result = QObject::tr("Raté : déplacé de %1 marqueur(s)")
                                       .arg(QLocale().toString(along / m_refSide, 'f', 1));
                }
            }
            m_lastGood = good;
            m_score += good;
            ++m_done;
            events.push_back(event(context, good ? QStringLiteral("simon_good") : QStringLiteral("simon_miss"),
                                   QObject::tr("Jacques a dit : ordre %1/%2 « %3%4 » — %5")
                                       .arg(m_done)
                                       .arg(m_ordersTotal)
                                       .arg(m_simon ? QObject::tr("Jacques a dit : ") : QString())
                                       .arg(orderText(m_order).toLower())
                                       .arg(m_result),
                                   0, m_score));
            m_phase = Result;
            m_phaseMs = t;
        }
        break;
    case Result:
        if (t - m_phaseMs < kResultMs)
            break;
        if (m_done == m_ordersTotal) {
            m_phase = Finished;
            events.push_back(event(context, QStringLiteral("simon_finish"),
                                   QObject::tr("Jacques a dit : terminé, %1/%2").arg(m_score).arg(m_ordersTotal), 0,
                                   m_score));
            finish();
        } else {
            m_phase = Pause;
            m_phaseMs = t;
        }
        break;
    case Finished:
        break;
    }
    return events;
}

QString SimonSaysGame::bannerText(const GameContext &context) const
{
    switch (m_phase) {
    case Finished:
        return QObject::tr("TERMINÉ ! %1 ordre(s) réussi(s) sur %2").arg(m_score).arg(m_ordersTotal);
    case Result:
        return m_result;
    case Announce: {
        const int left = int(std::ceil((m_windowMs - (context.timeMs - m_phaseMs)) / 1000.0));
        return QObject::tr("%1%2 ! (%3)")
            .arg(m_simon ? QObject::tr("JACQUES A DIT : ") : QString())
            .arg(orderText(m_order))
            .arg(std::max(0, left));
    }
    case Pause:
        break;
    }
    return m_seen ? QObject::tr("Attention...") : QObject::tr("Marqueur n° %1 non vu").arg(m_marker);
}

QColor SimonSaysGame::bannerColor(const GameContext &) const
{
    switch (m_phase) {
    case Finished: return blue();
    case Result: return m_lastGood ? green() : red();
    case Announce: return purple();  // même couleur avec ou sans « Jacques a dit » : rien ne trahit le piège
    case Pause: break;
    }
    return m_seen ? grey() : orange();
}

QString SimonSaysGame::countersText(const GameContext &) const
{
    return QObject::tr("Ordre %1/%2 — Réussis : %3").arg(std::min(m_done + 1, m_ordersTotal)).arg(m_ordersTotal).arg(m_score);
}

QJsonObject SimonSaysGame::toJson(const GameContext &context) const
{
    static const char *phases[] = {"pause", "announce", "result", "finished"};
    static const char *orders[] = {"forward", "backward", "turn_left", "turn_right"};
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("marker_id"), m_marker);
    o.insert(QStringLiteral("phase"), QString::fromLatin1(phases[m_phase]));
    if (m_phase == Announce || m_phase == Result) {
        o.insert(QStringLiteral("order"), QString::fromLatin1(orders[m_order]));
        o.insert(QStringLiteral("simon"), m_simon);
    }
    if (m_phase == Announce)
        o.insert(QStringLiteral("remaining_s"), std::round(std::max<qint64>(0, m_windowMs - (context.timeMs - m_phaseMs)) / 100.0) / 10);
    o.insert(QStringLiteral("score"), m_score);
    o.insert(QStringLiteral("orders_done"), m_done);
    o.insert(QStringLiteral("orders_total"), m_ordersTotal);
    return o;
}
