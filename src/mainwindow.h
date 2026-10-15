#pragma once

#include "cameracalibration.h"
#include "camerasettingsdialog.h"
#include "floormapper.h"
#include "markerdetector.h"
#include "motionfilter.h"
#include "objecttracker.h"
#include "positionsender.h"
#include "squaredetector.h"
#include "zonemanager.h"

#include <QCameraFormat>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QThreadPool>
#include <QMainWindow>

#include <memory>
#include <vector>

class QCamera;
class QCameraDevice;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QMediaCaptureSession;
class QMediaRecorder;
class QToolButton;
class QVideoFrameInput;
class QAction;
class QScreen;
class PlayerWindow;
class QPushButton;
class QShortcut;
class QSlider;
class QSpinBox;
class QTabWidget;
class QVideoFrame;
class QVideoSink;
class VideoWidget;

// Réglages lus dans l'interface au lancement du traitement d'une image (le fil de traitement
// ne doit pas accéder aux widgets).
struct ProcessingOptions
{
    bool squaresOn = false;
    bool markersOn = false;
    bool smoothingOn = false;
    bool calibOn = false;
    bool captureNow = false;  // capture du damier demandée (bouton ou capture automatique)
    qint64 timeMs = 0;        // horloge du suivi au moment de l'image
};

// Résultat du traitement d'une image, produit par le fil de traitement.
struct FrameResult
{
    QImage image;
    ProcessingOptions options;
    std::vector<DetectedSquare> squares;
    std::vector<DetectedMarker> markers;
    std::vector<MotionEvent> events;
    bool trackSizeChanged = false;   // résolution différente de celle des objets suivis
    bool trackStateChanged = false;  // un objet suivi a été perdu ou retrouvé
    std::vector<QPointF> boardCorners;
    bool boardFound = false;
    bool captured = false;
};

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
    void toggleZonePicking();
    void removeSelectedZone();
    void clearZones();
    void onRectangleSelected(const QRect &displayRect);
    void stopTracking();
    void onSelectionPointClicked(const QPoint &displayPos);
    void cancelCurrentInput();
    void saveEventLog();
    // Calibration et repère du sol de la caméra identifiée par « key » (slot : invocable par les tests).
    void loadCameraReference(const QString &key);
    void onProcessingFinished();
    void removeSelectedObject();
    void onObjectRenamed(QListWidgetItem *item);

private:
    // Capture : enregistre l'image de la webcam (ou l'image affichée, avec les détections) en PNG.
    void saveCapture(bool withDetections);
    QString captureFolder() const;
    // Vidéo : flux de la webcam (enregistré par Qt Multimedia) ou images affichées, avec les
    // détections (envoyées une à une à un second enregistreur).
    void startVideo(bool withDetections);
    // Fenêtre du joueur : la vidéo seule, sur un second écran.
    void setPlayerWindowOpen(bool open);
    QScreen *playerScreen() const;
    void stopVideo();
    void toggleVideo(bool withDetections);
    void updateVideoButton();
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
    static QString cameraKeyFor(const QCameraDevice &device);
    void migrateLegacyReference();
    void migrateCameraKey(const QCameraDevice &device);
    void updateCalibrationStatus();
    QString calibrationPath() const;
    void loadSettings();
    void saveFloorSettings() const;
    void updateFloorStatus();
    void updateTrackStatus();
    void updateZoneStatus();
    void updateZoneList();
    void saveZones() const;
    void addZone(const QRect &displayRect);
    void applySmoothingToTracker();
    void configureTracker(ObjectTracker &tracker) const;
    void onTrackAlgorithmChosen(int index);
    bool ensureTrackerModels(int algorithm);
    void logEvents(const std::vector<MotionEvent> &events);
    void saveSettings() const;
    // Traitement d'une image dans le fil de traitement, puis affichage dans le fil principal.
    void startProcessing(const QImage &image);
    FrameResult processFrame(const QImage &image, const ProcessingOptions &options);
    void presentFrame(const FrameResult &result);
    // Attend la fin du traitement en cours : à appeler avant de modifier un objet utilisé par le
    // fil de traitement (détecteurs, calibration, repère du sol, objets suivis, lissage).
    void waitForProcessing() const;

    QMediaCaptureSession *m_session = nullptr;
    QCamera *m_camera = nullptr;
    QString m_cameraKey;  // identifiant de la caméra dans les réglages (calibration, repère du sol...)
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
    QDoubleSpinBox *m_floorHeightSpin = nullptr;
    QLabel *m_floorLabel = nullptr;

    QComboBox *m_trackAlgoCombo = nullptr;

    int m_trackAlgoIndex = 0;  // méthode en vigueur (rétablie si un téléchargement échoue)
    QPushButton *m_trackSelectButton = nullptr;
    QPushButton *m_trackStartButton = nullptr;
    QPushButton *m_trackRemoveButton = nullptr;
    QListWidget *m_objectList = nullptr;
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
    // Objets suivis, chacun avec son propre tracker, son nom et sa couleur.
    struct TrackedItem
    {
        int id = 0;
        QString name;
        QColor color;
        std::unique_ptr<ObjectTracker> tracker;
    };
    std::vector<TrackedItem> m_tracked;
    int m_nextTrackId = 1;
    MotionSmoother m_smoother;
    QSize m_smoothImageSize;   // résolution des positions lissées
    QSize m_trackImageSize;    // résolution à laquelle les objets ont été sélectionnés
    QElapsedTimer m_clock;     // horloge des points de trajectoire
    bool m_floorPicking = false;
    bool m_zonePicking = false;  // tracé d'une zone en cours (sélection d'un rectangle)
    ZoneManager m_zones;
    QPushButton *m_zoneAddButton = nullptr;
    QPushButton *m_zoneConfirmButton = nullptr;
    QPushButton *m_zoneRemoveButton = nullptr;
    QPushButton *m_zoneClearButton = nullptr;
    QListWidget *m_zoneList = nullptr;
    QCheckBox *m_zoneShowCheck = nullptr;
    QLabel *m_zoneLabel = nullptr;
    QLabel *m_zoneOccupancyLabel = nullptr;
    std::vector<QPointF> m_floorPicked;  // points cliqués, dans l'image réelle de la caméra
    QSize m_lastFrameSizeAtPick;
    bool m_floorPoseShown = false;  // état de la position de la caméra affiché dans l'onglet Sol
    QElapsedTimer m_lastCapture;
    bool m_captureRequested = false;
    QColor m_targetColor;
    QImage m_lastFrame;  // dernière image affichée (les clics portent sur elle)
    QToolButton *m_videoButton = nullptr;
    PlayerWindow *m_playerWindow = nullptr;
    QToolButton *m_playerButton = nullptr;
    QAction *m_playerDetectionsAction = nullptr;
    QAction *m_playerFlipAction = nullptr;
    QAction *m_playerFullScreenAction = nullptr;
    QString m_playerScreenName;  // écran choisi pour la fenêtre du joueur (vide : automatique)
    QMediaRecorder *m_recorder = nullptr;         // vidéo de la webcam
    QMediaCaptureSession *m_overlaySession = nullptr;  // vidéo avec les détections
    QVideoFrameInput *m_frameInput = nullptr;
    QMediaRecorder *m_overlayRecorder = nullptr;
    QElapsedTimer m_overlayClock;                 // horodatage des images envoyées
    QString m_videoPath;
    bool m_videoStopping = false;  // arrêt demandé, fichier en cours de finalisation
    QImage m_lastDisplay;  // même image, telle qu'affichée (détections dessinées, miroir)
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
    QFutureWatcher<FrameResult> m_watcher;
    // Fils de calcul pour les tâches parallèles d'une image (détection des marqueurs, suivi de
    // chaque objet) ; distincts de ceux qui exécutent processFrame, pour éviter tout blocage.
    QThreadPool m_innerPool;
    QImage m_pendingImage;  // image arrivée pendant un traitement (seule la plus récente est gardée)
    QElapsedTimer m_fpsTimer;
    int m_frameCount = 0;
    double m_fps = 0.0;
};
