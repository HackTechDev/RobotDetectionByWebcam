#pragma once

#include "objecttracker.h"

#include <QColor>
#include <QMap>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <vector>

class QSettings;

// Zone rectangulaire fixe, tracée sur l'image et nommée.
struct Zone
{
    int id = 0;
    QString name;
    QColor color;
    QRectF rect;             // en fraction de l'image (0 à 1) : valable pour toute résolution
    QStringList occupants;   // noms de ce qui se trouve dans la zone, à la dernière image
};

// Ce qui peut entrer dans une zone : objet suivi, marqueur ou carré.
struct ZoneOccupant
{
    QString key;     // identifiant stable (« tracked:3 », « marker:7 », « square »)
    QString name;    // nom affiché (« Robot bleu », « ArUco 7 », « Carré »)
    int objectId = 0;
    QPointF imagePos;
    bool hasFloor = false;
    QPointF floorPos;
};

// Zones fixes de l'image, avec l'occupation de chacune et les entrées et sorties.
class ZoneManager
{
public:
    // Ajoute une zone (rectangle en pixels de l'image « imageSize ») ; retourne son numéro.
    int add(const QString &name, const QRectF &imageRect, const QSize &imageSize);
    void remove(int id);
    void clear();
    void rename(int id, const QString &name);
    const std::vector<Zone> &zones() const { return m_zones; }
    QString nextName() const;
    QColor colorOf(int id) const;

    // Rectangle d'une zone en pixels, pour une image de cette taille.
    static QRectF imageRect(const Zone &zone, const QSize &imageSize);

    // Zones d'une caméra, dans le groupe « group » des réglages.
    void load(QSettings &settings, const QString &group);
    void save(QSettings &settings, const QString &group) const;

    // Met à jour l'occupation des zones et retourne les entrées et sorties. Une entrée est
    // signalée dès que le point est dans la zone ; une sortie, quand il en est resté dehors (ou
    // n'a plus été vu) pendant 0,3 s, avec une marge de 1 % de l'image : un point qui tremble sur
    // le bord ne déclenche pas une suite d'entrées et de sorties.
    std::vector<MotionEvent> update(const std::vector<ZoneOccupant> &occupants, const QSize &imageSize,
                                    qint64 timeMs);

private:
    struct Presence
    {
        bool inside = false;
        qint64 lastInsideMs = 0;
        ZoneOccupant last;  // dernière position connue dans la zone
    };
    std::vector<Zone> m_zones;
    QMap<int, QMap<QString, Presence>> m_presence;  // zone -> occupant -> présence
    int m_nextId = 1;
};
