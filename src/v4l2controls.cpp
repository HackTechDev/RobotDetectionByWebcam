#include "v4l2controls.h"

#include <QObject>

#include <algorithm>

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace V4l2 {

#ifdef Q_OS_LINUX

namespace {
// Contrôles propres aux webcams Logitech (pilote uvcvideo).
constexpr quint32 kLogitechLedMode = 0x0a046d05;
constexpr quint32 kLogitechLedFrequency = 0x0a046d06;
} // namespace

namespace {

// Descripteur ouvert le temps d'un appel ; les contrôles se règlent même pendant que la caméra
// filme dans un autre descripteur.
class Device
{
public:
    explicit Device(const QByteArray &path)
        : m_fd(::open(path.constData(), O_RDWR | O_NONBLOCK))
    {
    }
    ~Device()
    {
        if (m_fd >= 0)
            ::close(m_fd);
    }
    bool isOpen() const { return m_fd >= 0; }
    int fd() const { return m_fd; }

private:
    int m_fd;
};

} // namespace

std::vector<Control> listControls(const QByteArray &device)
{
    std::vector<Control> controls;
    const Device dev(device);
    if (!dev.isOpen())
        return controls;
    v4l2_queryctrl query{};
    query.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while (::ioctl(dev.fd(), VIDIOC_QUERYCTRL, &query) == 0) {
        const bool usable = !(query.flags & (V4L2_CTRL_FLAG_DISABLED | V4L2_CTRL_FLAG_READ_ONLY))
                            && (query.type == V4L2_CTRL_TYPE_INTEGER || query.type == V4L2_CTRL_TYPE_BOOLEAN
                                || query.type == V4L2_CTRL_TYPE_MENU);
        if (usable) {
            Control c;
            c.id = query.id;
            c.name = QString::fromUtf8(reinterpret_cast<const char *>(query.name));
            c.type = query.type == V4L2_CTRL_TYPE_BOOLEAN ? Control::Boolean
                     : query.type == V4L2_CTRL_TYPE_MENU  ? Control::Menu
                                                          : Control::Integer;
            c.minimum = query.minimum;
            c.maximum = query.maximum;
            c.step = std::max(1, int(query.step));
            c.defaultValue = query.default_value;
            c.inactive = query.flags & V4L2_CTRL_FLAG_INACTIVE;
            v4l2_control value{};
            value.id = query.id;
            c.value = ::ioctl(dev.fd(), VIDIOC_G_CTRL, &value) == 0 ? value.value : query.default_value;
            if (c.type == Control::Menu) {
                for (int i = query.minimum; i <= query.maximum; ++i) {
                    v4l2_querymenu item{};
                    item.id = query.id;
                    item.index = quint32(i);
                    if (::ioctl(dev.fd(), VIDIOC_QUERYMENU, &item) == 0)
                        c.menu.append({i, QString::fromUtf8(reinterpret_cast<const char *>(item.name))});
                }
            }
            controls.push_back(c);
        }
        query.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }
    return controls;
}

bool setControl(const QByteArray &device, quint32 id, int value)
{
    const Device dev(device);
    if (!dev.isOpen())
        return false;
    v4l2_control control{};
    control.id = id;
    control.value = value;
    return ::ioctl(dev.fd(), VIDIOC_S_CTRL, &control) == 0;
}

bool exposureRange(const QByteArray &device, double *minMs, double *maxMs)
{
    const Device dev(device);
    if (!dev.isOpen())
        return false;
    v4l2_queryctrl query{};
    query.id = V4L2_CID_EXPOSURE_ABSOLUTE;  // en unités de 100 µs
    if (::ioctl(dev.fd(), VIDIOC_QUERYCTRL, &query) != 0 || (query.flags & V4L2_CTRL_FLAG_DISABLED))
        return false;
    *minMs = std::max(1, int(query.minimum)) / 10.0;
    *maxMs = query.maximum / 10.0;
    return *maxMs > *minMs;
}

bool restoreAutoExposure(const QByteArray &device)
{
    for (int mode : {int(V4L2_EXPOSURE_AUTO), int(V4L2_EXPOSURE_APERTURE_PRIORITY)}) {
        if (setControl(device, V4L2_CID_EXPOSURE_AUTO, mode))
            return true;
    }
    return false;
}

bool isExposureOrWhiteBalance(quint32 id)
{
    return id == V4L2_CID_EXPOSURE_AUTO || id == V4L2_CID_EXPOSURE_ABSOLUTE || id == V4L2_CID_AUTO_WHITE_BALANCE
           || id == V4L2_CID_WHITE_BALANCE_TEMPERATURE;
}

QString frenchName(const Control &c)
{
    switch (c.id) {
    case V4L2_CID_BRIGHTNESS: return QObject::tr("Luminosité");
    case V4L2_CID_CONTRAST: return QObject::tr("Contraste");
    case V4L2_CID_SATURATION: return QObject::tr("Saturation");
    case V4L2_CID_HUE: return QObject::tr("Teinte");
    case V4L2_CID_GAMMA: return QObject::tr("Gamma");
    case V4L2_CID_GAIN: return QObject::tr("Gain");
    case V4L2_CID_POWER_LINE_FREQUENCY: return QObject::tr("Fréquence du secteur");
    case V4L2_CID_SHARPNESS: return QObject::tr("Netteté");
    case V4L2_CID_BACKLIGHT_COMPENSATION: return QObject::tr("Compensation du contre-jour");
    case V4L2_CID_EXPOSURE_AUTO_PRIORITY: return QObject::tr("Ralentir les images en exposition auto.");
    case V4L2_CID_FOCUS_AUTO: return QObject::tr("Mise au point automatique");
    case V4L2_CID_FOCUS_ABSOLUTE: return QObject::tr("Mise au point");
    case V4L2_CID_ZOOM_ABSOLUTE: return QObject::tr("Zoom");
    case kLogitechLedMode: return QObject::tr("Voyant");
    case kLogitechLedFrequency: return QObject::tr("Clignotement du voyant");
    default: return c.name;
    }
}

QString frenchDescription(const Control &c)
{
    switch (c.id) {
    case V4L2_CID_GAIN:
        return QObject::tr("Amplification du signal : image plus claire, mais plus bruitée. En exposition "
                           "automatique, la caméra le règle elle-même.");
    case V4L2_CID_POWER_LINE_FREQUENCY:
        return QObject::tr("Évite le scintillement sous un éclairage artificiel : 50 Hz en Europe, 60 Hz en "
                           "Amérique du Nord.");
    case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
        return QObject::tr("En exposition automatique, autorise la caméra à réduire le nombre d'images par "
                           "seconde pour allonger la pose dans une pièce sombre. Décochez pour garder le débit "
                           "maximal.");
    case V4L2_CID_BACKLIGHT_COMPENSATION:
        return QObject::tr("Éclaircit le sujet quand la lumière vient de derrière lui (fenêtre).");
    case V4L2_CID_SHARPNESS:
        return QObject::tr("Renforcement des contours ; trop fort, il crée des halos et du bruit.");
    case kLogitechLedMode:
        return QObject::tr("Voyant de la webcam : « Automatique » l'allume pendant la capture.");
    default:
        return QString();
    }
}

QString frenchMenuItem(const Control &c, int value, const QString &driverLabel)
{
    if (c.id == V4L2_CID_POWER_LINE_FREQUENCY) {
        switch (value) {
        case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED: return QObject::tr("Désactivé");
        case V4L2_CID_POWER_LINE_FREQUENCY_50HZ: return QObject::tr("50 Hz (Europe)");
        case V4L2_CID_POWER_LINE_FREQUENCY_60HZ: return QObject::tr("60 Hz (Amérique du Nord)");
        case V4L2_CID_POWER_LINE_FREQUENCY_AUTO: return QObject::tr("Automatique");
        default: break;
        }
    }
    if (c.id == kLogitechLedMode) {
        switch (value) {
        case 0: return QObject::tr("Éteint");
        case 1: return QObject::tr("Allumé");
        case 2: return QObject::tr("Clignotant");
        case 3: return QObject::tr("Automatique");
        default: break;
        }
    }
    return driverLabel;
}

#else

QString frenchName(const Control &c) { return c.name; }
QString frenchDescription(const Control &) { return QString(); }
QString frenchMenuItem(const Control &, int, const QString &driverLabel) { return driverLabel; }
std::vector<Control> listControls(const QByteArray &) { return {}; }
bool setControl(const QByteArray &, quint32, int) { return false; }
bool exposureRange(const QByteArray &, double *, double *) { return false; }
bool restoreAutoExposure(const QByteArray &) { return false; }
bool isExposureOrWhiteBalance(quint32) { return false; }

#endif

} // namespace V4l2
