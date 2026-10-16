#include "drawinggame.h"

#include <QPainter>

#include <algorithm>
#include <cmath>

using namespace GameUtil;

namespace {

constexpr qint64 kStillMs = 2000;        // immobile 2 s après avoir tracé : dessin terminé
constexpr double kMinLength = 0.3;       // longueur minimale du tracé (part du périmètre du modèle)
constexpr double kTolerance = 0.10;      // écart qui donne 0 % (part de la diagonale du cadre)
constexpr int kSamples = 120;

double length(const QPolygonF &line)
{
    double l = 0;
    for (int i = 1; i < line.size(); ++i)
        l += distance(line[i - 1], line[i]);
    return l;
}

// Écart de « from » à « to » : moyenne des distances au point le plus proche, et 90e centile, qui
// pénalise les parties manquées (les coins d'un carré dessiné en cercle, par exemple).
double nearestDistance(const QPolygonF &from, const QPolygonF &to)
{
    if (from.isEmpty() || to.isEmpty())
        return 1e18;
    std::vector<double> d;
    for (const QPointF &p : from) {
        double best = 1e18;
        for (const QPointF &q : to)
            best = std::min(best, distance(p, q));
        d.push_back(best);
    }
    double mean = 0;
    for (double v : d)
        mean += v;
    mean /= d.size();
    std::sort(d.begin(), d.end());
    const double p90 = d[std::min(d.size() - 1, size_t(0.9 * d.size()))];
    return 0.5 * mean + 0.5 * p90;
}

} // namespace

QString DrawingGame::rules() const
{
    return QObject::tr("Reproduisez la forme en pointillés avec la trajectoire du robot. Le dessin commence au premier "
                       "mouvement et se termine quand le robot reste immobile 2 s (ou avec « Terminer le dessin »). "
                       "La ressemblance dépend de l'écart entre le tracé et le modèle, et de la part de la forme "
                       "couverte.");
}

std::vector<GameParam> DrawingGame::params() const
{
    return {
        {QStringLiteral("robot"), QObject::tr("Robot"), GameParam::Tracked, {}, 0, {},
         QObject::tr("Objet suivi qui joue le rôle du crayon (onglet Suivi)")},
        {QStringLiteral("frame"), QObject::tr("Cadre du dessin"), GameParam::Zone, {}, 0, {},
         QObject::tr("Zone dans laquelle la forme est dessinée (onglet Zones)")},
        {QStringLiteral("shape"), QObject::tr("Forme"), GameParam::Choice,
         {QObject::tr("Carré"), QObject::tr("Cercle"), QObject::tr("Triangle"), QObject::tr("Zigzag")}, 0, {}, {}},
    };
}

QPolygonF DrawingGame::resample(const QPolygonF &line, int n)
{
    QPolygonF out;
    const double total = length(line);
    if (line.size() < 2 || total <= 0)
        return line;
    const double step = total / (n - 1);
    out << line.first();
    double carried = 0;
    for (int i = 1; i < line.size(); ++i) {
        const QPointF a = line[i - 1], b = line[i];
        const double seg = distance(a, b);
        double pos = step - carried;
        while (pos <= seg && out.size() < n) {
            out << a + (b - a) * (pos / seg);
            pos += step;
        }
        carried = seg - (pos - step);
    }
    while (out.size() < n)
        out << line.last();
    return out;
}

double DrawingGame::similarity(const QPolygonF &trace, const QPolygonF &model, double diagonal)
{
    if (trace.size() < 2 || model.size() < 2 || diagonal <= 0)
        return 0.0;
    const QPolygonF t = resample(trace, kSamples), m = resample(model, kSamples);
    // Précision (le tracé suit le modèle) et couverture (toute la forme est parcourue).
    const double d = (nearestDistance(t, m) + nearestDistance(m, t)) / 2;
    return std::clamp(100.0 * (1.0 - d / (kTolerance * diagonal)), 0.0, 100.0);
}

QPolygonF DrawingGame::model(const GameContext &context) const
{
    const QRectF zone = context.zoneRect(m_settings.value(QStringLiteral("frame")));
    // Forme carrée centrée dans le cadre, avec une marge de 15 %.
    const double side = std::min(zone.width(), zone.height()) * 0.7;
    const QRectF r(zone.center().x() - side / 2, zone.center().y() - side / 2, side, side);
    QPolygonF p;
    switch (m_shape) {
    case 1:  // cercle
        for (int i = 0; i <= 72; ++i) {
            const double a = 2 * M_PI * i / 72;
            p << r.center() + QPointF(std::cos(a), std::sin(a)) * side / 2;
        }
        break;
    case 2:  // triangle
        p << QPointF(r.center().x(), r.top()) << r.bottomRight() << r.bottomLeft() << QPointF(r.center().x(), r.top());
        break;
    case 3:  // zigzag
        p << r.topLeft() << QPointF(r.left() + side / 3, r.bottom()) << QPointF(r.left() + 2 * side / 3, r.top())
          << r.bottomRight();
        break;
    default:  // carré
        p << r.topLeft() << r.topRight() << r.bottomRight() << r.bottomLeft() << r.topLeft();
        break;
    }
    return p;
}

void DrawingGame::onStart(const GameContext &)
{
    m_shape = m_settings.value(QStringLiteral("shape"));
    m_phase = Waiting;
    m_trace.clear();
    m_hasAnchor = false;
    m_stillSinceMs = -1;
    m_score = 0;
    m_newBest = false;
    m_finishRequested = false;
}

void DrawingGame::action(const GameContext &)
{
    m_finishRequested = true;  // jugé à la prochaine image
}

std::vector<MotionEvent> DrawingGame::evaluate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const QRectF zone = context.zoneRect(m_settings.value(QStringLiteral("frame")));
    m_score = similarity(m_trace, model(context), std::hypot(zone.width(), zone.height()));
    m_newBest = m_score > m_best.value(m_shape, -1);
    if (m_newBest) {
        m_best.insert(m_shape, m_score);
        m_bestTrace.insert(m_shape, m_trace);
    }
    m_phase = Done;
    events.push_back(event(context, QStringLiteral("drawing_done"),
                           QObject::tr("Dessin au sol : %1, ressemblance %2 %%3")
                               .arg(params()[2].choices.value(m_shape).toLower())
                               .arg(int(std::lround(m_score)))
                               .arg(m_newBest ? QObject::tr(", meilleur dessin") : QString()),
                           m_settings.value(QStringLiteral("robot")), int(std::lround(m_score))));
    finish();
    return events;
}

std::vector<MotionEvent> DrawingGame::onUpdate(const GameContext &context)
{
    std::vector<MotionEvent> events;
    const qint64 t = context.timeMs;
    const GameObject *robot = context.object(m_settings.value(QStringLiteral("robot")));
    if (m_finishRequested) {
        m_finishRequested = false;
        if (m_phase == Drawing && m_trace.size() >= 2)
            return evaluate(context);
    }
    if (!robot || !robot->state->found)
        return events;
    const QPointF c = robot->state->center;
    if (m_phase == Waiting) {
        if (!m_hasAnchor) {
            m_anchor = c;
            m_hasAnchor = true;
        } else if (distance(c, m_anchor) > context.motionPx) {
            m_phase = Drawing;
            m_trace << m_anchor << c;
            events.push_back(event(context, QStringLiteral("drawing_start"), QObject::tr("Dessin au sol : début du tracé"),
                                   robot->id));
        }
        return events;
    }
    if (distance(c, m_trace.last()) >= 2.0)
        m_trace << c;
    // Fin : immobile depuis 2 s, avec un tracé assez long.
    if (m_stillSinceMs < 0 || distance(c, m_stillPos) > context.motionPx) {
        m_stillPos = c;
        m_stillSinceMs = t;
    } else if (t - m_stillSinceMs >= kStillMs && length(m_trace) >= kMinLength * length(model(context))) {
        return evaluate(context);
    }
    return events;
}

void DrawingGame::draw(const GameDrawContext &draw, const GameContext &context) const
{
    QPainter *p = draw.painter;
    auto toDisplay = [&draw](const QPolygonF &poly) {
        QPolygonF out;
        for (const QPointF &pt : poly)
            out << draw.toDisplay(pt);
        return out;
    };
    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(QColor(200, 200, 200), draw.penWidth, Qt::DotLine));
    p->drawRect(draw.rectToDisplay(context.zoneRect(m_settings.value(QStringLiteral("frame")))));
    // Meilleur dessin de cette forme, en léger.
    if (m_bestTrace.contains(m_shape) && m_phase != Done) {
        p->setPen(QPen(QColor(255, 255, 255, 70), draw.penWidth * 2));
        p->drawPolyline(toDisplay(m_bestTrace.value(m_shape)));
    }
    p->setPen(QPen(QColor(255, 230, 0), draw.penWidth * 2, Qt::DashLine));
    p->drawPolyline(toDisplay(model(context)));
    if (m_trace.size() > 1) {
        p->setPen(QPen(QColor(0, 230, 255), draw.penWidth * 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p->drawPolyline(toDisplay(m_trace));
    }
}

QString DrawingGame::bannerText(const GameContext &) const
{
    const QString shape = params()[2].choices.value(m_shape);
    switch (m_phase) {
    case Done:
        return m_newBest ? QObject::tr("%1 : ressemblance %2 % — MEILLEUR DESSIN").arg(shape).arg(int(std::lround(m_score)))
                         : QObject::tr("%1 : ressemblance %2 % (meilleur %3 %)")
                               .arg(shape)
                               .arg(int(std::lround(m_score)))
                               .arg(int(std::lround(m_best.value(m_shape))));
    case Drawing:
        return QObject::tr("Dessinez : %1 — arrêtez-vous 2 s pour terminer").arg(shape.toLower());
    case Waiting:
        break;
    }
    return QObject::tr("Dessinez un %1 : partez quand vous voulez").arg(shape.toLower());
}

QColor DrawingGame::bannerColor(const GameContext &) const
{
    switch (m_phase) {
    case Done: return m_score >= 70 ? green() : m_score >= 40 ? gold() : red();
    case Drawing: return QColor(0, 140, 170);
    case Waiting: break;
    }
    return grey();
}

QString DrawingGame::countersText(const GameContext &) const
{
    QStringList bests;
    const QStringList shapes = params()[2].choices;
    for (auto it = m_best.cbegin(); it != m_best.cend(); ++it)
        bests << QObject::tr("%1 %2 %").arg(shapes.value(it.key())).arg(int(std::lround(it.value())));
    return QObject::tr("Points du tracé : %1 — Meilleurs dessins : %2")
        .arg(m_trace.size())
        .arg(bests.isEmpty() ? QObject::tr("aucun") : bests.join(QStringLiteral(", ")));
}

QJsonObject DrawingGame::toJson(const GameContext &context) const
{
    static const char *phases[] = {"waiting", "drawing", "done"};
    static const char *shapes[] = {"square", "circle", "triangle", "zigzag"};
    QJsonObject o = Game::toJson(context);
    o.insert(QStringLiteral("robot_id"), m_settings.value(QStringLiteral("robot")));
    o.insert(QStringLiteral("shape"), QString::fromLatin1(shapes[std::clamp(m_shape, 0, 3)]));
    o.insert(QStringLiteral("phase"), QString::fromLatin1(phases[m_phase]));
    o.insert(QStringLiteral("trace_points"), int(m_trace.size()));
    if (m_phase == Done)
        o.insert(QStringLiteral("similarity"), std::round(m_score));
    o.insert(QStringLiteral("best"), std::round(m_best.value(m_shape, 0)));
    return o;
}
