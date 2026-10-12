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
                                        const std::vector<DetectedMarker> *markers)
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

    return QJsonDocument(msg).toJson(QJsonDocument::Compact);
}

bool PositionSender::send(const QSize &imageSize, const std::vector<DetectedSquare> *squares,
                          const std::vector<DetectedMarker> *markers)
{
    if (m_address.isNull())
        return false;

    const QByteArray data =
        buildMessage(m_frame++, QDateTime::currentMSecsSinceEpoch(), imageSize, squares, markers);
    if (m_socket.writeDatagram(data, m_address, m_port) != data.size()) {
        m_lastError = m_socket.errorString();
        return false;
    }
    m_lastError.clear();
    m_lastSize = data.size();
    ++m_sentCount;
    return true;
}
