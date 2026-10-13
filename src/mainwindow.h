#pragma once

#include "cameracalibration.h"
#include "camerasettingsdialog.h"
#include "floormapper.h"
#include "markerdetector.h"
#include "motionfilter.h"
#include "objecttracker.h"
#include "positionsender.h"
#include "squaredetector.h"

#include <QCameraFormat>
#include <QElapsedTimer>
#include <QMainWindow>

class QCamera;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QMediaCaptureSession;
class QPushButton;
class QShortcut;
class QSlider;
class QSpinBox;
class QTabWidget;
class QVideoFrame;
class QVideoSink;
class VideoWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Fichier des réglages de l'application (format INI).
    static QString settingsPath();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onFrame(const QVideoFrame &frame);
    void onCameraChanged(int index);
    void onResolutionChanged(int index);
    void onImageClicked(const QPoint &displayPos);
    void chooseColor();
    void updateParams();
    void onColorModeChanged();
    void onDictionaryChanged(int index);
    void generateMarker();
    void onBoardChanged();
    void calibrate();
    void generateBoard();
    void applyUdpTarget();
    void toggleFloorPicking();
    void clearFloor();
    void onFloorSizeChanged();
    void toggleTrackSelection();
    void onRectangleSelected(const QRect &displayRect);
    void stopTracking();
    void onSelectionPointClicked(const QPoint &displayPos);
    void cancelCurrentInput();
    void saveEventLog();

private:
    QSlider *addSlider(QFormLayout *form, const QString &label, int min, int max, int value);
    void setTargetColor(const QColor &color);
    void selectColorModeFor(const QColor &color);
    void populateCameras();
    void populateResolutions();
    void applyResolution(int index);
    QString resolutionLabel(int index) const;
    void saveResolution(int index) const;
    void saveCameraSettings() const;
    void loadCameraSettings();
    QString resolutionSettingsKey() const;
    void updateCalibrationStatus();
    QString calibrationPath() const;
    void loadSettings();
    void saveFloorSettings() const;
    void updateFloorStatus();
    void updateTrackStatus();
    void applySmoothingToTracker();
    void logEvents(const std::vector<MotionEvent> &events);
    void saveSettings() const;

    QMediaCaptureSession *m_session = nullptr;
    QCamera *m_camera = nullptr;
    QVideoSink *m_sink = nullptr;

    VideoWidget *m_video = nullptr;
    QComboBox *m_cameraCombo = nullptr;
    QComboBox *m_resolutionCombo = nullptr;
    QPushButton *m_colorButton = nullptr;
    QFormLayout *m_squareForm = nullptr;
    QComboBox *m_colorModeCombo = nullptr;  // 0 = couleur vive (teinte), 1 = blanc, gris ou noir
    QSlider *m_valTolSlider = nullptr;
    QSlider *m_satMaxSlider = nullptr;
    QSlider *m_hueTolSlider = nullptr;
    QSlider *m_satSlider = nullptr;
    QSlider *m_valSlider = nullptr;
    QSlider *m_areaSlider = nullptr;
    QSlider *m_fillSlider = nullptr;
    QCheckBox *m_maskCheck = nullptr;
    QCheckBox *m_mirrorCheck = nullptr;
    CameraSettingsDialog *m_cameraSettingsDialog = nullptr;
    QTabWidget *m_tabs = nullptr;
    QGroupBox *m_squareBox = nullptr;
    QGroupBox *m_arucoBox = nullptr;
    QComboBox *m_dictCombo = nullptr;
    QDoubleSpinBox *m_markerLengthSpin = nullptr;

    QGroupBox *m_captureBox = nullptr;
    QSpinBox *m_boardColsSpin = nullptr;
    QSpinBox *m_boardRowsSpin = nullptr;
    QDoubleSpinBox *m_squareSizeSpin = nullptr;
    QCheckBox *m_autoCaptureCheck = nullptr;
    QPushButton *m_calibrateButton = nullptr;
    QLabel *m_calibLabel = nullptr;

    QGroupBox *m_udpBox = nullptr;
    QLineEdit *m_udpHostEdit = nullptr;
    QSpinBox *m_udpPortSpin = nullptr;
    QLabel *m_udpStatusLabel = nullptr;

    QDoubleSpinBox *m_floorSizeXSpin = nullptr;
    QDoubleSpinBox *m_floorSizeYSpin = nullptr;
    QPushButton *m_floorPickButton = nullptr;
    QCheckBox *m_floorShowCheck = nullptr;
    QLabel *m_floorLabel = nullptr;

    QComboBox *m_trackAlgoCombo = nullptr;
    QPushButton *m_trackSelectButton = nullptr;
    QPushButton *m_trackStartButton = nullptr;
    QDoubleSpinBox *m_motionMmSpin = nullptr;
    QDoubleSpinBox *m_motionPxSpin = nullptr;
    QListWidget *m_eventList = nullptr;
    QShortcut *m_confirmShortcut = nullptr;
    QShortcut *m_confirmShortcutKeypad = nullptr;
    QPushButton *m_trackStopButton = nullptr;
    QCheckBox *m_trackTrailCheck = nullptr;
    QDoubleSpinBox *m_trackTrailSpin = nullptr;
    QLabel *m_trackLabel = nullptr;

    QGroupBox *m_smoothBox = nullptr;
    QSlider *m_smoothStrengthSlider = nullptr;
    QDoubleSpinBox *m_smoothHoldSpin = nullptr;
    QCheckBox *m_smoothTrailCheck = nullptr;
    QDoubleSpinBox *m_smoothTrailSpin = nullptr;
    QLabel *m_statusLabel = nullptr;

    SquareDetector m_detector;
    MarkerDetector m_markerDetector;
    CameraCalibration m_calibration;
    PositionSender m_sender;
    FloorMapper m_floor;
    ObjectTracker m_tracker;
    MotionSmoother m_smoother;
    QSize m_smoothImageSize;   // résolution des positions lissées
    QSize m_trackImageSize;    // résolution à laquelle l'objet a été sélectionné
    QElapsedTimer m_clock;     // horloge des points de trajectoire
    bool m_floorPicking = false;
    std::vector<QPointF> m_floorPicked;  // points cliqués, dans l'image réelle de la caméra
    QSize m_lastFrameSizeAtPick;
    QElapsedTimer m_lastCapture;
    bool m_captureRequested = false;
    QColor m_targetColor;
    QImage m_lastFrame;
    // Une entrée par résolution de m_resolutionCombo.
    struct ResolutionChoice
    {
        QCameraFormat fast;     // format le plus rapide (souvent MJPEG)
        QCameraFormat raw;      // meilleur format non compressé (peut être nul)
        bool useRaw = false;    // le format rapide ne respecte pas la résolution
        const QCameraFormat &current() const { return useRaw ? raw : fast; }
    };
    QList<ResolutionChoice> m_resolutions;
    int m_pendingResolution = -1;  // résolution demandée, en attente de vérification
    bool m_resolutionSynced = false;
    QElapsedTimer m_fpsTimer;
    int m_frameCount = 0;
    double m_fps = 0.0;
};
