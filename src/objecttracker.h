#pragma once

#include "motionfilter.h"

#include <QImage>
#include <QPointF>
#include <QRectF>
#include <QStringList>
#include <deque>
#include <opencv2/core.hpp>

class FloorMapper;

namespace cv {
class Tracker;
}

// État du suivi d'un objet, en coordonnées de l'image réelle de la caméra.
struct TrackedObject
{
    bool active = false;  // un objet a été sélectionné
    bool found = false;   // l'objet est trouvé dans l'image courante
    bool predicted = false;  // perdu depuis peu : position prédite à vitesse constante
    QRectF box;           // cadre du tracker (dernière position connue si l'objet est perdu)
    QPointF center;       // position lissée (ou prédite)

    // Mouvement, estimé par le filtre de Kalman.
    QPointF velocity;       // en pixels par seconde
    double speed = 0.0;     // en pixels par seconde
    double direction = 0.0; // direction du déplacement en degrés ]-180, 180] (sens horaire)
    bool moving = false;

    // Sur le sol, si le repère du sol est défini.
    bool hasFloor = false;
    QPointF floorPosition;      // en mm
    QPointF floorVelocity;      // en mm par seconde
    double floorSpeed = 0.0;    // en mm par seconde
    double floorDirection = 0;  // en degrés, de l'axe X vers l'axe Y
};

// Suivi d'un objet encadré à la main (trackers CSRT ou KCF d'OpenCV), avec sa trajectoire.
class ObjectTracker
{
public:
    enum Algorithm { Csrt, Kcf };

    struct TrailPoint
    {
        qint64 timeMs;
        QPointF position;  // centre dans l'image
    };

    ObjectTracker();
    ~ObjectTracker();

    static QStringList algorithmNames();
    void setAlgorithm(Algorithm algorithm) { m_algorithm = algorithm; }
    Algorithm algorithm() const { return m_algorithm; }

    // Durée de trajectoire conservée.
    void setTrailDuration(double seconds) { m_trailMs = qint64(seconds * 1000); }
    // Force du lissage (0 à 100). Si « smoothPosition » est faux, la position est celle du tracker
    // (brute) ; la vitesse reste estimée par le filtre, sinon elle serait trop bruitée.
    void setSmoothing(int strength, bool smoothPosition = true)
    {
        m_smoothing = strength;
        m_smoothPosition = smoothPosition;
    }
    // Durée pendant laquelle la position reste prédite après une perte (0 = pas de prédiction).
    void setHoldTime(double seconds) { m_holdMs = qint64(seconds * 1000); }

    // Commence le suivi de l'objet encadré par « box » dans « image ». Retourne false si le
    // cadre est trop petit ou si le tracker n'a pas pu démarrer.
    bool start(const QImage &image, const QRectF &box, qint64 timeMs);
    void stop();
    bool isActive() const { return m_state.active; }

    // Suit l'objet dans une nouvelle image, puis calcule son mouvement. La position sur le sol
    // est calculée si « floor » est prêt (FloorMapper::prepare() appelé pour cette image).
    const TrackedObject &update(const QImage &image, qint64 timeMs, const FloorMapper *floor);

    const TrackedObject &state() const { return m_state; }
    const std::deque<TrailPoint> &trail() const { return m_trail; }

private:
    cv::Mat toWorkingImage(const QImage &image, double &scale) const;
    void computeMotion(const FloorMapper *floor);

    cv::Ptr<cv::Tracker> m_tracker;
    Algorithm m_algorithm = Csrt;
    qint64 m_trailMs = 10000;
    int m_smoothing = 50;
    bool m_smoothPosition = true;
    qint64 m_holdMs = 500;
    KalmanFilter2D m_filter;
    qint64 m_lastUpdateMs = 0;
    qint64 m_lastSeenMs = 0;
    TrackedObject m_state;
    std::deque<TrailPoint> m_trail;
};
