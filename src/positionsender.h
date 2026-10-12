#pragma once

#include "markerdetector.h"
#include "squaredetector.h"

#include <QHostAddress>
#include <QSize>
#include <QString>
#include <QUdpSocket>
#include <vector>

// Envoie les résultats de détection en JSON, un datagramme UDP par image.
class PositionSender
{
public:
    // Destination : adresse IP ou nom d'hôte. Retourne false si l'adresse est invalide.
    bool setTarget(const QString &host, quint16 port);
    bool hasTarget() const { return !m_address.isNull(); }
    QString targetDescription() const;

    // Une liste nulle (pointeur nul) signifie que la détection correspondante est désactivée :
    // la clé est alors absente du message.
    bool send(const QSize &imageSize, const std::vector<DetectedSquare> *squares,
              const std::vector<DetectedMarker> *markers);

    qint64 sentCount() const { return m_sentCount; }
    qint64 lastSize() const { return m_lastSize; }
    QString lastError() const { return m_lastError; }

    // Construit le message JSON (compact, sur une ligne).
    static QByteArray buildMessage(qint64 frame, qint64 timestampMs, const QSize &imageSize,
                                   const std::vector<DetectedSquare> *squares,
                                   const std::vector<DetectedMarker> *markers);

private:
    QUdpSocket m_socket;
    QHostAddress m_address;
    QString m_host;
    quint16 m_port = 0;
    qint64 m_frame = 0;
    qint64 m_sentCount = 0;
    qint64 m_lastSize = 0;
    QString m_lastError;
};
