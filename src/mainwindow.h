#pragma once

#include "cameracalibration.h"
#include "markerdetector.h"
#include "squaredetector.h"

#include <QElapsedTimer>
#include <QMainWindow>

class QCamera;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class QMediaCaptureSession;
class QPushButton;
class QSlider;
class QSpinBox;
class QVideoFrame;
class QVideoSink;
class VideoWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onFrame(const QVideoFrame &frame);
    void onCameraChanged(int index);
    void onImageClicked(const QPoint &pos);
    void chooseColor();
    void updateParams();
    void onDictionaryChanged(int index);
    void generateMarker();
    void onBoardChanged();
    void calibrate();
    void generateBoard();

private:
    QSlider *addSlider(QFormLayout *form, const QString &label, int min, int max, int value);
    void setTargetColor(const QColor &color);
    void populateCameras();
    void updateCalibrationStatus();
    QString calibrationPath() const;

    QMediaCaptureSession *m_session = nullptr;
    QCamera *m_camera = nullptr;
    QVideoSink *m_sink = nullptr;

    VideoWidget *m_video = nullptr;
    QComboBox *m_cameraCombo = nullptr;
    QPushButton *m_colorButton = nullptr;
    QSlider *m_hueTolSlider = nullptr;
    QSlider *m_satSlider = nullptr;
    QSlider *m_valSlider = nullptr;
    QSlider *m_areaSlider = nullptr;
    QSlider *m_fillSlider = nullptr;
    QCheckBox *m_maskCheck = nullptr;
    QCheckBox *m_mirrorCheck = nullptr;
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
    QLabel *m_statusLabel = nullptr;

    SquareDetector m_detector;
    MarkerDetector m_markerDetector;
    CameraCalibration m_calibration;
    QElapsedTimer m_lastCapture;
    bool m_captureRequested = false;
    QColor m_targetColor;
    QImage m_lastFrame;
    QElapsedTimer m_fpsTimer;
    int m_frameCount = 0;
    double m_fps = 0.0;
};
