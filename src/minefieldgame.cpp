#include "minefieldgame.h"

#include <QJsonArray>
#include <QPainter>
#include <QRandomGenerator>

#include <cmath>

using namespace GameUtil;

namespace {

constexpr qint64 kBoomFlashMs = 2000;

} // namespace

QString MinefieldGame::rules() const
{
    return QObject::tr("Traversez du départ à l'arrivée sans toucher les mines invisibles. Une explosion reste marquée "
                       "sur l'image : revenez au départ et retentez en l'évitant. Gagnez en un minimum d'essais ; les "
                       "mines et les trajectoires sont dévoilées à la fin.");
}

std::vector<GameParam> MinefieldGame::params() const
{
    return {
        {QStringLiteral("robot"), QObject::tr("Robot"), GameParam::Tracked, {}, 0, {},
         QObject::tr("Objet suivi qui joue le rôle du robot (onglet Suivi)")},
        {QStringLiteral("start"), QObject::tr("Départ"), GameParam::Zone, {}, 0, {}, QObject::tr("Zone de départ")},
        {QStringLiteral("finish"), QObject::tr("Arrivée"), GameParam::Zone, {}, 1, {}, QObject::tr("Zone d'arrivée")},
        {QStringLiteral("terrain"), QObject::tr("Terrain"), GameParam::ZoneOptional, {}, 0, QObject::tr("Toute l'image"),
         QObject::tr("Zone où les mines sont placées")},
        {QStringLiteral("mines"), QObject::tr("Mines"), GameParam::Choice,
         {QObject::tr("5 mines"), QObject::tr("8 mines"), QObject::tr("12 mines")}, 1, {}, {}},
        {QStringLiteral("detector"), QObject::tr("Détecteur"), GameParam::Choice,
         {QObject::tr("Sans détecteur"), QObject::tr("Avec détecteur (bip à l'approche)")}, 0, {}, {}},
    };
}

QString MinefieldGame::checkReady(const GameSettings &settings, const GameContext &context) const
{
    const QString base = Game::checkReady(settings, context);
    if (!base.isEmpty())
        return base;
    if (settings.value(QStringLiteral("start")) == settings.value(QStringLiteral("finish")))
        return QObject::tr("Choisissez deux zones différentes pour le départ et l'arrivée.");
    return QString();
}

void MinefieldGame::placeMines(const GameContext &context)
{
    static const int counts[] = {5, 8, 12};
    const int count = counts[std::clamp(m_settings.value(QStringLiteral("mines")), 0, 2)];
    QRectF terrain = context.zoneRect(m_settings.value(QStringLiteral("terrain")));
    if (terrain.isEmpty())
        terrain = QRectF(QPointF(0, 0), QSizeF(context.imageSize));
    m_radius = 0.04 * std::min(context.imageSize.width(), context.imageSize.height());
    const QRectF start = context.zoneRect(m_start).adjusted(-m_radius, -m_radius, m_radius, m_radius);
    const QRectF finishZone = context.zoneRect(m_finish).adjusted(-m_radius, -m_radius, m_radius, m_radius);
    const QRectF inner = terrain.adjusted(m_radius, m_radius, -m_radius, -m_radius);
    QRandomGenerator *rng = QRandomGenerator::global();
    m_mines.clear();
    for (int tries = 0; tries < 2000 && m_mines.size() < count; ++tries) {
        const QPointF p(inner.left() + rng->generateDouble() * inner.width(),
                        inner.top() + rng->generateDouble() * inner.height());
        if (start.contains(p) || finishZone.contains(p))
            continue;
        bool apart = true;
        for (const QPointF &m : m_mines)
            apart = apart && distance(p, m) > 2.5 * m_radius;
        if (apart)
            m_mines << p;
    }
}

void MinefieldGame::onStart(const GameContext &context)
{
    m_robot = m_settings.value(QStringLiteral("robot"));
    m_start = m_settings.value(QStringLiteral("start"));
    m_finish = m_settings.value(QStringLiteral("finish"));
    m_detector = m_settings.value(QStringLiteral("detector")) == 1;
    m_phase = Waiting;
    m_explosions.clear();
    m_attempts.clear();
    m_beep = 0;
    m_boomMs = -10000;
    placeMines(context);  // nouvelles mines à chaque partie
}

std::vector<MotionEvent> MinefieldGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const GameObject *robot = context.object(m_robot);
    m_seen = robot && robot->state->found;
    if (!m_seen)
        return events;
    const QPointF c = robot->state->center;
    const bool inStart = context.zoneRect(m_start).contains(c);
    m_beep = 0;

    switch (m_phase) {
    case Waiting:
        // Le robot doit d'abord être dans le départ.
        if (inStart)
            m_phase = Ready;
        break;
    case Ready:
        // Un essai commence quand le robot quitte le départ.
        if (!inStart) {
            m_attempts << QPolygonF();
            m_phase = Crossing;
        }
        break;
    case Crossing: {
        if (inStart) {
            m_phase = Ready;  // revenu au départ sans explosion : l'essai suivant repartira de là
            break;
        }
        m_attempts.last() << c;
        double nearest = 1e18;
        int hit = -1;
        for (int i = 0; i < m_mines.size(); ++i) {
            const double d = distance(c, m_mines[i]);
            if (d < m_radius)
                hit = i;
            nearest = std::min(nearest, d);
        }
        if (hit >= 0) {
            m_phase = Boom;
            m_boomMs = context.timeMs;
            if (!m_explosions.contains(m_mines[hit]))
                m_explosions << m_mines[hit];
            events.push_back(event(context, QStringLiteral("minefield_boom"),
                                   QObject::tr("Champ de mines : BOUM ! (essai %1)").arg(m_attempts.size()), m_robot,
                                   m_attempts.size()));
            break;
        }
        if (context.zoneRect(m_finish).contains(c)) {
            m_phase = Won;
            events.push_back(event(context, QStringLiteral("minefield_win"),
                                   QObject::tr("Champ de mines : traversée réussie en %1 essai(s)").arg(m_attempts.size()),
                                   m_robot, m_attempts.size()));
            finish();
            break;
        }
        if (m_detector)
            m_beep = nearest < 1.6 * m_radius ? 2 : nearest < 2.5 * m_radius ? 1 : 0;
        break;
    }
    case Boom:
        // Retour au départ pour un nouvel essai.
        if (inStart)
            m_phase = Ready;
        break;
    case Won:
        break;
    }
    return events;
}

void MinefieldGame::draw(const GameDrawContext &draw, const GameContext &context) const
{
    QPainter *p = draw.painter;
    const QRectF start = draw.rectToDisplay(context.zoneRect(m_start));
    const QRectF end = draw.rectToDisplay(context.zoneRect(m_finish));
    p->setBrush(QColor(60, 220, 120, 45));
    p->setPen(QPen(QColor(60, 220, 120), draw.penWidth * 2));
    p->drawRect(start);
    p->setBrush(QColor(255, 220, 0, 45));
    p->setPen(QPen(QColor(255, 220, 0), draw.penWidth * 2));
    p->drawRect(end);
    label(draw, start.topLeft(), QObject::tr("Départ"), QColor(120, 255, 160));
    label(draw, end.topLeft(), QObject::tr("Arrivée"), QColor(255, 230, 80));
    const QRectF terrain = draw.rectToDisplay(context.zoneRect(m_settings.value(QStringLiteral("terrain"))));
    if (!terrain.isEmpty()) {
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(QColor(200, 200, 200), draw.penWidth, Qt::DotLine));
        p->drawRect(terrain);
    }
    // À la fin : mines et trajectoires dévoilées.
    if (m_phase == Won) {
        const QColor colors[] = {QColor(0, 230, 255), QColor(255, 120, 200), QColor(160, 255, 120), QColor(255, 180, 60)};
        for (int i = 0; i < m_attempts.size(); ++i) {
            QPolygonF line;
            for (const QPointF &pt : m_attempts[i])
                line << draw.toDisplay(pt);
            p->setPen(QPen(colors[i % 4], draw.penWidth * 2));
            p->drawPolyline(line);
        }
        for (const QPointF &m : m_mines) {
            p->setPen(QPen(Qt::black, draw.penWidth));
            p->setBrush(QColor(80, 80, 80, 200));
            p->drawEllipse(draw.toDisplay(m), m_radius, m_radius);
        }
    }
    // Explosions : marquées pendant toute la partie.
    for (const QPointF &m : m_explosions) {
        const QPointF c = draw.toDisplay(m);
        p->setBrush(QColor(255, 60, 30, 120));
        p->setPen(QPen(QColor(255, 60, 30), draw.penWidth * 2));
        p->drawEllipse(c, m_radius, m_radius);
        p->drawLine(c + QPointF(-m_radius, -m_radius) * 0.7, c + QPointF(m_radius, m_radius) * 0.7);
        p->drawLine(c + QPointF(-m_radius, m_radius) * 0.7, c + QPointF(m_radius, -m_radius) * 0.7);
    }
    p->setBrush(Qt::NoBrush);
}

QString MinefieldGame::bannerText(const GameContext &context) const
{
    switch (m_phase) {
    case Won:
        return QObject::tr("TRAVERSÉE RÉUSSIE en %1 essai(s) !").arg(m_attempts.size());
    case Boom:
        return context.timeMs - m_boomMs < kBoomFlashMs ? QObject::tr("BOUM !")
                                                        : QObject::tr("BOUM ! Retournez au départ");
    case Crossing:
        if (m_beep == 2)
            return QObject::tr("BIP BIP BIP !");
        if (m_beep == 1)
            return QObject::tr("Bip... bip...");
        return QObject::tr("Essai %1 : traversez jusqu'à l'arrivée").arg(m_attempts.size());
    case Ready:
        return QObject::tr("Prêt : essai %1, partez quand vous voulez").arg(m_attempts.size() + 1);
    case Waiting:
        break;
    }
    return m_seen ? QObject::tr("Placez le robot dans le départ") : QObject::tr("Robot non trouvé");
}

QColor MinefieldGame::bannerColor(const GameContext &) const
{
    switch (m_phase) {
    case Won: return blue();
    case Boom: return red();
    case Crossing: return m_beep == 2 ? orange() : m_beep == 1 ? gold() : green();
    case Ready: return grey();
    case Waiting: break;
    }
    return m_seen ? grey() : orange();
}

QString MinefieldGame::countersText(const GameContext &) const
{
    return QObject::tr("Essai %1 — Explosions : %2 — Mines : %3%4")
        .arg(std::max<int>(1, m_attempts.size()))
        .arg(m_explosions.size())
        .arg(m_mines.size())
        .arg(m_detector ? QObject::tr(" — détecteur actif") : QString());
}

QJsonObject MinefieldGame::toJson(const GameContext &context) const
{
    static const char *phases[] = {"waiting", "ready", "crossing", "boom", "won"};
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("robot_id"), m_robot);
    o.insert(QStringLiteral("phase"), QString::fromLatin1(phases[m_phase]));
    o.insert(QStringLiteral("mines"), int(m_mines.size()));
    o.insert(QStringLiteral("attempts"), std::max<int>(1, m_attempts.size()));
    QJsonArray explosions;
    for (const QPointF &m : m_explosions)
        explosions.append(QJsonArray{std::round(m.x()), std::round(m.y())});
    o.insert(QStringLiteral("explosions_px"), explosions);  // seules les mines déjà explosées sont dévoilées
    o.insert(QStringLiteral("beep"), m_beep);
    return o;
}
