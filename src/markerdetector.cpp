#include "markerdetector.h"
#include "cameracalibration.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/objdetect/aruco_detector.hpp>

#include <algorithm>
#include <cmath>

namespace {

struct DictionaryEntry
{
    const char *name;
    cv::aruco::PredefinedDictionaryType type;
};

const DictionaryEntry kDictionaries[] = {
    {"4x4 (50)", cv::aruco::DICT_4X4_50},
    {"4x4 (250)", cv::aruco::DICT_4X4_250},
    {"5x5 (100)", cv::aruco::DICT_5X5_100},
    {"6x6 (250)", cv::aruco::DICT_6X6_250},
    {"ArUco original (1024)", cv::aruco::DICT_ARUCO_ORIGINAL},
    {"AprilTag 36h11 (587)", cv::aruco::DICT_APRILTAG_36h11},
};

constexpr int kDictionaryCount = int(sizeof(kDictionaries) / sizeof(kDictionaries[0]));

constexpr double kRadToDeg = 180.0 / M_PI;

// Angles d'Euler (convention Z-Y-X) d'une matrice de rotation, en degrés.
QVector3D eulerAngles(const cv::Mat &r)
{
    const double r00 = r.at<double>(0, 0), r10 = r.at<double>(1, 0), r20 = r.at<double>(2, 0);
    const double r21 = r.at<double>(2, 1), r22 = r.at<double>(2, 2);
    const double x = std::atan2(r21, r22);
    const double y = std::atan2(-r20, std::hypot(r21, r22));
    const double z = std::atan2(r10, r00);
    return QVector3D(float(x * kRadToDeg), float(y * kRadToDeg), float(z * kRadToDeg));
}

} // namespace

MarkerDetector::MarkerDetector()
{
    setDictionary(0);
}

MarkerDetector::~MarkerDetector() = default;

QStringList MarkerDetector::dictionaryNames()
{
    QStringList names;
    for (const DictionaryEntry &d : kDictionaries)
        names << QString::fromUtf8(d.name);
    return names;
}

void MarkerDetector::setDictionary(int index)
{
    m_dictIndex = std::clamp(index, 0, kDictionaryCount - 1);
    cv::aruco::DetectorParameters params;
    params.cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
    m_detector = std::make_unique<cv::aruco::ArucoDetector>(
        cv::aruco::getPredefinedDictionary(kDictionaries[m_dictIndex].type), params);
}

int MarkerDetector::dictionarySize() const
{
    return m_detector->getDictionary().bytesList.rows;
}

std::vector<DetectedMarker> MarkerDetector::detect(const QImage &image)
{
    std::vector<DetectedMarker> result;
    if (image.isNull())
        return result;

    // ArUco travaille en niveaux de gris, à pleine résolution pour les petits marqueurs.
    const QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    const cv::Mat mat(gray.height(), gray.width(), CV_8UC1,
                      const_cast<uchar *>(gray.constBits()), size_t(gray.bytesPerLine()));

    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> corners;
    m_detector->detectMarkers(mat, corners, ids);

    // Pose 3D : coins du marqueur dans son repère (ordre imposé par SOLVEPNP_IPPE_SQUARE).
    const bool withPose =
        m_calibration && m_calibration->isValidFor(image.size()) && m_markerLength > 0;
    cv::Mat cameraMatrix, distCoeffs;
    std::vector<cv::Point3f> objectPoints;
    std::vector<cv::Point3f> axisPoints;
    if (withPose) {
        cameraMatrix = m_calibration->cameraMatrixFor(image.size());
        distCoeffs = m_calibration->distCoeffs();
        const float h = float(m_markerLength / 2);
        objectPoints = {{-h, h, 0}, {h, h, 0}, {h, -h, 0}, {-h, -h, 0}};
        // Origine, axe X (droite), axe Y (bas) et normale du marqueur (vers la caméra).
        axisPoints = {{0, 0, 0}, {h, 0, 0}, {0, -h, 0}, {0, 0, h}};
    }

    for (size_t i = 0; i < ids.size(); ++i) {
        const auto &c = corners[i];
        DetectedMarker m;
        m.id = ids[i];
        double cx = 0, cy = 0, perimeter = 0;
        for (size_t k = 0; k < 4; ++k) {
            m.corners << QPointF(c[k].x, c[k].y);
            cx += c[k].x;
            cy += c[k].y;
            const cv::Point2f &n = c[(k + 1) % 4];
            perimeter += std::hypot(n.x - c[k].x, n.y - c[k].y);
        }
        m.center = QPointF(cx / 4, cy / 4);
        m.side = perimeter / 4;
        // Orientation du bord supérieur (coin 0 -> coin 1).
        m.angle = std::atan2(c[1].y - c[0].y, c[1].x - c[0].x) * kRadToDeg;

        cv::Mat rvec, tvec;
        if (withPose
            && cv::solvePnP(objectPoints, c, cameraMatrix, distCoeffs, rvec, tvec, false,
                            cv::SOLVEPNP_IPPE_SQUARE)) {
            m.hasPose = true;
            m.position = QVector3D(float(tvec.at<double>(0)), float(tvec.at<double>(1)),
                                   float(tvec.at<double>(2)));
            m.distance = m.position.length();
            // Le repère OpenCV du marqueur a Y vers le haut et Z vers la caméra. On le
            // retourne autour de X pour que (0, 0, 0) = marqueur droit, face à la caméra.
            cv::Mat rotation;
            cv::Rodrigues(rvec, rotation);
            m.rotation = eulerAngles(rotation * cv::Mat(cv::Matx33d(1, 0, 0, 0, -1, 0, 0, 0, -1)));

            std::vector<cv::Point2f> projected;
            cv::projectPoints(axisPoints, rvec, tvec, cameraMatrix, distCoeffs, projected);
            for (size_t k = 0; k < 4; ++k)
                m.axes[k] = QPointF(projected[k].x, projected[k].y);
        }
        result.push_back(m);
    }

    std::sort(result.begin(), result.end(),
              [](const DetectedMarker &a, const DetectedMarker &b) { return a.id < b.id; });
    return result;
}

QImage MarkerDetector::generateMarker(int id, int sizePx) const
{
    cv::Mat marker;
    cv::aruco::generateImageMarker(m_detector->getDictionary(), id, sizePx, marker, 1);

    // Marge blanche d'un quart de la taille : indispensable à la détection.
    const int margin = sizePx / 4;
    cv::Mat bordered;
    cv::copyMakeBorder(marker, bordered, margin, margin, margin, margin, cv::BORDER_CONSTANT,
                       cv::Scalar(255));
    return QImage(bordered.data, bordered.cols, bordered.rows, int(bordered.step),
                  QImage::Format_Grayscale8)
        .copy();
}
