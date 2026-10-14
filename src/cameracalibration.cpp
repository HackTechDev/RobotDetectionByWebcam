#include "cameracalibration.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include <cmath>

#include <QDir>
#include <QFileInfo>
#include <QObject>

namespace {

// Largeur maximale utilisée pour la recherche rapide du damier.
constexpr int kSearchWidth = 640;

cv::Mat wrapGray(const QImage &gray)
{
    return cv::Mat(gray.height(), gray.width(), CV_8UC1, const_cast<uchar *>(gray.constBits()),
                   size_t(gray.bytesPerLine()));
}

} // namespace

void CameraCalibration::setBoard(const Board &board)
{
    if (board == m_board)
        return;
    m_board = board;
    clearCaptures();
}

bool CameraCalibration::findBoard(const QImage &image, std::vector<QPointF> &corners) const
{
    corners.clear();
    if (image.isNull())
        return false;

    const QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    cv::Mat mat = wrapGray(gray);
    double scale = 1.0;
    if (mat.cols > kSearchWidth) {
        scale = double(mat.cols) / kSearchWidth;
        cv::Mat small;
        cv::resize(mat, small, cv::Size(), 1.0 / scale, 1.0 / scale, cv::INTER_AREA);
        mat = small;
    }

    std::vector<cv::Point2f> pts;
    const bool found = cv::findChessboardCorners(
        mat, cv::Size(m_board.cols, m_board.rows), pts,
        cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);
    if (!found)
        return false;

    corners.reserve(pts.size());
    for (const cv::Point2f &p : pts)
        corners.emplace_back(p.x * scale, p.y * scale);
    return true;
}

void CameraCalibration::addCapture(const QImage &image, const std::vector<QPointF> &corners)
{
    if (image.size() != m_captureSize) {
        clearCaptures();
        m_captureSize = image.size();
    }

    std::vector<cv::Point2f> pts;
    pts.reserve(corners.size());
    for (const QPointF &p : corners)
        pts.emplace_back(float(p.x()), float(p.y()));

    const QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    cv::cornerSubPix(wrapGray(gray), pts, cv::Size(11, 11), cv::Size(-1, -1),
                     cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.01));
    m_imagePoints.push_back(std::move(pts));
}

void CameraCalibration::reset()
{
    clearCaptures();
    m_cameraMatrix = cv::Mat();
    m_distCoeffs = cv::Mat();
    m_imageSize = QSize();
    m_rms = 0.0;
}

void CameraCalibration::clearCaptures()
{
    m_imagePoints.clear();
    m_captureSize = QSize();
}

bool CameraCalibration::calibrate(QString *error)
{
    if (m_imagePoints.size() < 5) {
        if (error)
            *error = QObject::tr("Il faut au moins 5 captures du damier.");
        return false;
    }

    std::vector<cv::Point3f> board;
    for (int y = 0; y < m_board.rows; ++y)
        for (int x = 0; x < m_board.cols; ++x)
            board.emplace_back(float(x * m_board.squareSize), float(y * m_board.squareSize), 0.f);
    const std::vector<std::vector<cv::Point3f>> objectPoints(m_imagePoints.size(), board);

    cv::Mat cameraMatrix, distCoeffs;
    std::vector<cv::Mat> rvecs, tvecs;
    try {
        // k3 bloqué à 0 : libre, il compense k2 dans la zone couverte par le damier et fait
        // diverger la correction de distorsion près des bords de l'image (sur-ajustement).
        m_rms = cv::calibrateCamera(objectPoints, m_imagePoints,
                                    cv::Size(m_captureSize.width(), m_captureSize.height()),
                                    cameraMatrix, distCoeffs, rvecs, tvecs, cv::CALIB_FIX_K3);
    } catch (const cv::Exception &e) {
        if (error)
            *error = QString::fromStdString(e.what());
        return false;
    }

    m_cameraMatrix = cameraMatrix;
    m_distCoeffs = distCoeffs;
    m_imageSize = m_captureSize;
    return true;
}

bool CameraCalibration::isValidFor(const QSize &size) const
{
    if (!isCalibrated() || size.isEmpty() || m_imageSize.isEmpty())
        return false;
    const double calibRatio = double(m_imageSize.width()) / m_imageSize.height();
    const double ratio = double(size.width()) / size.height();
    return std::abs(ratio - calibRatio) / calibRatio < 0.01;
}

cv::Mat CameraCalibration::cameraMatrixFor(const QSize &size) const
{
    if (m_cameraMatrix.empty() || size == m_imageSize || m_imageSize.isEmpty())
        return m_cameraMatrix;

    // fx, cx suivent la largeur ; fy, cy suivent la hauteur.
    cv::Mat k = m_cameraMatrix.clone();
    const double sx = double(size.width()) / m_imageSize.width();
    const double sy = double(size.height()) / m_imageSize.height();
    k.at<double>(0, 0) *= sx;
    k.at<double>(0, 2) *= sx;
    k.at<double>(1, 1) *= sy;
    k.at<double>(1, 2) *= sy;
    return k;
}

bool CameraCalibration::save(const QString &path) const
{
    if (!isCalibrated())
        return false;
    QDir().mkpath(QFileInfo(path).absolutePath());
    cv::FileStorage fs(path.toStdString(), cv::FileStorage::WRITE);
    if (!fs.isOpened())
        return false;
    fs << "image_width" << m_imageSize.width();
    fs << "image_height" << m_imageSize.height();
    fs << "camera_matrix" << m_cameraMatrix;
    fs << "distortion_coefficients" << m_distCoeffs;
    fs << "rms_error" << m_rms;
    fs << "board_cols" << m_board.cols;
    fs << "board_rows" << m_board.rows;
    fs << "square_size_mm" << m_board.squareSize;
    return true;
}

bool CameraCalibration::load(const QString &path)
{
    if (!QFileInfo::exists(path))
        return false;
    try {
        cv::FileStorage fs(path.toStdString(), cv::FileStorage::READ);
        if (!fs.isOpened())
            return false;
        cv::Mat k, d;
        int w = 0, h = 0;
        fs["image_width"] >> w;
        fs["image_height"] >> h;
        fs["camera_matrix"] >> k;
        fs["distortion_coefficients"] >> d;
        if (k.rows != 3 || k.cols != 3 || w <= 0 || h <= 0)
            return false;
        fs["rms_error"] >> m_rms;
        m_cameraMatrix = k;
        m_distCoeffs = d;
        m_imageSize = QSize(w, h);
    } catch (const cv::Exception &) {
        return false;
    }
    return true;
}

QImage CameraCalibration::generateBoard(const Board &board, int squarePx)
{
    const int squaresX = board.cols + 1;
    const int squaresY = board.rows + 1;
    const int margin = squarePx;
    QImage img(squaresX * squarePx + 2 * margin, squaresY * squarePx + 2 * margin,
               QImage::Format_Grayscale8);
    img.fill(255);
    for (int y = 0; y < squaresY; ++y) {
        for (int x = 0; x < squaresX; ++x) {
            if ((x + y) % 2)
                continue;
            for (int py = 0; py < squarePx; ++py) {
                uchar *line = img.scanLine(margin + y * squarePx + py) + margin + x * squarePx;
                std::fill(line, line + squarePx, uchar(0));
            }
        }
    }
    return img;
}
