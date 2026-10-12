#pragma once

#include <QImage>
#include <QPointF>
#include <QPolygonF>
#include <QStringList>
#include <QVector3D>
#include <memory>
#include <vector>

namespace cv::aruco {
class ArucoDetector;
}

class CameraCalibration;

// Un marqueur ArUco détecté, exprimé dans les coordonnées de l'image d'origine.
struct DetectedMarker
{
    int id = -1;
    QPolygonF corners;   // 4 coins dans l'ordre ArUco (haut-gauche, haut-droit, bas-droit, bas-gauche)
    QPointF center;
    double side = 0.0;   // côté moyen en pixels
    double angle = 0.0;  // orientation en degrés dans ]-180, 180] (0 = marqueur droit, sens horaire positif)

    // Pose 3D, disponible seulement si la caméra est calibrée.
    // Repère caméra : X vers la droite, Y vers le bas, Z vers l'avant (en mm).
    bool hasPose = false;
    QVector3D position;     // centre du marqueur en mm
    QVector3D rotation;     // angles d'Euler (X, Y, Z) en degrés ; (0, 0, 0) = marqueur droit face à la caméra
    double distance = 0.0;  // distance caméra -> marqueur en mm
    QPointF axes[4];        // projection de l'origine, des axes X et Y du marqueur et de sa normale
};

// Détecte des marqueurs ArUco avec le module objdetect d'OpenCV.
class MarkerDetector
{
public:
    MarkerDetector();
    ~MarkerDetector();

    // Noms des dictionnaires proposés, dans l'ordre attendu par setDictionary().
    static QStringList dictionaryNames();

    void setDictionary(int index);
    int dictionaryIndex() const { return m_dictIndex; }
    // Nombre de marqueurs du dictionnaire courant (identifiants 0 à size-1).
    int dictionarySize() const;

    // Côté réel du marqueur imprimé (partie noire, sans la marge), en mm.
    void setMarkerLength(double mm) { m_markerLength = mm; }
    double markerLength() const { return m_markerLength; }

    // Calibration utilisée pour la pose 3D (peut être nulle ou non calibrée).
    void setCalibration(const CameraCalibration *calibration) { m_calibration = calibration; }

    // Retourne les marqueurs trouvés, triés par identifiant.
    std::vector<DetectedMarker> detect(const QImage &image);

    // Image d'un marqueur à imprimer, avec une marge blanche.
    QImage generateMarker(int id, int sizePx) const;

private:
    std::unique_ptr<cv::aruco::ArucoDetector> m_detector;
    int m_dictIndex = 0;
    double m_markerLength = 50.0;
    const CameraCalibration *m_calibration = nullptr;
};
