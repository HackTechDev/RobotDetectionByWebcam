#include "camerasettingsdialog.h"

#include <QCamera>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLocale>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>


namespace {

// Curseur d'exposition logarithmique, entre les limites de la caméra (0,5 à 250 ms si elle ne
// les donne pas). La caméra arrondit de toute façon la valeur demandée.
constexpr int kExposureSteps = 1000;

QString formatMs(double ms)
{
    return ms < 10 ? QLocale().toString(ms, 'f', 1) : QLocale().toString(ms, 'f', 0);
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

double CameraSettingsDialog::sliderToMs(int value) const
{
    return m_minExposureMs * std::pow(m_maxExposureMs / m_minExposureMs, double(value) / kExposureSteps);
}

int CameraSettingsDialog::msToSlider(double ms) const
{
    const double t = std::log(ms / m_minExposureMs) / std::log(m_maxExposureMs / m_minExposureMs);
    return int(std::lround(std::clamp(t, 0.0, 1.0) * kExposureSteps));
}

CameraSettingsDialog::CameraSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Réglages de l'image"));
    // Exposition et balance des blancs à gauche, autres réglages à droite.
    auto *layout = new QVBoxLayout(this);
    auto *columns = new QHBoxLayout;
    auto *left = new QVBoxLayout;
    columns->addLayout(left);
    layout->addLayout(columns);

    auto *expoBox = new QGroupBox(tr("Exposition"), this);
    auto *expoForm = new QFormLayout(expoBox);
    m_exposureModeCombo = new QComboBox(expoBox);
    m_exposureModeCombo->addItems({tr("Automatique"), tr("Manuelle")});
    expoForm->addRow(tr("Mode"), m_exposureModeCombo);
    m_exposureSlider = new QSlider(Qt::Horizontal, expoBox);
    m_exposureSlider->setRange(0, kExposureSteps);
    m_exposureValueLabel = new QLabel(expoBox);
    expoForm->addRow(tr("Temps de pose"), sliderRow(m_exposureSlider, m_exposureValueLabel));
    m_exposureRangeLabel = new QLabel(expoBox);
    expoForm->addRow(m_exposureRangeLabel);
    m_exposureReadout = new QLabel(expoBox);
    expoForm->addRow(m_exposureReadout);
    auto *expoHelp = new QLabel(tr("<i>Une pose courte donne plus d'images par seconde et moins de flou "
                                   "quand le robot bouge, mais une image plus sombre : éclairez davantage "
                                   "la scène. La caméra arrondit la valeur demandée.</i>"),
                                expoBox);
    expoHelp->setWordWrap(true);
    expoForm->addRow(expoHelp);
    left->addWidget(expoBox);

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
    left->addWidget(wbBox);
    left->addStretch();

    // Autres réglages du pilote, construits selon la caméra (rebuildControls()).
    m_controlsBox = new QGroupBox(tr("Autres réglages"), this);
    auto *controlsLayout = new QVBoxLayout(m_controlsBox);
    m_controlsForm = new QFormLayout;
    controlsLayout->addLayout(m_controlsForm);
    auto *defaultsButton = new QPushButton(tr("Valeurs par défaut de la webcam"), m_controlsBox);
    defaultsButton->setToolTip(tr("Remet les autres réglages aux valeurs d'usine de la webcam"));
    controlsLayout->addWidget(defaultsButton);
    m_controlsBox->setMinimumWidth(380);
    columns->addWidget(m_controlsBox, 0, Qt::AlignTop);
    connect(defaultsButton, &QPushButton::clicked, this, [this] {
        if (!m_camera)
            return;
        const QByteArray device = m_camera->cameraDevice().id();
        // Seules les valeurs différentes de celles de l'ouverture restent à mémoriser.
        m_settings.controls.clear();
        for (const V4l2::Control &c : m_controls) {
            V4l2::setControl(device, c.id, c.defaultValue);
            if (m_initialValues.value(c.id, c.defaultValue) != c.defaultValue)
                m_settings.controls.insert(c.id, c.defaultValue);
        }
        rebuildControls();
        emit settingsChanged();
    });

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
    m_initialValues.clear();
    m_minExposureMs = 0.5;
    m_maxExposureMs = 250.0;
    if (m_camera) {
        const QByteArray device = m_camera->cameraDevice().id();
        // Vraies limites du temps de pose, si la webcam les donne.
        double minMs = 0, maxMs = 0;
        if (V4l2::exposureRange(device, &minMs, &maxMs)) {
            m_minExposureMs = minMs;
            m_maxExposureMs = maxMs;
        }
        // Valeurs à l'ouverture, pour rendre la webcam dans cet état à la fermeture.
        for (const V4l2::Control &c : V4l2::listControls(device))
            m_initialValues.insert(c.id, c.value);
    }
    rebuildControls();
    updateWidgets();
}

void CameraSettingsDialog::restoreCamera()
{
    if (!m_camera)
        return;
    apply(m_camera, Settings());  // exposition et balance des blancs automatiques
    const QByteArray device = m_camera->cameraDevice().id();
    for (auto it = m_settings.controls.cbegin(); it != m_settings.controls.cend(); ++it) {
        if (m_initialValues.contains(it.key()))
            V4l2::setControl(device, it.key(), m_initialValues.value(it.key()));
    }
}

void CameraSettingsDialog::rebuildControls()
{
    // Un widget par réglage : curseur (entier), case à cocher (oui/non), liste (menu).
    while (m_controlsForm->rowCount() > 0)
        m_controlsForm->removeRow(0);
    m_controlWidgets.clear();
    m_controls.clear();
    if (m_camera) {
        for (const V4l2::Control &c : V4l2::listControls(m_camera->cameraDevice().id())) {
            if (!V4l2::isExposureOrWhiteBalance(c.id))
                m_controls.push_back(c);
        }
    }
    for (const V4l2::Control &c : m_controls) {
        const int value = m_settings.controls.value(c.id, c.value);
        const QString tip = V4l2::frenchDescription(c);
        auto changed = [this, id = c.id](int v) {
            m_settings.controls.insert(id, v);
            if (m_camera)
                V4l2::setControl(m_camera->cameraDevice().id(), id, v);
            refreshControlStates();
            emit settingsChanged();
        };
        QWidget *widget = nullptr;
        if (c.type == V4l2::Control::Boolean || (c.type == V4l2::Control::Integer && c.minimum == 0 && c.maximum == 1)) {
            auto *check = new QCheckBox(V4l2::frenchName(c), m_controlsBox);
            check->setChecked(value != 0);
            connect(check, &QCheckBox::toggled, this, [changed](bool on) { changed(on ? 1 : 0); });
            m_controlsForm->addRow(check);
            widget = check;
        } else if (c.type == V4l2::Control::Menu) {
            auto *combo = new QComboBox(m_controlsBox);
            for (const auto &[v, label] : c.menu)
                combo->addItem(V4l2::frenchMenuItem(c, v, label), v);
            combo->setCurrentIndex(std::max(0, combo->findData(value)));
            connect(combo, &QComboBox::currentIndexChanged, this,
                    [combo, changed](int index) { changed(combo->itemData(index).toInt()); });
            m_controlsForm->addRow(V4l2::frenchName(c), combo);
            widget = combo;
        } else {
            auto *slider = new QSlider(Qt::Horizontal, m_controlsBox);
            slider->setRange(c.minimum, c.maximum);
            slider->setSingleStep(c.step);
            slider->setPageStep(std::max(c.step, (c.maximum - c.minimum) / 10));
            slider->setValue(value);
            auto *label = new QLabel(m_controlsBox);
            label->setMinimumWidth(110);
            auto showValue = [label, def = c.defaultValue](int v) {
                label->setText(v == def ? tr("%1 (défaut)").arg(v) : QString::number(v));
            };
            showValue(value);
            connect(slider, &QSlider::valueChanged, this, [changed, showValue](int v) {
                showValue(v);
                changed(v);
            });
            m_controlsForm->addRow(V4l2::frenchName(c), sliderRow(slider, label));
            widget = slider;
        }
        widget->setToolTip(tip);
        m_controlWidgets.insert(c.id, widget);
    }
    m_controlsBox->setVisible(!m_controls.empty());
    refreshControlStates();
}

void CameraSettingsDialog::refreshControlStates()
{
    // Certains réglages n'ont d'effet que selon un autre (mise au point manuelle...) : grisés.
    if (!m_camera)
        return;
    for (const V4l2::Control &c : V4l2::listControls(m_camera->cameraDevice().id())) {
        if (QWidget *w = m_controlWidgets.value(c.id))
            w->setEnabled(!c.inactive);
    }
}

void CameraSettingsDialog::setSettings(const Settings &settings)
{
    m_settings = settings;
    rebuildControls();
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
        // Qt n'y parvient pas avec les webcams limitées au mode « priorité ouverture » (C270).
        V4l2::restoreAutoExposure(camera->cameraDevice().id());
    }
    if (s.manualWhiteBalance && camera->isWhiteBalanceModeSupported(QCamera::WhiteBalanceManual)) {
        camera->setWhiteBalanceMode(QCamera::WhiteBalanceManual);
        camera->setColorTemperature(s.colorTemperature);
    } else if (camera->isWhiteBalanceModeSupported(QCamera::WhiteBalanceAuto)) {
        camera->setWhiteBalanceMode(QCamera::WhiteBalanceAuto);
    }
    const QByteArray device = camera->cameraDevice().id();
    for (auto it = s.controls.cbegin(); it != s.controls.cend(); ++it)
        V4l2::setControl(device, it.key(), it.value());
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
    m_exposureRangeLabel->setText(tr("<small>Plage de la caméra : %1 à %2 ms</small>")
                                      .arg(formatMs(m_minExposureMs), formatMs(m_maxExposureMs)));
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
