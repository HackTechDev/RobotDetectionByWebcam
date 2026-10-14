#pragma once

#include "v4l2controls.h"

#include <QDialog>
#include <QMap>
#include <QPointer>
#include <vector>

class QCamera;
class QComboBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class QSlider;
class QTimer;

// Réglages manuels de la caméra : exposition, balance des blancs et autres réglages du pilote
// (luminosité, contraste, gain, fréquence du secteur...).
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
        QMap<quint32, int> controls;  // autres réglages modifiés : identifiant V4L2 -> valeur
    };

    explicit CameraSettingsDialog(QWidget *parent = nullptr);

    // Caméra à régler (peut changer quand on change de webcam).
    void setCamera(QCamera *camera);
    void setSettings(const Settings &settings);
    Settings settings() const { return m_settings; }

    // Applique les réglages à une caméra active.
    static void apply(QCamera *camera, const Settings &settings);
    // Rend la webcam dans l'état où elle était à son ouverture (exposition et balance des blancs
    // automatiques, autres réglages modifiés remis à leur valeur d'origine) : ces réglages sont
    // stockés dans la webcam et resteraient sinon pour les autres logiciels.
    void restoreCamera();

signals:
    void settingsChanged();

private:
    void updateFromWidgets();
    void updateWidgets();
    void updateReadout();
    void rebuildControls();
    void refreshControlStates();
    double sliderToMs(int value) const;
    int msToSlider(double ms) const;

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
    QLabel *m_exposureRangeLabel = nullptr;
    QGroupBox *m_controlsBox = nullptr;
    QFormLayout *m_controlsForm = nullptr;
    std::vector<V4l2::Control> m_controls;     // autres réglages de la caméra courante
    QMap<quint32, QWidget *> m_controlWidgets;
    QMap<quint32, int> m_initialValues;         // valeurs à l'ouverture de la caméra
    double m_minExposureMs = 0.5;               // limites du temps de pose
    double m_maxExposureMs = 250.0;
    QTimer *m_readoutTimer = nullptr;
};
