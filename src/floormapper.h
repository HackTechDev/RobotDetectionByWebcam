#pragma once

#include "markerdetector.h"
#include "squaredetector.h"

#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <opencv2/core.hpp>
#include <vector>

class CameraCalibration;

// Conversion des positions de l'image en millimètres sur le sol, à partir de 4 points
// cliqués aux coins d'un rectangle de dimensions connues :
//   point 1 = origine (0, 0), point 2 = (largeur, 0), point 3 = (largeur, hauteur),
//   point 4 = (0, hauteur).
// L'axe X va du point 1 au point 2, l'axe Y du point 1 au point 4.
class FloorMapper
{
public:
    // Points dans l'image réelle de la caméra, pour la résolution imageSize.
    void setPoints(const std::vector<QPointF> &points, const QSize &imageSize);
    void clearPoints();
    bool isDefined() const { return m_points.size() == 4; }
    const std::vector<QPointF> &points() const { return m_points; }
    QSize pointsImageSize() const { return m_pointsImageSize; }

    // Dimensions du rectangle de référence en mm (X = point 1 -> 2, Y = point 1 -> 4).
    void setSize(const QSizeF &sizeMm) { m_sizeMm = sizeMm; }
    QSizeF size() const { return m_sizeMm; }

    // Calibration facultative : si elle correspond à l'image, la distorsion est corrigée.
    void setCalibration(const CameraCalibration *calibration) { m_calibration = calibration; }

    // Vrai si la zone s'applique à cette résolution (même format d'image, à 1 % près).
    bool isValidFor(const QSize &imageSize) const;
    // Points de la zone mis à l'échelle pour une autre résolution (même format d'image).
    std::vector<QPointF> pointsFor(const QSize &imageSize) const;

    // Calcule la transformation pour une taille d'image. À appeler une fois par image.
    bool prepare(const QSize &imageSize);
    bool isReady() const { return m_ready; }
    bool correctsDistortion() const { return m_undistort; }

    QPointF toFloor(const QPointF &imagePoint) const;
    // Direction de « from » vers « to » (points de l'image) sur le sol, en degrés dans
    // ]-180, 180], comptée de l'axe X vers l'axe Y.
    double heading(const QPointF &from, const QPointF &to) const;

    // Remplit la position et le cap au sol des carrés et des marqueurs (après prepare()).
    void apply(std::vector<DetectedSquare> &squares, std::vector<DetectedMarker> &markers) const;

private:
    std::vector<cv::Point2f> undistort(const std::vector<cv::Point2f> &points) const;

    std::vector<QPointF> m_points;
    QSize m_pointsImageSize;
    QSizeF m_sizeMm{1000.0, 1000.0};
    const CameraCalibration *m_calibration = nullptr;

    bool m_ready = false;
    bool m_undistort = false;
    cv::Mat m_homography;
    cv::Mat m_cameraMatrix;
    cv::Mat m_distCoeffs;
};
