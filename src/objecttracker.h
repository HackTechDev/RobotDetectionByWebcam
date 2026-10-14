#pragma once

#include "motionfilter.h"

#include <QImage>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QStringList>
#include <deque>
#include <vector>
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
    // Ressemblance du contenu du cadre avec l'objet sélectionné, de 0 à 1 (lissée dans le temps).
    double confidence = 0.0;
    bool drifted = false;  // perdu parce que le cadre ne ressemble plus à l'objet
    // Sorti de l'image (perdu au bord) : seul un retour par un bord de l'image peut le retrouver.
    bool outOfView = false;
    bool reacquired = false;  // retrouvé à cette image par la recherche dans toute l'image
    int reacquisitions = 0;   // nombre de reprises automatiques depuis la sélection
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

// Événement de mouvement de l'objet suivi, ou entrée et sortie d'une zone.
struct MotionEvent
{
    enum Type { MotionStart, MotionStop, Lost, Found, ZoneEnter, ZoneExit };
    Type type = MotionStart;
    qint64 timeMs = 0;      // horloge du suivi
    qint64 wallTimeMs = 0;  // heure réelle (ms depuis 1970), renseignée par l'application
    bool onFloor = false;   // positions et distances en mm sur le sol (sinon en pixels)
    QPointF position;       // position de l'objet au moment de l'événement
    double distance = 0;    // fin de mouvement : distance entre le départ et l'arrivée
    double travelled = 0;   // fin de mouvement : longueur du chemin parcouru
    double duration = 0;    // fin de mouvement : durée en secondes
    int objectId = 0;       // objet concerné (renseigné par l'application)
    QString objectName;
    int zoneId = 0;         // entrée et sortie de zone : zone concernée
    QString zoneName;
};

// Suivi d'un objet encadré à la main (trackers CSRT, KCF, NanoTrack ou VitTrack d'OpenCV), avec sa
// trajectoire.
class ObjectTracker
{
public:
    // Nano et Vit : réseaux de neurones, plus robustes ; leurs modèles se téléchargent à part
    // (TrackerModels).
    enum Algorithm { Csrt, Kcf, Nano, Vit };

    struct TrailPoint
    {
        qint64 timeMs;
        QPointF position;         // centre dans l'image
        bool newSegment = false;  // premier point après une reprise : pas de trait depuis le précédent
    };

    ObjectTracker();
    ~ObjectTracker();

    static QStringList algorithmNames();
    // Vrai si l'algorithme est utilisable (modèles présents pour Nano et Vit).
    static bool isAvailable(Algorithm algorithm);

    // Délimite automatiquement l'objet situé sous « point » (sélection d'un clic). Si la
    // délimitation échoue, retourne un carré autour du point et met « uncertain » à vrai.
    static QRect delineateAt(const QImage &image, const QPoint &point, bool *uncertain = nullptr);
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
    // Seuils de mise en mouvement : distance à la position de repos, en mm sur le sol (si le
    // repère est défini) ou en pixels. Retour au repos quand l'objet reste dans un rayon de la
    // moitié du seuil pendant 0,5 s.
    void setMotionThresholds(double floorMm, double imagePx)
    {
        m_motionMm = floorMm;
        m_motionPx = imagePx;
    }

    // Événements survenus depuis le dernier appel.
    std::vector<MotionEvent> takeEvents();

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
    // Ressemblance (0 à 1) du contenu de « roi » avec l'apparence mémorisée de l'objet (mise à
    // jour lentement), ou avec son apparence d'origine si « initial » est vrai.
    double appearanceScore(const cv::Mat &work, const cv::Rect &roi, bool initial = false) const;
    static cv::Mat colorHistogram(const cv::Mat &bgrPatch);
    // Cherche l'objet dans l'image de travail (seulement le long des bords si « bordersOnly ») ;
    // retourne un cadre vide s'il n'est pas trouvé.
    cv::Rect searchObject(const cv::Mat &work, bool bordersOnly) const;
    bool createTracker(const cv::Mat &work, const cv::Rect &roi);
    void computeMotion(const FloorMapper *floor);
    void updateMotionState(qint64 timeMs);
    void addEvent(MotionEvent::Type type, qint64 timeMs);

    cv::Ptr<cv::Tracker> m_tracker;
    Algorithm m_algorithm = Csrt;
    qint64 m_trailMs = 10000;
    int m_smoothing = 50;
    bool m_smoothPosition = true;
    qint64 m_holdMs = 500;
    KalmanFilter2D m_filter;
    qint64 m_lastUpdateMs = 0;
    qint64 m_lastSeenMs = 0;

    // Apparence de l'objet mémorisée à la sélection, pour l'indice de confiance.
    cv::Mat m_refHist;   // histogramme teinte-saturation (mis à jour lentement)
    cv::Mat m_refGray;   // motif en niveaux de gris, mis à jour lentement (suit les rotations lentes)
    cv::Mat m_initGray;  // motif d'origine, tel qu'à la sélection
    cv::Mat m_initHist;  // couleurs d'origine
    int m_lowConfidenceFrames = 0;
    int m_lostFrames = 0;    // images consécutives sans l'objet
    bool m_lastTouchedBorder = false;  // le dernier cadre trouvé touchait le bord de l'image
    cv::Rect m_lastRoi;      // dernier cadre trouvé (image de travail)
    cv::Rect m_candidate;    // candidat de reprise à confirmer à la recherche suivante

    // Détection de mouvement (dans l'unité courante : mm sur le sol ou pixels).
    double m_motionMm = 20.0;
    double m_motionPx = 15.0;
    bool m_anchorValid = false;
    bool m_anchorOnFloor = false;
    QPointF m_anchor;          // position de repos
    QPointF m_lastMotionPos;
    QPointF m_moveStartPos;
    qint64 m_moveStartMs = 0;
    std::deque<std::pair<qint64, QPointF>> m_recentPositions;  // dernières positions (fin de mouvement)
    double m_travelled = 0.0;
    bool m_lostReported = false;
    std::vector<MotionEvent> m_events;
    QSizeF m_lastBoxSize;    // taille du cadre (image de travail) quand l'objet était trouvé
    TrackedObject m_state;
    std::deque<TrailPoint> m_trail;
};
