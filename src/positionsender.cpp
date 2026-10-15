#include "positionsender.h"

#include <QDateTime>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>

namespace {

// Arrondi pour garder des messages courts et lisibles (0.1 px, 0.1 mm, 0.1°).
double round1(double v)
{
    return std::round(v * 10.0) / 10.0;
}

QJsonArray point(const QPointF &p)
{
    return {round1(p.x()), round1(p.y())};
}

QJsonArray polygon(const QPolygonF &poly)
{
    QJsonArray a;
    for (const QPointF &p : poly)
        a.append(point(p));
    return a;
}

QJsonArray vector3(const QVector3D &v)
{
    return {round1(v.x()), round1(v.y()), round1(v.z())};
}

} // namespace

bool PositionSender::setTarget(const QString &host, quint16 port)
{
    m_host = host.trimmed();
    m_port = port;
    m_address = QHostAddress(m_host);
    if (m_address.isNull() && !m_host.isEmpty()) {
        // Nom d'hôte : résolution (bloquante, faite seulement quand l'adresse change).
        const QHostInfo info = QHostInfo::fromName(m_host);
        for (const QHostAddress &a : info.addresses()) {
            if (a.protocol() == QAbstractSocket::IPv4Protocol) {
                m_address = a;
                break;
            }
        }
        if (m_address.isNull() && !info.addresses().isEmpty())
            m_address = info.addresses().constFirst();
    }
    m_lastError = m_address.isNull() ? QObject::tr("Adresse invalide : %1").arg(m_host) : QString();
    return !m_address.isNull();
}

QString PositionSender::targetDescription() const
{
    return QStringLiteral("%1:%2").arg(m_address.toString()).arg(m_port);
}

QByteArray PositionSender::buildMessage(qint64 frame, qint64 timestampMs, const QSize &imageSize,
                                        const std::vector<DetectedSquare> *squares,
                                        const std::vector<DetectedMarker> *markers,
                                        const std::vector<TrackedEntry> *tracked,
                                        const std::vector<const SmoothedTrack *> *tracks,
                                        const std::vector<MotionEvent> *events,
                                        const std::vector<ZoneEntry> *zones, const GameEntry *game)
{
    QJsonObject msg;
    msg.insert(QStringLiteral("frame"), frame);
    msg.insert(QStringLiteral("timestamp_ms"), timestampMs);
    msg.insert(QStringLiteral("image"), QJsonObject{{QStringLiteral("width"), imageSize.width()},
                                                    {QStringLiteral("height"), imageSize.height()}});

    if (squares) {
        QJsonArray list;
        for (const DetectedSquare &s : *squares) {
            QJsonObject o{{QStringLiteral("center"), point(s.center)},
                          {QStringLiteral("side"), round1(s.side)},
                          {QStringLiteral("angle"), round1(s.angle)},
                          {QStringLiteral("corners"), polygon(s.corners)}};
            if (s.hasFloor) {
                o.insert(QStringLiteral("floor"),
                         QJsonObject{{QStringLiteral("position_mm"), point(s.floorPosition)},
                                     {QStringLiteral("angle_deg"), round1(s.floorHeading)}});
            }
            list.append(o);
        }
        msg.insert(QStringLiteral("squares"), list);
    }

    if (markers) {
        QJsonArray list;
        for (const DetectedMarker &m : *markers) {
            QJsonObject o{{QStringLiteral("id"), m.id},
                          {QStringLiteral("center"), point(m.center)},
                          {QStringLiteral("side"), round1(m.side)},
                          {QStringLiteral("angle"), round1(m.angle)},
                          {QStringLiteral("corners"), polygon(m.corners)}};
            if (m.hasFloor) {
                o.insert(QStringLiteral("floor"),
                         QJsonObject{{QStringLiteral("position_mm"), point(m.floorPosition)},
                                     {QStringLiteral("heading_deg"), round1(m.floorHeading)}});
            }
            if (m.hasPose) {
                o.insert(QStringLiteral("pose"),
                         QJsonObject{{QStringLiteral("position_mm"), vector3(m.position)},
                                     {QStringLiteral("rotation_deg"), vector3(m.rotation)},
                                     {QStringLiteral("distance_mm"), round1(m.distance)}});
            }
            list.append(o);
        }
        msg.insert(QStringLiteral("markers"), list);
    }

    if (tracks) {
        QJsonArray list;
        for (const SmoothedTrack *t : *tracks) {
            QJsonObject o{{QStringLiteral("type"), t->kind == SmoothedTrack::Square ? QStringLiteral("square")
                                                                                     : QStringLiteral("marker")},
                          {QStringLiteral("predicted"), t->predicted},
                          {QStringLiteral("position_px"), point(t->position)},
                          {QStringLiteral("velocity_px_s"), point(t->velocity)},
                          {QStringLiteral("speed_px_s"), round1(t->speed)},
                          {QStringLiteral("angle_deg"), round1(t->angle)}};
            if (t->kind == SmoothedTrack::Marker)
                o.insert(QStringLiteral("id"), t->id);
            if (t->hasFloor) {
                // Même nom que dans « squares » et « markers » : angle (carré) ou cap (marqueur).
                const QString headingKey = t->kind == SmoothedTrack::Square ? QStringLiteral("angle_deg")
                                                                            : QStringLiteral("heading_deg");
                o.insert(QStringLiteral("floor"),
                         QJsonObject{{QStringLiteral("position_mm"), point(t->floorPosition)},
                                     {QStringLiteral("velocity_mm_s"), point(t->floorVelocity)},
                                     {QStringLiteral("speed_mm_s"), round1(t->floorSpeed)},
                                     {QStringLiteral("direction_deg"), round1(t->floorDirection)},
                                     {headingKey, round1(t->floorHeading)}});
            }
            list.append(o);
        }
        msg.insert(QStringLiteral("tracks"), list);
    }

    // Objets suivis : une entrée par objet, avec son numéro et son nom.
    if (tracked && !tracked->empty()) {
        QJsonArray list;
        for (const TrackedEntry &entry : *tracked) {
            const TrackedObject *t = entry.state;
            QJsonObject o{{QStringLiteral("id"), entry.id},
                          {QStringLiteral("name"), entry.name},
                          {QStringLiteral("found"), t->found},
                          {QStringLiteral("predicted"), t->predicted},
                          {QStringLiteral("confidence"), std::round(t->confidence * 100) / 100},
                          {QStringLiteral("drifted"), t->drifted},
                          {QStringLiteral("out_of_view"), t->outOfView},
                          {QStringLiteral("reacquired"), t->reacquired},
                          {QStringLiteral("reacquisitions"), t->reacquisitions},
                          {QStringLiteral("center"), point(t->center)},
                          {QStringLiteral("box"), QJsonArray{round1(t->box.x()), round1(t->box.y()),
                                                             round1(t->box.width()), round1(t->box.height())}},
                          {QStringLiteral("moving"), t->moving},
                          {QStringLiteral("velocity_px_s"), point(t->velocity)},
                          {QStringLiteral("speed_px_s"), round1(t->speed)},
                          {QStringLiteral("direction_deg"), round1(t->direction)}};
            if (t->hasFloor) {
                o.insert(QStringLiteral("floor"),
                         QJsonObject{{QStringLiteral("position_mm"), point(t->floorPosition)},
                                     {QStringLiteral("velocity_mm_s"), point(t->floorVelocity)},
                                     {QStringLiteral("speed_mm_s"), round1(t->floorSpeed)},
                                     {QStringLiteral("direction_deg"), round1(t->floorDirection)}});
            }
            list.append(o);
        }
        msg.insert(QStringLiteral("tracked"), list);
    }

    // Événements de l'objet suivi survenus à cette image (chacun n'est envoyé qu'une fois).
    if (events && !events->empty()) {
        static const char *types[] = {"motion_start", "motion_stop", "lost", "found", "zone_enter", "zone_exit", "game_zone_enter", "game_zone_exit"};
        QJsonArray list;
        for (const MotionEvent &e : *events) {
            QJsonObject o{{QStringLiteral("type"), QString::fromLatin1(types[e.type])},
                          {QStringLiteral("object_id"), e.objectId},
                          {QStringLiteral("name"), e.objectName},
                          {QStringLiteral("timestamp_ms"), e.wallTimeMs},
                          {e.onFloor ? QStringLiteral("position_mm") : QStringLiteral("position_px"), point(e.position)}};
            if (e.type == MotionEvent::ZoneEnter || e.type == MotionEvent::ZoneExit || e.type == MotionEvent::GameEnter
                || e.type == MotionEvent::GameExit) {
                o.insert(QStringLiteral("zone_id"), e.zoneId);
                o.insert(QStringLiteral("zone"), e.zoneName);
            }
            if (e.type == MotionEvent::MotionStop) {
                const QString suffix = e.onFloor ? QStringLiteral("_mm") : QStringLiteral("_px");
                o.insert(QStringLiteral("distance") + suffix, round1(e.distance));
                o.insert(QStringLiteral("travelled") + suffix, round1(e.travelled));
                o.insert(QStringLiteral("duration_s"), std::round(e.duration * 100) / 100);
            }
            list.append(o);
        }
        msg.insert(QStringLiteral("events"), list);
    }

    // Zones : rectangle, coins sur le sol et occupants de chacune.
    if (zones && !zones->empty()) {
        QJsonArray list;
        for (const ZoneEntry &entry : *zones) {
            QJsonArray occupants;
            for (const QString &name : entry.zone->occupants)
                occupants.append(name);
            QJsonObject o{{QStringLiteral("id"), entry.zone->id},
                          {QStringLiteral("name"), entry.zone->name},
                          {QStringLiteral("box"), QJsonArray{round1(entry.box.x()), round1(entry.box.y()),
                                                             round1(entry.box.width()), round1(entry.box.height())}},
                          {QStringLiteral("occupants"), occupants}};
            if (!entry.floorCorners.empty()) {
                QJsonArray corners;
                for (const QPointF &p : entry.floorCorners)
                    corners.append(point(p));
                o.insert(QStringLiteral("floor_corners_mm"), corners);
            }
            list.append(o);
        }
        msg.insert(QStringLiteral("zones"), list);
    }

    // Jeu « Détection robot dans zone » en cours.
    if (game) {
        QJsonObject g{{QStringLiteral("mode"), QStringLiteral("robot_in_zone")},
                      {QStringLiteral("robot_id"), game->robotId},
                      {QStringLiteral("robot"), game->robot},
                      {QStringLiteral("target_zone_id"), game->targetZoneId},
                      {QStringLiteral("found"), game->found},
                      {QStringLiteral("inside"), game->inside},
                      {QStringLiteral("time_inside_s"), std::round(game->timeInside * 10) / 10},
                      {QStringLiteral("entries"), game->entries}};
        if (game->inside) {
            g.insert(QStringLiteral("zone_id"), game->zoneId);
            g.insert(QStringLiteral("zone"), game->zone);
        }
        msg.insert(QStringLiteral("game"), g);
    }

    return QJsonDocument(msg).toJson(QJsonDocument::Compact);
}

bool PositionSender::send(const QSize &imageSize, const std::vector<DetectedSquare> *squares,
                          const std::vector<DetectedMarker> *markers, const std::vector<TrackedEntry> *tracked,
                          const std::vector<const SmoothedTrack *> *tracks,
                          const std::vector<MotionEvent> *events, const std::vector<ZoneEntry> *zones,
                          const GameEntry *game)
{
    if (m_address.isNull())
        return false;

    const QByteArray data =
        buildMessage(m_frame++, QDateTime::currentMSecsSinceEpoch(), imageSize, squares, markers, tracked, tracks, events, zones, game);
    if (m_socket.writeDatagram(data, m_address, m_port) != data.size()) {
        m_lastError = m_socket.errorString();
        return false;
    }
    m_lastError.clear();
    m_lastSize = data.size();
    ++m_sentCount;
    return true;
}
