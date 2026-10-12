#include "camerasettingsdialog.h"

#include <QCamera>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace {

// Curseur d'exposition logarithmique : de 0,5 ms à 250 ms. Qt ne donne pas toujours les
// limites de la caméra, qui arrondit de toute façon la valeur demandée.
constexpr double kMinExposureMs = 0.5;
constexpr double kMaxExposureMs = 250.0;
constexpr int kExposureSteps = 1000;

double sliderToMs(int value)
{
    return kMinExposureMs * std::pow(kMaxExposureMs / kMinExposureMs, double(value) / kExposureSteps);
}

int msToSlider(double ms)
{
    const double t = std::log(ms / kMinExposureMs) / std::log(kMaxExposureMs / kMinExposureMs);
    return int(std::lround(std::clamp(t, 0.0, 1.0) * kExposureSteps));
}

QString formatMs(double ms)
{
    return ms < 10 ? QString::number(ms, 'f', 1) : QString::number(ms, 'f', 0);
}

// Remet l'exposition automatique en écrivant directement le contrôle V4L2. Qt (backend FFmpeg)
// n'y parvient pas avec les webcams qui n'acceptent que le mode « priorité ouverture » (comme la
// Logitech C270) : la caméra garde alors le dernier temps de pose manuel.
bool restoreAutoExposure(const QCamera *camera)
{
#ifdef Q_OS_LINUX
    const QByteArray device = camera->cameraDevice().id();  // par exemple /dev/video0
    const int fd = ::open(device.constData(), O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return false;
    bool ok = false;
    for (int mode : {int(V4L2_EXPOSURE_AUTO), int(V4L2_EXPOSURE_APERTURE_PRIORITY)}) {
        v4l2_control control{};
        control.id = V4L2_CID_EXPOSURE_AUTO;
        control.value = mode;
        if (::ioctl(fd, VIDIOC_S_CTRL, &control) == 0) {
            ok = true;
            break;
        }
    }
    ::close(fd);
    return ok;
#else
    Q_UNUSED(camera);
    return false;
#endif
}

QWidget *sliderRow(QSlider *slider, QLabel *value)
{
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->addWidget(slider);
    value->setMinimumWidth(110);
    h->addWidget(value);
    return row;
}

} // namespace

CameraSettingsDialog::CameraSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Réglages de l'image"));
    auto *layout = new QVBoxLayout(this);

    auto *expoBox = new QGroupBox(tr("Exposition"), this);
    auto *expoForm = new QFormLayout(expoBox);
    m_exposureModeCombo = new QComboBox(expoBox);
    m_exposureModeCombo->addItems({tr("Automatique"), tr("Manuelle")});
    expoForm->addRow(tr("Mode"), m_exposureModeCombo);
    m_exposureSlider = new QSlider(Qt::Horizontal, expoBox);
    m_exposureSlider->setRange(0, kExposureSteps);
    m_exposureValueLabel = new QLabel(expoBox);
    expoForm->addRow(tr("Temps de pose"), sliderRow(m_exposureSlider, m_exposureValueLabel));
    m_exposureReadout = new QLabel(expoBox);
    expoForm->addRow(m_exposureReadout);
    auto *expoHelp = new QLabel(tr("<i>Une pose courte donne plus d'images par seconde et moins de flou "
                                   "quand le robot bouge, mais une image plus sombre : éclairez davantage "
                                   "la scène. La caméra arrondit la valeur demandée.</i>"),
                                expoBox);
    expoHelp->setWordWrap(true);
    expoForm->addRow(expoHelp);
    layout->addWidget(expoBox);

    auto *wbBox = new QGroupBox(tr("Balance des blancs"), this);
    auto *wbForm = new QFormLayout(wbBox);
    m_whiteBalanceModeCombo = new QComboBox(wbBox);
    m_whiteBalanceModeCombo->addItems({tr("Automatique"), tr("Manuelle")});
    wbForm->addRow(tr("Mode"), m_whiteBalanceModeCombo);
    m_temperatureSlider = new QSlider(Qt::Horizontal, wbBox);
    m_temperatureSlider->setRange(20, 75);  // centaines de kelvins : 2000 à 7500 K
    m_temperatureValueLabel = new QLabel(wbBox);
    wbForm->addRow(tr("Température"), sliderRow(m_temperatureSlider, m_temperatureValueLabel));
    auto *wbHelp = new QLabel(tr("<i>Température de la lumière de la pièce : basse pour une lumière "
                                 "chaude (ampoule), haute pour la lumière du jour. En manuel, les "
                                 "couleurs ne dérivent plus, ce qui stabilise la détection du carré "
                                 "coloré.</i>"),
                              wbBox);
    wbHelp->setWordWrap(true);
    wbForm->addRow(wbHelp);
    layout->addWidget(wbBox);

    m_supportLabel = new QLabel(this);
    m_supportLabel->setWordWrap(true);
    layout->addWidget(m_supportLabel);

    // Boutons libellés à la main : les boutons standard de Qt s'afficheraient en anglais.
    auto *buttons = new QDialogButtonBox(this);
    QPushButton *reset = buttons->addButton(tr("Tout en automatique"), QDialogButtonBox::ResetRole);
    QPushButton *closeButton = buttons->addButton(tr("Fermer"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(reset, &QPushButton::clicked, this, [this] {
        Settings s = m_settings;
        s.manualExposure = false;
        s.manualWhiteBalance = false;
        setSettings(s);
        apply(m_camera, m_settings);
        emit settingsChanged();
    });
    for (QComboBox *combo : {m_exposureModeCombo, m_whiteBalanceModeCombo})
        connect(combo, &QComboBox::currentIndexChanged, this, &CameraSettingsDialog::updateFromWidgets);
    for (QSlider *slider : {m_exposureSlider, m_temperatureSlider})
        connect(slider, &QSlider::valueChanged, this, &CameraSettingsDialog::updateFromWidgets);

    // Temps de pose réellement appliqué (aussi en automatique), rafraîchi pendant l'affichage.
    m_readoutTimer = new QTimer(this);
    m_readoutTimer->setInterval(500);
    connect(m_readoutTimer, &QTimer::timeout, this, &CameraSettingsDialog::updateReadout);
    m_readoutTimer->start();

    updateWidgets();
}

void CameraSettingsDialog::setCamera(QCamera *camera)
{
    m_camera = camera;
    updateWidgets();
}

void CameraSettingsDialog::setSettings(const Settings &settings)
{
    m_settings = settings;
    updateWidgets();
}

void CameraSettingsDialog::apply(QCamera *camera, const Settings &s)
{
    if (!camera)
        return;
    if (s.manualExposure && camera->isExposureModeSupported(QCamera::ExposureManual)) {
        camera->setExposureMode(QCamera::ExposureManual);
        camera->setManualExposureTime(float(s.exposureMs / 1000.0));
    } else if (camera->isExposureModeSupported(QCamera::ExposureAuto)) {
        camera->setExposureMode(QCamera::ExposureAuto);
        restoreAutoExposure(camera);
    }
    if (s.manualWhiteBalance && camera->isWhiteBalanceModeSupported(QCamera::WhiteBalanceManual)) {
        camera->setWhiteBalanceMode(QCamera::WhiteBalanceManual);
        camera->setColorTemperature(s.colorTemperature);
    } else if (camera->isWhiteBalanceModeSupported(QCamera::WhiteBalanceAuto)) {
        camera->setWhiteBalanceMode(QCamera::WhiteBalanceAuto);
    }
}

void CameraSettingsDialog::updateFromWidgets()
{
    m_settings.manualExposure = m_exposureModeCombo->currentIndex() == 1;
    m_settings.exposureMs = sliderToMs(m_exposureSlider->value());
    m_settings.manualWhiteBalance = m_whiteBalanceModeCombo->currentIndex() == 1;
    m_settings.colorTemperature = m_temperatureSlider->value() * 100;
    updateWidgets();
    apply(m_camera, m_settings);
    emit settingsChanged();
}

void CameraSettingsDialog::updateWidgets()
{
    const bool manualExpoOk = m_camera && m_camera->isExposureModeSupported(QCamera::ExposureManual);
    const bool manualWbOk = m_camera && m_camera->isWhiteBalanceModeSupported(QCamera::WhiteBalanceManual);

    const QSignalBlocker b1(m_exposureModeCombo), b2(m_exposureSlider), b3(m_whiteBalanceModeCombo),
        b4(m_temperatureSlider);
    m_exposureModeCombo->setCurrentIndex(m_settings.manualExposure ? 1 : 0);
    m_exposureSlider->setValue(msToSlider(m_settings.exposureMs));
    m_whiteBalanceModeCombo->setCurrentIndex(m_settings.manualWhiteBalance ? 1 : 0);
    m_temperatureSlider->setValue(m_settings.colorTemperature / 100);

    m_exposureModeCombo->setEnabled(manualExpoOk);
    m_exposureSlider->setEnabled(manualExpoOk && m_settings.manualExposure);
    m_whiteBalanceModeCombo->setEnabled(manualWbOk);
    m_temperatureSlider->setEnabled(manualWbOk && m_settings.manualWhiteBalance);

    const double ms = m_settings.exposureMs;
    m_exposureValueLabel->setText(tr("%1 ms").arg(formatMs(ms)));
    m_temperatureValueLabel->setText(tr("%1 K").arg(m_settings.colorTemperature));

    QStringList missing;
    if (!manualExpoOk)
        missing << tr("exposition manuelle");
    if (!manualWbOk)
        missing << tr("balance des blancs manuelle");
    m_supportLabel->setText(missing.isEmpty() || !m_camera
                                ? QString()
                                : tr("<b>Non pris en charge par cette caméra :</b> %1.").arg(missing.join(tr(", "))));
    updateReadout();
}

void CameraSettingsDialog::updateReadout()
{
    if (!isVisible())
        return;
    if (!m_camera || !m_camera->isActive()) {
        m_exposureReadout->setText(tr("<i>Caméra inactive.</i>"));
        return;
    }
    // En automatique, le pilote ne met pas toujours à jour la valeur lue : on ne l'affiche pas.
    if (m_camera->exposureMode() != QCamera::ExposureManual) {
        m_exposureReadout->setText(tr("<i>Temps de pose réglé par la caméra selon la lumière.</i>"));
        return;
    }
    const double ms = m_camera->exposureTime() * 1000.0;
    if (ms <= 0) {
        m_exposureReadout->clear();
        return;
    }
    m_exposureReadout->setText(tr("Appliqué par la caméra : <b>%1 ms</b>, soit au plus %2 images/s")
                                   .arg(formatMs(ms))
                                   .arg(1000.0 / ms, 0, 'f', 0));
}
