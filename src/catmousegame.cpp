#include "catmousegame.h"

#include <QJsonArray>
#include <QPainter>

#include <cmath>

using namespace GameUtil;

namespace {

constexpr qint64 kCooldownMs = 3000;
constexpr qint64 kFlashMs = 2000;

} // namespace

QString CatMouseGame::rules() const
{
    return QObject::tr("Le chat marque un point quand il touche la souris (cadres en contact), sauf si la souris est "
                       "au refuge. Après une prise, séparez-vous avant la suivante. Les rôles s'inversent "
                       "régulièrement ; le plus de points à la fin du temps gagne.");
}

std::vector<GameParam> CatMouseGame::params() const
{
    return {
        {QStringLiteral("a"), QObject::tr("Joueur 1 (chat au départ)"), GameParam::Tracked, {}, 0, {},
         QObject::tr("Premier objet suivi (onglet Suivi)")},
        {QStringLiteral("b"), QObject::tr("Joueur 2 (souris au départ)"), GameParam::Tracked, {}, 1, {},
         QObject::tr("Second objet suivi (onglet Suivi)")},
        {QStringLiteral("refuge"), QObject::tr("Refuge de la souris"), GameParam::ZoneOptional, {}, 0,
         QObject::tr("Aucun refuge"), QObject::tr("Zone où la souris ne peut pas être attrapée")},
        {QStringLiteral("duration"), QObject::tr("Durée"), GameParam::Choice,
         {QObject::tr("1 minute"), QObject::tr("2 minutes"), QObject::tr("3 minutes"), QObject::tr("5 minutes")}, 1, {}, {}},
        {QStringLiteral("swap"), QObject::tr("Changement de rôles"), GameParam::Choice,
         {QObject::tr("Toutes les 20 s"), QObject::tr("Toutes les 30 s"), QObject::tr("Toutes les 60 s")}, 1, {}, {}},
    };
}

QString CatMouseGame::checkReady(const GameSettings &settings, const GameContext &context) const
{
    const QString base = Game::checkReady(settings, context);
    if (!base.isEmpty())
        return base;
    if (settings.value(QStringLiteral("a")) == settings.value(QStringLiteral("b")))
        return QObject::tr("Choisissez deux objets suivis différents (ajoutez-en un second dans l'onglet Suivi).");
    return QString();
}

void CatMouseGame::onStart(const GameContext &context)
{
    m_a = m_settings.value(QStringLiteral("a"));
    m_b = m_settings.value(QStringLiteral("b"));
    static const int minutes[] = {1, 2, 3, 5};
    static const int swaps[] = {20, 30, 60};
    m_durationMs = qint64(minutes[std::clamp(m_settings.value(QStringLiteral("duration")), 0, 3)]) * 60000;
    m_swapMs = qint64(swaps[std::clamp(m_settings.value(QStringLiteral("swap")), 0, 2)]) * 1000;
    m_scoreA = m_scoreB = 0;
    m_swapped = false;
    m_startMs = m_lastSwapMs = context.timeMs;
    m_contact = false;
    m_needSeparation = false;
    m_lastCatchMs = -10000;
    m_mouseSafe = false;
    m_finished = false;
}

QString CatMouseGame::nameOf(const GameContext &context, int id) const
{
    const GameObject *o = context.object(id);
    return o ? o->name : QObject::tr("Objet %1").arg(id);
}

double CatMouseGame::remaining(qint64 timeMs) const
{
    return std::max<qint64>(0, m_startMs + m_durationMs - timeMs) / 1000.0;
}

double CatMouseGame::nextSwap(qint64 timeMs) const
{
    return std::max<qint64>(0, m_lastSwapMs + m_swapMs - timeMs) / 1000.0;
}

std::vector<MotionEvent> CatMouseGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const qint64 t = context.timeMs;
    if (t >= m_startMs + m_durationMs) {
        m_finished = true;
        events.push_back(event(context, QStringLiteral("cat_mouse_finish"),
                               QObject::tr("Chat et souris : terminé, %1 %2 — %3 %4")
                                   .arg(nameOf(context, m_a))
                                   .arg(m_scoreA)
                                   .arg(nameOf(context, m_b))
                                   .arg(m_scoreB)));
        finish();
        return events;
    }
    if (t - m_lastSwapMs >= m_swapMs) {
        m_swapped = !m_swapped;
        m_lastSwapMs = t;
        m_needSeparation = true;  // pas de prise immédiate au changement de rôles
        events.push_back(event(context, QStringLiteral("cat_mouse_swap"),
                               QObject::tr("Chat et souris : changement de rôles, %1 devient le chat")
                                   .arg(nameOf(context, catId())),
                               catId()));
    }
    const GameObject *cat = context.object(catId());
    const GameObject *mouse = context.object(mouseId());
    if (!cat || !mouse || !cat->state->found || !mouse->state->found) {
        m_contact = false;
        return events;
    }
    // Contact : cadres qui se touchent (avec une petite marge).
    const QRectF cb = cat->state->box, mb = mouse->state->box;
    const double margin = 0.05 * std::max(cb.width(), mb.width());
    m_contact = cb.adjusted(-margin, -margin, margin, margin).intersects(mb);
    const QRectF refuge = context.zoneRect(m_settings.value(QStringLiteral("refuge")));
    m_mouseSafe = !refuge.isEmpty() && refuge.contains(mouse->state->center);
    if (!m_contact)
        m_needSeparation = false;
    if (m_contact && !m_needSeparation && !m_mouseSafe && t - m_lastCatchMs >= kCooldownMs) {
        (m_swapped ? m_scoreB : m_scoreA)++;
        m_lastCatchMs = t;
        m_needSeparation = true;
        m_catchText = QObject::tr("ATTRAPÉE ! +1 pour %1").arg(cat->name);
        events.push_back(event(context, QStringLiteral("cat_mouse_catch"),
                               QObject::tr("Chat et souris : %1 attrape %2 (%3 à %4)")
                                   .arg(cat->name, mouse->name)
                                   .arg(m_swapped ? m_scoreB : m_scoreA)
                                   .arg(m_swapped ? m_scoreA : m_scoreB),
                               catId(), m_swapped ? m_scoreB : m_scoreA));
    }
    return events;
}

void CatMouseGame::draw(const GameDrawContext &draw, const GameContext &context) const
{
    QPainter *p = draw.painter;
    const QRectF refuge = draw.rectToDisplay(context.zoneRect(m_settings.value(QStringLiteral("refuge"))));
    if (!refuge.isEmpty()) {
        p->setBrush(QColor(60, 220, 120, 50));
        p->setPen(QPen(QColor(60, 220, 120), draw.penWidth, Qt::DashLine));
        p->drawRect(refuge);
        label(draw, refuge.topLeft(), QObject::tr("Refuge"), QColor(120, 255, 160));
    }
    for (const auto &[id, text, color] : {std::tuple{catId(), QObject::tr("CHAT"), QColor(255, 80, 80)},
                                          std::tuple{mouseId(), QObject::tr("SOURIS"), QColor(90, 170, 255)}}) {
        const GameObject *o = context.object(id);
        if (!o || !(o->state->found || o->state->predicted))
            continue;
        const QRectF r = draw.rectToDisplay(o->state->box);
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(color, draw.penWidth * 2));
        p->drawEllipse(r.center(), r.width() * 0.7, r.height() * 0.7);
        label(draw, QPointF(r.left(), r.bottom() + draw.font.pixelSize() + draw.penWidth * 2), text, color);
    }
}

QString CatMouseGame::bannerText(const GameContext &context) const
{
    const qint64 t = context.timeMs;
    if (m_finished) {
        if (m_scoreA == m_scoreB)
            return QObject::tr("TERMINÉ ! Égalité %1 à %2").arg(m_scoreA).arg(m_scoreB);
        const bool aWins = m_scoreA > m_scoreB;
        return QObject::tr("TERMINÉ ! %1 gagne %2 à %3")
            .arg(nameOf(context, aWins ? m_a : m_b))
            .arg(std::max(m_scoreA, m_scoreB))
            .arg(std::min(m_scoreA, m_scoreB));
    }
    if (t - m_lastCatchMs < kFlashMs)
        return m_catchText;
    if (t - m_lastSwapMs < kFlashMs && t - m_startMs > kFlashMs)
        return QObject::tr("CHANGEMENT DE RÔLES ! %1 est le chat").arg(nameOf(context, catId()));
    QString text = QObject::tr("Chat : %1 — %2 — %3 à %4")
                       .arg(nameOf(context, catId()))
                       .arg(clock(remaining(t)))
                       .arg(m_scoreA)
                       .arg(m_scoreB);
    if (m_mouseSafe)
        text += QObject::tr(" — souris au refuge");
    return text;
}

QColor CatMouseGame::bannerColor(const GameContext &context) const
{
    const qint64 t = context.timeMs;
    if (m_finished)
        return blue();
    if (t - m_lastCatchMs < kFlashMs)
        return red();
    if (t - m_lastSwapMs < kFlashMs && t - m_startMs > kFlashMs)
        return purple();
    return m_mouseSafe ? green() : grey();
}

QString CatMouseGame::countersText(const GameContext &context) const
{
    return QObject::tr("%1 : %2 — %3 : %4\nTemps restant : %5 — changement de rôles dans %6 s")
        .arg(nameOf(context, m_a))
        .arg(m_scoreA)
        .arg(nameOf(context, m_b))
        .arg(m_scoreB)
        .arg(clock(remaining(context.timeMs)))
        .arg(int(std::ceil(nextSwap(context.timeMs))));
}

QJsonObject CatMouseGame::toJson(const GameContext &context) const
{
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("cat_id"), catId());
    o.insert(QStringLiteral("mouse_id"), mouseId());
    o.insert(QStringLiteral("scores"), QJsonArray{QJsonObject{{QStringLiteral("id"), m_a}, {QStringLiteral("points"), m_scoreA}},
                                                  QJsonObject{{QStringLiteral("id"), m_b}, {QStringLiteral("points"), m_scoreB}}});
    o.insert(QStringLiteral("contact"), m_contact);
    o.insert(QStringLiteral("mouse_safe"), m_mouseSafe);
    o.insert(QStringLiteral("remaining_s"), std::round(remaining(context.timeMs) * 10) / 10);
    o.insert(QStringLiteral("next_swap_s"), std::round(nextSwap(context.timeMs) * 10) / 10);
    o.insert(QStringLiteral("finished"), m_finished);
    return o;
}
