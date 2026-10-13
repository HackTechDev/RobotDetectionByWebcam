#pragma once

#include "markerdetector.h"
#include "motionfilter.h"
#include "objecttracker.h"
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

    // Un pointeur nul signifie que la détection correspondante est désactivée (ou qu'aucun
    // objet n'est suivi) : la clé est alors absente du message.
    bool send(const QSize &imageSize, const std::vector<DetectedSquare> *squares,
              const std::vector<DetectedMarker> *markers, const TrackedObject *tracked = nullptr,
              const std::vector<const SmoothedTrack *> *tracks = nullptr,
              const std::vector<MotionEvent> *events = nullptr);

    qint64 sentCount() const { return m_sentCount; }
    qint64 lastSize() const { return m_lastSize; }
    QString lastError() const { return m_lastError; }

    // Construit le message JSON (compact, sur une ligne).
    static QByteArray buildMessage(qint64 frame, qint64 timestampMs, const QSize &imageSize,
                                   const std::vector<DetectedSquare> *squares,
                                   const std::vector<DetectedMarker> *markers,
                                   const TrackedObject *tracked = nullptr,
                                   const std::vector<const SmoothedTrack *> *tracks = nullptr,
                                   const std::vector<MotionEvent> *events = nullptr);

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
