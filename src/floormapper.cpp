#include "floormapper.h"
#include "cameracalibration.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include <cmath>

namespace {

constexpr double kRadToDeg = 180.0 / M_PI;

QPointF midpoint(const QPointF &a, const QPointF &b)
{
    return (a + b) / 2;
}

} // namespace

void FloorMapper::setPoints(const std::vector<QPointF> &points, const QSize &imageSize)
{
    m_points = points;
    m_pointsImageSize = imageSize;
    m_ready = false;
}

void FloorMapper::clearPoints()
{
    m_points.clear();
    m_pointsImageSize = QSize();
    m_ready = false;
}

bool FloorMapper::isValidFor(const QSize &imageSize) const
{
    if (!isDefined() || imageSize.isEmpty() || m_pointsImageSize.isEmpty())
        return false;
    const double zoneRatio = double(m_pointsImageSize.width()) / m_pointsImageSize.height();
    const double ratio = double(imageSize.width()) / imageSize.height();
    return std::abs(ratio - zoneRatio) / zoneRatio < 0.01;
}

std::vector<QPointF> FloorMapper::pointsFor(const QSize &imageSize) const
{
    std::vector<QPointF> out;
    if (!isValidFor(imageSize))
        return out;
    const double sx = double(imageSize.width()) / m_pointsImageSize.width();
    const double sy = double(imageSize.height()) / m_pointsImageSize.height();
    for (const QPointF &p : m_points)
        out.emplace_back(p.x() * sx, p.y() * sy);
    return out;
}

bool FloorMapper::prepare(const QSize &imageSize)
{
    m_ready = false;
    m_poseValid = false;
    const std::vector<QPointF> pts = pointsFor(imageSize);
    if (pts.size() != 4 || m_sizeMm.width() <= 0 || m_sizeMm.height() <= 0)
        return false;

    m_undistort = m_calibration && m_calibration->isValidFor(imageSize);
    if (m_undistort) {
        m_cameraMatrix = m_calibration->cameraMatrixFor(imageSize);
        m_distCoeffs = m_calibration->distCoeffs();
    }

    std::vector<cv::Point2f> image;
    for (const QPointF &p : pts)
        image.emplace_back(float(p.x()), float(p.y()));
    image = undistort(image);

    // Quadrilatère dégénéré (points confondus ou alignés) : pas de transformation possible.
    if (std::abs(cv::contourArea(image)) < 100.0)
        return false;

    const float w = float(m_sizeMm.width()), h = float(m_sizeMm.height());
    const std::vector<cv::Point2f> floor = {{0, 0}, {w, 0}, {w, h}, {0, h}};
    m_homography = cv::getPerspectiveTransform(image, floor);
    m_ready = !m_homography.empty();

    // Avec la calibration : position de la caméra par rapport au sol, à partir des 4 points.
    if (m_ready && m_undistort) {
        std::vector<cv::Point2f> distorted;
        for (const QPointF &p : pts)
            distorted.emplace_back(float(p.x()), float(p.y()));
        const std::vector<cv::Point3f> floor3d = {{0, 0, 0}, {w, 0, 0}, {w, h, 0}, {0, h, 0}};
        cv::Mat rvec, tvec;
        if (cv::solvePnP(floor3d, distorted, m_cameraMatrix, m_distCoeffs, rvec, tvec, false, cv::SOLVEPNP_IPPE)) {
            cv::Mat r;
            cv::Rodrigues(rvec, r);
            m_rotation = cv::Matx33d(r);
            const cv::Vec3d t(tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2));
            m_cameraCenter = -(m_rotation.t() * t);
            m_poseValid = std::abs(m_cameraCenter[2]) > 1.0;
        }
    }
    return m_ready;
}

std::vector<cv::Point2f> FloorMapper::undistort(const std::vector<cv::Point2f> &points) const
{
    if (!m_undistort || points.empty())
        return points;
    // Résultat exprimé en pixels d'une image sans distorsion (même matrice intrinsèque).
    std::vector<cv::Point2f> out;
    cv::undistortPoints(points, out, m_cameraMatrix, m_distCoeffs, cv::noArray(), m_cameraMatrix);
    return out;
}

QPointF FloorMapper::toFloor(const QPointF &imagePoint, double heightMm) const
{
    if (!m_ready)
        return {};
    if (heightMm > 0 && m_poseValid) {
        // Rayon de vue du point (repère de la caméra, coordonnées normalisées sans distorsion),
        // exprimé dans le repère du sol, puis intersection avec le plan situé à « heightMm » du
        // sol, du côté de la caméra.
        std::vector<cv::Point2f> in{cv::Point2f(float(imagePoint.x()), float(imagePoint.y()))}, normalized;
        cv::undistortPoints(in, normalized, m_cameraMatrix, m_distCoeffs);
        const cv::Vec3d ray = m_rotation.t() * cv::Vec3d(normalized[0].x, normalized[0].y, 1.0);
        const double planeZ = m_cameraCenter[2] > 0 ? heightMm : -heightMm;
        if (std::abs(ray[2]) > 1e-9) {
            const double s = (planeZ - m_cameraCenter[2]) / ray[2];
            if (s > 0) {
                const cv::Vec3d p = m_cameraCenter + s * ray;
                return QPointF(p[0], p[1]);
            }
        }
    }
    std::vector<cv::Point2f> in = undistort({cv::Point2f(float(imagePoint.x()), float(imagePoint.y()))});
    std::vector<cv::Point2f> out;
    cv::perspectiveTransform(in, out, m_homography);
    return QPointF(out[0].x, out[0].y);
}

double FloorMapper::heading(const QPointF &from, const QPointF &to, double heightMm) const
{
    const QPointF d = toFloor(to, heightMm) - toFloor(from, heightMm);
    return std::atan2(d.y(), d.x()) * kRadToDeg;
}

void FloorMapper::apply(std::vector<DetectedSquare> &squares, std::vector<DetectedMarker> &markers) const
{
    if (!m_ready)
        return;
    for (DetectedSquare &s : squares) {
        s.hasFloor = true;
        s.floorPosition = toFloor(s.center, m_objectHeight);
        // Un carré est symétrique : son cap n'est connu qu'à 90° près.
        const double a = heading(s.center, midpoint(s.corners[0], s.corners[1]), m_objectHeight);
        s.floorHeading = std::fmod(std::fmod(a, 90.0) + 90.0, 90.0);
    }
    for (DetectedMarker &m : markers) {
        m.hasFloor = true;
        m.floorPosition = toFloor(m.center, m_objectHeight);
        // Cap = direction du bord supérieur du marqueur (coins 0 et 1).
        m.floorHeading = heading(m.center, midpoint(m.corners[0], m.corners[1]), m_objectHeight);
    }
}
