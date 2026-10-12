#include "squaredetector.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>

namespace {

// Seuillage HSV avec gestion de la teinte circulaire (rouge ~ 0/180 dans OpenCV).
cv::Mat thresholdHsv(const cv::Mat &hsv, const SquareDetector::Params &p)
{
    if (p.achromatic) {
        cv::Mat mask;
        cv::inRange(hsv, cv::Scalar(0, 0, std::max(0, p.value - p.valueTolerance)),
                    cv::Scalar(179, p.satMax, std::min(255, p.value + p.valueTolerance)), mask);
        return mask;
    }

    // OpenCV code la teinte sur [0, 180[ : on divise les degrés par 2.
    const int h = p.hue / 2;
    const int tol = std::max(1, p.hueTolerance / 2);
    const int lo = h - tol;
    const int hi = h + tol;

    cv::Mat mask;
    auto range = [&](int hLo, int hHi, cv::Mat &out) {
        cv::inRange(hsv, cv::Scalar(hLo, p.satMin, p.valMin), cv::Scalar(hHi, 255, 255), out);
    };
    if (lo < 0 || hi > 179) {
        cv::Mat a, b;
        range(std::max(lo, 0), std::min(hi, 179), a);
        if (lo < 0)
            range(lo + 180, 179, b);
        else
            range(0, hi - 180, b);
        cv::bitwise_or(a, b, mask);
    } else {
        range(lo, hi, mask);
    }
    return mask;
}

} // namespace

std::vector<DetectedSquare> SquareDetector::detect(const QImage &image)
{
    std::vector<DetectedSquare> result;
    if (image.isNull())
        return result;

    const Params &p = m_params;

    // QImage RGB32 = octets B,G,R,A en mémoire : on l'enveloppe sans copie.
    const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
    const cv::Mat bgra(rgb.height(), rgb.width(), CV_8UC4,
                       const_cast<uchar *>(rgb.constBits()), size_t(rgb.bytesPerLine()));

    cv::Mat small;
    if (bgra.cols > p.processWidth) {
        const double f = double(p.processWidth) / bgra.cols;
        cv::resize(bgra, small, cv::Size(), f, f, cv::INTER_AREA);
    } else {
        small = bgra;
    }

    cv::Mat bgr, hsv;
    cv::cvtColor(small, bgr, cv::COLOR_BGRA2BGR);
    cv::GaussianBlur(bgr, bgr, cv::Size(3, 3), 0);
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);

    cv::Mat mask = thresholdHsv(hsv, p);

    // Ouverture (supprime le bruit) puis fermeture (bouche les petits trous).
    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);

    m_mask = QImage(mask.data, mask.cols, mask.rows, int(mask.step), QImage::Format_Grayscale8).copy();

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    const double scale = double(image.width()) / small.cols;
    for (const auto &contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < p.minArea)
            continue;

        const cv::RotatedRect rect = cv::minAreaRect(contour);
        const double w = rect.size.width, h = rect.size.height;
        if (w <= 0 || h <= 0)
            continue;

        const double aspect = std::min(w, h) / std::max(w, h);
        const double fill = area / (w * h);
        if (aspect < p.minAspect || fill < p.minFill)
            continue;

        cv::Point2f pts[4];
        rect.points(pts);

        DetectedSquare sq;
        for (const cv::Point2f &c : pts)
            sq.corners << QPointF(c.x * scale, c.y * scale);
        sq.center = QPointF(rect.center.x * scale, rect.center.y * scale);
        sq.side = (w + h) / 2 * scale;
        sq.area = area * scale * scale;
        sq.angle = std::fmod(std::fmod(double(rect.angle), 90.0) + 90.0, 90.0);
        result.push_back(sq);
    }

    std::sort(result.begin(), result.end(),
              [](const DetectedSquare &a, const DetectedSquare &b) { return a.area > b.area; });
    return result;
}
