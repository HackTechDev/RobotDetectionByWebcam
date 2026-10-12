#pragma once

#include <QImage>
#include <QPointF>
#include <QPolygonF>
#include <vector>

// Un carré détecté, exprimé dans les coordonnées de l'image d'origine.
struct DetectedSquare
{
    QPolygonF corners;   // 4 coins (rectangle orienté de surface minimale)
    QPointF center;
    double side = 0.0;   // côté moyen en pixels
    double angle = 0.0;  // orientation en degrés, dans [0, 90[
    double area = 0.0;   // surface du blob en pixels

    // Position sur le sol, disponible seulement si le repère du sol est défini.
    bool hasFloor = false;
    QPointF floorPosition;    // en mm dans le repère du sol
    double floorHeading = 0;  // en degrés dans [0, 90[ (un carré est symétrique)
};

// Détecte des carrés d'une couleur donnée avec OpenCV : seuillage HSV,
// contours puis analyse de forme (rectangle orienté minimal).
class SquareDetector
{
public:
    struct Params
    {
        // Mode « couleur » : sélection par teinte.
        int hue = 0;            // teinte cible [0, 359]
        int hueTolerance = 15;  // tolérance en degrés
        int satMin = 90;        // saturation minimale [0, 255]
        int valMin = 60;        // valeur minimale [0, 255]

        // Mode « blanc, gris ou noir » : pas de teinte, sélection des pixels peu saturés
        // dont la luminosité est proche de la cible.
        bool achromatic = false;
        int value = 255;          // luminosité cible [0, 255]
        int valueTolerance = 50;  // écart de luminosité accepté
        int satMax = 60;          // saturation maximale [0, 255]

        int minArea = 120;      // surface minimale (pixels de l'image de travail)
        double minAspect = 0.75;  // rapport petit côté / grand côté
        double minFill = 0.82;    // surface blob / surface du rectangle orienté
        int processWidth = 320;   // largeur de l'image de travail
    };

    void setParams(const Params &params) { m_params = params; }
    const Params &params() const { return m_params; }

    // Retourne les carrés trouvés, triés par surface décroissante.
    std::vector<DetectedSquare> detect(const QImage &image);

    // Masque binaire de la dernière détection (taille de l'image de travail).
    const QImage &lastMask() const { return m_mask; }

private:
    Params m_params;
    QImage m_mask;
};
