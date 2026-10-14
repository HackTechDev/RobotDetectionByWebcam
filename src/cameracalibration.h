#pragma once

#include <QImage>
#include <QPointF>
#include <QSize>
#include <QString>
#include <opencv2/core.hpp>
#include <vector>

// Calibration intrinsèque de la caméra à partir d'images d'un damier.
class CameraCalibration
{
public:
    struct Board
    {
        int cols = 9;              // coins intérieurs par ligne
        int rows = 6;              // coins intérieurs par colonne
        double squareSize = 25.0;  // côté d'une case en mm

        bool operator==(const Board &o) const
        {
            return cols == o.cols && rows == o.rows && squareSize == o.squareSize;
        }
    };

    // Changer de damier efface les captures en cours.
    void setBoard(const Board &board);
    const Board &board() const { return m_board; }

    // Recherche rapide du damier ; coins en coordonnées de l'image.
    bool findBoard(const QImage &image, std::vector<QPointF> &corners) const;

    // Affine les coins trouvés par findBoard() et les ajoute aux captures.
    void addCapture(const QImage &image, const std::vector<QPointF> &corners);
    int captureCount() const { return int(m_imagePoints.size()); }
    void clearCaptures();
    // Oublie la calibration (et les captures) : caméra non calibrée.
    void reset();

    // Calcule la matrice intrinsèque et la distorsion à partir des captures.
    bool calibrate(QString *error);

    bool isCalibrated() const { return !m_cameraMatrix.empty(); }
    double rmsError() const { return m_rms; }
    QSize imageSize() const { return m_imageSize; }

    // Vrai si la calibration s'applique à cette résolution (même format d'image, à 1 % près).
    bool isValidFor(const QSize &size) const;

    // Matrice intrinsèque mise à l'échelle pour une autre résolution (même format d'image).
    cv::Mat cameraMatrixFor(const QSize &size) const;
    const cv::Mat &distCoeffs() const { return m_distCoeffs; }

    bool save(const QString &path) const;
    bool load(const QString &path);

    // Image d'un damier à imprimer (cols+1 x rows+1 cases), avec une marge blanche.
    static QImage generateBoard(const Board &board, int squarePx);

private:
    Board m_board;
    std::vector<std::vector<cv::Point2f>> m_imagePoints;
    QSize m_captureSize;

    cv::Mat m_cameraMatrix;
    cv::Mat m_distCoeffs;
    QSize m_imageSize;
    double m_rms = 0.0;
};
