#include "collectgame.h"

#include <QJsonArray>
#include <QPainter>

#include <cmath>

using namespace GameUtil;

namespace {

constexpr qint64 kConfirmMs = 300;
constexpr qint64 kFlashMs = 1500;

} // namespace

QString CollectGame::rules() const
{
    return QObject::tr("Poussez avec le robot le plus d'objets possible dans la zone de collecte avant la fin du "
                       "temps. Tous les objets suivis autres que le robot sont à ramasser ; un objet compte quand "
                       "son centre est dans la zone.");
}

std::vector<GameParam> CollectGame::params() const
{
    return {
        {QStringLiteral("robot"), QObject::tr("Robot"), GameParam::Tracked, {}, 0, {},
         QObject::tr("Objet suivi qui joue le rôle du robot ; les autres objets suivis sont à ramasser")},
        {QStringLiteral("zone"), QObject::tr("Zone de collecte"), GameParam::Zone, {}, 0, {},
         QObject::tr("Zone où pousser les objets (onglet Zones)")},
        {QStringLiteral("duration"), QObject::tr("Durée"), GameParam::Choice,
         {QObject::tr("1 minute"), QObject::tr("2 minutes"), QObject::tr("3 minutes"), QObject::tr("5 minutes")}, 1, {}, {}},
    };
}

QString CollectGame::checkReady(const GameSettings &settings, const GameContext &context) const
{
    const QString base = Game::checkReady(settings, context);
    if (!base.isEmpty())
        return base;
    if (context.tracked.size() < 2)
        return QObject::tr("Ajoutez dans l'onglet Suivi les objets à ramasser, en plus du robot.");
    return QString();
}

void CollectGame::onStart(const GameContext &context)
{
    m_robot = m_settings.value(QStringLiteral("robot"));
    m_zone = m_settings.value(QStringLiteral("zone"));
    static const int minutes[] = {1, 2, 3, 5};
    m_durationMs = qint64(minutes[std::clamp(m_settings.value(QStringLiteral("duration")), 0, 3)]) * 60000;
    m_startMs = context.timeMs;
    m_items.clear();
    m_finished = false;
    m_lastChangeMs = -10000;
}

double CollectGame::remaining(qint64 timeMs) const
{
    return std::max<qint64>(0, m_startMs + m_durationMs - timeMs) / 1000.0;
}

int CollectGame::collected() const
{
    int n = 0;
    for (const ItemState &s : m_items)
        n += s.inside;
    return n;
}

std::vector<MotionEvent> CollectGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const qint64 t = context.timeMs;
    const QRectF zone = context.zoneRect(m_zone);
    // Objets à ramasser : tous les objets suivis sauf le robot (y compris ceux ajoutés en cours).
    m_total = 0;
    for (const GameObject &o : context.tracked)
        m_total += o.id != m_robot;
    for (const GameObject &o : context.tracked) {
        if (o.id == m_robot)
            continue;
        ItemState &s = m_items[o.id];
        if (!o.state->found)
            continue;  // objet caché ou perdu : son état ne change pas
        const bool in = zone.contains(o.state->center);
        if (in == s.inside) {
            s.candidate = in;
            continue;
        }
        if (in != s.candidate) {
            s.candidate = in;
            s.sinceMs = t;
        } else if (t - s.sinceMs >= kConfirmMs) {
            s.inside = in;
            m_lastChangeMs = t;
            m_lastGood = in;
            m_lastChange = in ? QObject::tr("Objet « %1 » ramassé !").arg(o.name)
                              : QObject::tr("Objet « %1 » ressorti").arg(o.name);
            events.push_back(event(context, in ? QStringLiteral("collect_in") : QStringLiteral("collect_out"),
                                   QObject::tr("Ramassage : %1 (%2/%3 dans la zone)")
                                       .arg(m_lastChange)
                                       .arg(collected())
                                       .arg(m_total),
                                   o.id, collected()));
        }
    }
    // Objets retirés du suivi : plus comptés.
    for (auto it = m_items.begin(); it != m_items.end();)
        it = context.object(it.key()) ? std::next(it) : m_items.erase(it);

    if (t >= m_startMs + m_durationMs) {
        m_finished = true;
        events.push_back(event(context, QStringLiteral("collect_finish"),
                               QObject::tr("Ramassage : terminé, %1 objet(s) ramassé(s) sur %2").arg(collected()).arg(m_total),
                               m_robot, collected()));
        finish();
    }
    return events;
}

void CollectGame::draw(const GameDrawContext &draw, const GameContext &context) const
{
    QPainter *p = draw.painter;
    const QRectF zone = draw.rectToDisplay(context.zoneRect(m_zone));
    p->setBrush(QColor(255, 200, 0, 45));
    p->setPen(QPen(QColor(255, 200, 0), draw.penWidth * 2));
    p->drawRect(zone);
    label(draw, zone.topLeft(), QObject::tr("Collecte : %1/%2").arg(collected()).arg(m_total), QColor(255, 220, 80));
    for (const GameObject &o : context.tracked) {
        if (o.id == m_robot || !(o.state->found || o.state->predicted))
            continue;
        const bool in = m_items.value(o.id).inside;
        const QRectF r = draw.rectToDisplay(o.state->box);
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(in ? QColor(60, 230, 100) : QColor(255, 150, 40), draw.penWidth * 2));
        p->drawEllipse(r.center(), r.width() * 0.6, r.height() * 0.6);
    }
    p->setBrush(Qt::NoBrush);
}

QString CollectGame::bannerText(const GameContext &context) const
{
    if (m_finished)
        return QObject::tr("TERMINÉ ! %1 objet(s) ramassé(s) sur %2").arg(collected()).arg(m_total);
    if (context.timeMs - m_lastChangeMs < kFlashMs)
        return m_lastChange;
    return QObject::tr("Ramassage — %1/%2 dans la zone — %3")
        .arg(collected())
        .arg(m_total)
        .arg(clock(remaining(context.timeMs)));
}

QColor CollectGame::bannerColor(const GameContext &context) const
{
    if (m_finished)
        return blue();
    if (context.timeMs - m_lastChangeMs < kFlashMs)
        return m_lastGood ? green() : red();
    return grey();
}

QString CollectGame::countersText(const GameContext &context) const
{
    return QObject::tr("Objets ramassés : %1 sur %2 — Temps restant : %3")
        .arg(collected())
        .arg(m_total)
        .arg(clock(remaining(context.timeMs)));
}

QJsonObject CollectGame::toJson(const GameContext &context) const
{
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("robot_id"), m_robot);
    o.insert(QStringLiteral("zone_id"), m_zone);
    QJsonArray inside;
    for (auto it = m_items.cbegin(); it != m_items.cend(); ++it) {
        if (it->inside)
            inside.append(it.key());
    }
    o.insert(QStringLiteral("in_zone"), inside);
    o.insert(QStringLiteral("collected"), collected());
    o.insert(QStringLiteral("total"), m_total);
    o.insert(QStringLiteral("remaining_s"), std::round(remaining(context.timeMs) * 10) / 10);
    o.insert(QStringLiteral("finished"), m_finished);
    return o;
}
