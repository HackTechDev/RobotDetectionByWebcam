#pragma once

#include "markerdetector.h"
#include "squaredetector.h"

#include <QPointF>
#include <deque>
#include <map>
#include <vector>

class FloorMapper;

// Filtre de Kalman à vitesse constante sur une position 2D, avec un pas de temps variable.
// Les deux axes sont indépendants : état (position, vitesse) par axe.
class KalmanFilter2D
{
public:
    // processNoise : densité spectrale de l'accélération (px²/s³). Plus elle est faible, plus
    // le lissage est fort (et le retard grand). measurementNoise : variance de la mesure (px²).
    void setNoise(double processNoise, double measurementNoise);
    // Réglage du filtre pour une force de lissage de 0 (aucun) à 100 (très fort).
    void setStrength(int strength);

    void reset(const QPointF &position);
    void predict(double dt);
    void update(const QPointF &measurement);

    QPointF position() const { return {m_axes[0].p, m_axes[1].p}; }
    QPointF velocity() const { return {m_axes[0].v, m_axes[1].v}; }

private:
    struct Axis
    {
        double p = 0, v = 0;
        double P00 = 0, P01 = 0, P11 = 0;  // covariance (symétrique)
    };

    Axis m_axes[2];
    double m_q = 3000.0;
    double m_r = 2.0;
};

// Position lissée d'un carré ou d'un marqueur, suivi d'une image à l'autre.
struct SmoothedTrack
{
    enum Kind { Square, Marker };
    Kind kind = Marker;
    int id = -1;             // identifiant du marqueur (-1 pour le carré)
    bool predicted = false;  // non détecté dans cette image : position prédite
    QPointF position;        // position lissée, en pixels
    QPointF velocity;        // en pixels par seconde
    double speed = 0.0;      // en pixels par seconde
    double angle = 0.0;      // angle lissé dans l'image (en degrés, même convention que la détection)

    bool hasFloor = false;
    QPointF floorPosition;      // en mm
    QPointF floorVelocity;      // en mm par seconde
    double floorSpeed = 0.0;    // en mm par seconde
    double floorDirection = 0;  // direction du déplacement sur le sol, en degrés
    double floorHeading = 0;    // cap (marqueur) ou angle (carré) lissé sur le sol, en degrés

    std::deque<QPointF> trail;  // positions lissées récentes, en pixels
};

// Lissage dans le temps des carrés et des marqueurs détectés : un filtre de Kalman par
// marqueur (identifié par son ID) et un pour le carré principal (le plus grand).
class MotionSmoother
{
public:
    // Force du lissage de 0 (aucun) à 100 (très fort).
    void setStrength(int strength);
    int strength() const { return m_strength; }
    // Durée pendant laquelle un objet non détecté reste prédit avant d'être oublié.
    void setHoldTime(double seconds) { m_holdMs = qint64(seconds * 1000); }
    void setTrailDuration(double seconds) { m_trailMs = qint64(seconds * 1000); }
    void clear();

    // À appeler à chaque image, après la détection (et FloorMapper::prepare()).
    void update(qint64 timeMs, const std::vector<DetectedSquare> &squares,
                const std::vector<DetectedMarker> &markers, const FloorMapper *floor);

    // Objets suivis : le carré principal d'abord, puis les marqueurs par identifiant.
    std::vector<const SmoothedTrack *> tracks() const;

private:
    struct Track
    {
        SmoothedTrack out;
        KalmanFilter2D filter;
        qint64 lastUpdateMs = 0;   // dernière image traitée
        qint64 lastSeenMs = 0;     // dernière détection
        std::deque<qint64> trailTimes;
        bool angleInit = false;
        double angleX = 1, angleY = 0;  // angle lissé, sous forme de vecteur unitaire
    };

    void step(Track &track, qint64 timeMs, const QPointF *measurement, double angle, double angleSymmetry,
              double resetDistance);
    void finish(Track &track, qint64 timeMs, const FloorMapper *floor, double angleSymmetry);

    int m_strength = 50;
    qint64 m_holdMs = 500;
    qint64 m_trailMs = 10000;
    bool m_hasSquare = false;
    Track m_square;
    std::map<int, Track> m_markers;
};
