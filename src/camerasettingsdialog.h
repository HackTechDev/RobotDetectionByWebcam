#pragma once

#include <QDialog>
#include <QPointer>

class QCamera;
class QComboBox;
class QLabel;
class QSlider;
class QTimer;

// Réglages manuels de la caméra : exposition et balance des blancs.
// Les possibilités dépendent de la webcam ; les réglages non pris en charge sont grisés.
class CameraSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    struct Settings
    {
        bool manualExposure = false;
        double exposureMs = 33.0;  // temps de pose en mode manuel
        bool manualWhiteBalance = false;
        int colorTemperature = 4500;  // en kelvins, en mode manuel
    };

    explicit CameraSettingsDialog(QWidget *parent = nullptr);

    // Caméra à régler (peut changer quand on change de webcam).
    void setCamera(QCamera *camera);
    void setSettings(const Settings &settings);
    Settings settings() const { return m_settings; }

    // Applique les réglages à une caméra active.
    static void apply(QCamera *camera, const Settings &settings);

signals:
    void settingsChanged();

private:
    void updateFromWidgets();
    void updateWidgets();
    void updateReadout();

    QPointer<QCamera> m_camera;
    Settings m_settings;

    QComboBox *m_exposureModeCombo = nullptr;
    QSlider *m_exposureSlider = nullptr;
    QLabel *m_exposureValueLabel = nullptr;
    QLabel *m_exposureReadout = nullptr;
    QComboBox *m_whiteBalanceModeCombo = nullptr;
    QSlider *m_temperatureSlider = nullptr;
    QLabel *m_temperatureValueLabel = nullptr;
    QLabel *m_supportLabel = nullptr;
    QTimer *m_readoutTimer = nullptr;
};
