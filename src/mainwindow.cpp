#include "mainwindow.h"
#include "videowidget.h"

#include <QApplication>
#include <QCamera>
#include <QCameraDevice>
#include <QCameraFormat>
#include <QCheckBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoSink>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_session(new QMediaCaptureSession(this))
    , m_sink(new QVideoSink(this))
{
    setWindowTitle(tr("Détection de carré coloré et de marqueurs ArUco"));
    m_markerDetector.setCalibration(&m_calibration);

    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);

    m_video = new VideoWidget(central);
    layout->addWidget(m_video, 1);

    auto *panel = new QVBoxLayout;
    layout->addLayout(panel);

    auto *camBox = new QGroupBox(tr("Webcam"), central);
    auto *camForm = new QFormLayout(camBox);
    m_cameraCombo = new QComboBox(camBox);
    camForm->addRow(tr("Caméra"), m_cameraCombo);
    m_resolutionCombo = new QComboBox(camBox);
    m_resolutionCombo->setToolTip(tr("Une résolution plus basse accélère la détection.\n"
                                     "La calibration n'est valable que pour le même format (4:3, 16:9...)."));
    camForm->addRow(tr("Résolution"), m_resolutionCombo);
    m_mirrorCheck = new QCheckBox(tr("Effet miroir"), camBox);
    camForm->addRow(m_mirrorCheck);
    panel->addWidget(camBox);

    auto *tabs = new QTabWidget(central);
    panel->addWidget(tabs);

    auto *detBox = new QGroupBox(tr("Activer la détection"), tabs);
    detBox->setCheckable(true);
    detBox->setChecked(true);
    m_squareBox = detBox;
    auto *detForm = new QFormLayout(detBox);
    m_colorButton = new QPushButton(detBox);
    m_colorButton->setMinimumHeight(28);
    detForm->addRow(tr("Couleur"), m_colorButton);
    auto *hint = new QLabel(tr("<i>Astuce : cliquez sur l'image pour<br>choisir la couleur du carré.</i>"), detBox);
    detForm->addRow(hint);

    const SquareDetector::Params defaults;
    m_hueTolSlider = addSlider(detForm, tr("Tolérance teinte"), 1, 90, defaults.hueTolerance);
    m_satSlider = addSlider(detForm, tr("Saturation min"), 0, 255, defaults.satMin);
    m_valSlider = addSlider(detForm, tr("Luminosité min"), 0, 255, defaults.valMin);
    m_areaSlider = addSlider(detForm, tr("Surface min"), 10, 5000, defaults.minArea);
    m_fillSlider = addSlider(detForm, tr("Remplissage min (%)"), 50, 100, int(defaults.minFill * 100));
    m_maskCheck = new QCheckBox(tr("Afficher le masque"), detBox);
    detForm->addRow(m_maskCheck);
    tabs->addTab(detBox, tr("Carré coloré"));

    m_arucoBox = new QGroupBox(tr("Activer la détection"), tabs);
    m_arucoBox->setCheckable(true);
    m_arucoBox->setChecked(true);
    auto *arucoForm = new QFormLayout(m_arucoBox);
    m_dictCombo = new QComboBox(m_arucoBox);
    m_dictCombo->addItems(MarkerDetector::dictionaryNames());
    arucoForm->addRow(tr("Dictionnaire"), m_dictCombo);
    m_markerLengthSpin = new QDoubleSpinBox(m_arucoBox);
    m_markerLengthSpin->setRange(1, 2000);
    m_markerLengthSpin->setDecimals(1);
    m_markerLengthSpin->setSuffix(QStringLiteral(" mm"));
    m_markerLengthSpin->setValue(m_markerDetector.markerLength());
    m_markerLengthSpin->setToolTip(tr("Côté de la partie noire du marqueur imprimé, sans la marge blanche"));
    arucoForm->addRow(tr("Taille du marqueur"), m_markerLengthSpin);
    auto *generateButton = new QPushButton(tr("Générer un marqueur..."), m_arucoBox);
    arucoForm->addRow(generateButton);
    tabs->addTab(m_arucoBox, tr("ArUco"));

    // Onglet calibration : damier, captures, calcul.
    auto *calibTab = new QWidget(tabs);
    auto *calibLayout = new QVBoxLayout(calibTab);
    auto *boardBox = new QGroupBox(tr("Damier"), calibTab);
    auto *boardForm = new QFormLayout(boardBox);
    const CameraCalibration::Board board;
    m_boardColsSpin = new QSpinBox(boardBox);
    m_boardColsSpin->setRange(3, 30);
    m_boardColsSpin->setValue(board.cols);
    m_boardRowsSpin = new QSpinBox(boardBox);
    m_boardRowsSpin->setRange(3, 30);
    m_boardRowsSpin->setValue(board.rows);
    m_squareSizeSpin = new QDoubleSpinBox(boardBox);
    m_squareSizeSpin->setRange(1, 500);
    m_squareSizeSpin->setDecimals(1);
    m_squareSizeSpin->setSuffix(QStringLiteral(" mm"));
    m_squareSizeSpin->setValue(board.squareSize);
    boardForm->addRow(tr("Coins intérieurs (largeur)"), m_boardColsSpin);
    boardForm->addRow(tr("Coins intérieurs (hauteur)"), m_boardRowsSpin);
    boardForm->addRow(tr("Côté d'une case"), m_squareSizeSpin);
    auto *generateBoardButton = new QPushButton(tr("Générer un damier..."), boardBox);
    boardForm->addRow(generateBoardButton);
    calibLayout->addWidget(boardBox);

    m_captureBox = new QGroupBox(tr("Mode calibration"), calibTab);
    m_captureBox->setCheckable(true);
    m_captureBox->setChecked(false);
    auto *captureLayout = new QVBoxLayout(m_captureBox);
    auto *captureButton = new QPushButton(tr("Capturer"), m_captureBox);
    m_autoCaptureCheck = new QCheckBox(tr("Capture automatique (toutes les 2 s)"), m_captureBox);
    auto *clearButton = new QPushButton(tr("Effacer les captures"), m_captureBox);
    m_calibrateButton = new QPushButton(tr("Calibrer"), m_captureBox);
    captureLayout->addWidget(captureButton);
    captureLayout->addWidget(m_autoCaptureCheck);
    captureLayout->addWidget(clearButton);
    captureLayout->addWidget(m_calibrateButton);
    calibLayout->addWidget(m_captureBox);

    m_calibLabel = new QLabel(calibTab);
    m_calibLabel->setWordWrap(true);
    m_calibLabel->setTextFormat(Qt::RichText);
    calibLayout->addWidget(m_calibLabel);
    calibLayout->addStretch();
    tabs->addTab(calibTab, tr("Calibration"));

    auto *resBox = new QGroupBox(tr("Résultat"), central);
    auto *resLayout = new QVBoxLayout(resBox);
    m_statusLabel = new QLabel(resBox);
    m_statusLabel->setTextFormat(Qt::RichText);
    m_statusLabel->setMinimumWidth(240);
    resLayout->addWidget(m_statusLabel);
    panel->addWidget(resBox);
    panel->addStretch();

    setCentralWidget(central);
    resize(1100, 650);

    connect(m_colorButton, &QPushButton::clicked, this, &MainWindow::chooseColor);
    connect(m_video, &VideoWidget::imageClicked, this, &MainWindow::onImageClicked);
    connect(m_sink, &QVideoSink::videoFrameChanged, this, &MainWindow::onFrame);
    connect(m_cameraCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onCameraChanged);
    connect(m_resolutionCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onResolutionChanged);
    connect(m_dictCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onDictionaryChanged);
    connect(generateButton, &QPushButton::clicked, this, &MainWindow::generateMarker);
    connect(m_markerLengthSpin, &QDoubleSpinBox::valueChanged, this,
            [this](double v) { m_markerDetector.setMarkerLength(v); });
    connect(m_boardColsSpin, &QSpinBox::valueChanged, this, &MainWindow::onBoardChanged);
    connect(m_boardRowsSpin, &QSpinBox::valueChanged, this, &MainWindow::onBoardChanged);
    connect(m_squareSizeSpin, &QDoubleSpinBox::valueChanged, this, &MainWindow::onBoardChanged);
    connect(generateBoardButton, &QPushButton::clicked, this, &MainWindow::generateBoard);
    connect(captureButton, &QPushButton::clicked, this, [this] { m_captureRequested = true; });
    connect(clearButton, &QPushButton::clicked, this, [this] {
        m_calibration.clearCaptures();
        updateCalibrationStatus();
    });
    connect(m_calibrateButton, &QPushButton::clicked, this, &MainWindow::calibrate);

    m_calibration.load(calibrationPath());
    m_lastCapture.start();
    updateCalibrationStatus();

    setTargetColor(QColor(220, 30, 30));
    m_session->setVideoSink(m_sink);
    m_fpsTimer.start();
    populateCameras();
}

MainWindow::~MainWindow() = default;

QSlider *MainWindow::addSlider(QFormLayout *form, const QString &label, int min, int max, int value)
{
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    auto *slider = new QSlider(Qt::Horizontal, row);
    slider->setRange(min, max);
    slider->setValue(value);
    auto *valueLabel = new QLabel(QString::number(value), row);
    valueLabel->setMinimumWidth(36);
    h->addWidget(slider);
    h->addWidget(valueLabel);
    form->addRow(label, row);
    connect(slider, &QSlider::valueChanged, valueLabel, qOverload<int>(&QLabel::setNum));
    connect(slider, &QSlider::valueChanged, this, &MainWindow::updateParams);
    return slider;
}

void MainWindow::populateCameras()
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        m_statusLabel->setText(tr("<b style='color:red'>Aucune webcam détectée.</b>"));
        return;
    }
    const QCameraDevice def = QMediaDevices::defaultVideoInput();
    int defIndex = 0;
    for (int i = 0; i < cameras.size(); ++i) {
        m_cameraCombo->addItem(cameras[i].description(), QVariant::fromValue(cameras[i]));
        if (cameras[i] == def)
            defIndex = i;
    }
    m_cameraCombo->setCurrentIndex(defIndex);
    onCameraChanged(defIndex);
}

void MainWindow::onCameraChanged(int index)
{
    if (index < 0)
        return;
    const auto device = m_cameraCombo->itemData(index).value<QCameraDevice>();
    if (m_camera && m_camera->cameraDevice() == device)
        return;

    delete m_camera;
    m_camera = new QCamera(device, this);
    connect(m_camera, &QCamera::errorOccurred, this, [this](QCamera::Error, const QString &msg) {
        m_statusLabel->setText(tr("<b style='color:red'>Erreur caméra :</b> %1").arg(msg.toHtmlEscaped()));
    });
    m_session->setCamera(m_camera);
    populateResolutions();
    m_camera->start();
}

namespace {

QString aspectName(const QSize &size)
{
    const double r = double(size.width()) / size.height();
    const struct { double ratio; const char *name; } known[] = {
        {4.0 / 3, "4:3"}, {16.0 / 9, "16:9"}, {16.0 / 10, "16:10"}, {5.0 / 4, "5:4"}, {11.0 / 9, "11:9"}};
    for (const auto &k : known)
        if (std::abs(r - k.ratio) / k.ratio < 0.01)
            return QString::fromLatin1(k.name);
    return QString::number(r, 'f', 2) + QStringLiteral(":1");
}

} // namespace

void MainWindow::populateResolutions()
{
    // Pour chaque résolution : le format le plus rapide (à vitesse égale, non compressé pour
    // éviter le décodage JPEG) et, en secours, le meilleur format non compressé.
    QList<ResolutionChoice> choices;
    for (const QCameraFormat &f : m_camera->cameraDevice().videoFormats()) {
        auto it = std::find_if(choices.begin(), choices.end(), [&](const ResolutionChoice &c) {
            return c.fast.resolution() == f.resolution();
        });
        const bool uncompressed = f.pixelFormat() != QVideoFrameFormat::Format_Jpeg;
        if (it == choices.end()) {
            ResolutionChoice c;
            c.fast = f;
            if (uncompressed)
                c.raw = f;
            choices.append(c);
            continue;
        }
        const bool faster = f.maxFrameRate() > it->fast.maxFrameRate() + 0.1f;
        const bool sameSpeed = std::abs(f.maxFrameRate() - it->fast.maxFrameRate()) <= 0.1f;
        if (faster || (sameSpeed && uncompressed))
            it->fast = f;
        if (uncompressed && (it->raw.isNull() || f.maxFrameRate() > it->raw.maxFrameRate()))
            it->raw = f;
    }
    std::sort(choices.begin(), choices.end(), [](const ResolutionChoice &a, const ResolutionChoice &b) {
        const QSize ra = a.fast.resolution(), rb = b.fast.resolution();
        return ra.width() * ra.height() > rb.width() * rb.height();
    });

    const QSignalBlocker blocker(m_resolutionCombo);
    m_resolutionCombo->clear();
    m_resolutions = choices;
    for (int i = 0; i < m_resolutions.size(); ++i)
        m_resolutionCombo->addItem(resolutionLabel(i));
    // Sans choix explicite, la caméra garde son format par défaut ; la liste se cale
    // sur la première image reçue.
    m_resolutionCombo->setCurrentIndex(-1);
    m_resolutionSynced = false;
    m_pendingResolution = -1;

    // Résolution mémorisée pour ce modèle de caméra, appliquée avant le démarrage.
    const QSettings settings(settingsPath(), QSettings::IniFormat);
    const QString key = resolutionSettingsKey();
    const QSize saved = settings.value(key + QStringLiteral("/size")).toSize();
    for (int i = 0; i < m_resolutions.size(); ++i) {
        if (m_resolutions[i].fast.resolution() != saved)
            continue;
        ResolutionChoice &choice = m_resolutions[i];
        choice.useRaw = settings.value(key + QStringLiteral("/uncompressed")).toBool() && !choice.raw.isNull();
        m_resolutionCombo->setItemText(i, resolutionLabel(i));
        m_resolutionCombo->setCurrentIndex(i);
        m_resolutionSynced = true;
        m_pendingResolution = i;
        m_camera->setCameraFormat(choice.current());
        break;
    }
}

QString MainWindow::settingsPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/settings.ini");
}

QString MainWindow::resolutionSettingsKey() const
{
    // Une entrée par modèle de caméra ; « / » sépare les groupes dans QSettings.
    QString name = m_camera->cameraDevice().description();
    name.replace(QLatin1Char('/'), QLatin1Char('_')).replace(QLatin1Char('\\'), QLatin1Char('_'));
    return QStringLiteral("cameras/") + name;
}

void MainWindow::saveResolution(int index) const
{
    const ResolutionChoice &choice = m_resolutions[index];
    QSettings settings(settingsPath(), QSettings::IniFormat);
    const QString key = resolutionSettingsKey();
    settings.setValue(key + QStringLiteral("/size"), choice.fast.resolution());
    settings.setValue(key + QStringLiteral("/uncompressed"), choice.useRaw);
}

QString MainWindow::resolutionLabel(int index) const
{
    const QCameraFormat &f = m_resolutions[index].current();
    const QSize r = f.resolution();
    return tr("%1 x %2 (%3) — %4 i/s")
        .arg(r.width())
        .arg(r.height())
        .arg(aspectName(r))
        .arg(double(f.maxFrameRate()), 0, 'g', 3);
}

void MainWindow::onResolutionChanged(int index)
{
    if (!m_camera || index < 0 || index >= m_resolutions.size())
        return;
    m_resolutionSynced = true;
    saveResolution(index);
    applyResolution(index);
}

void MainWindow::applyResolution(int index)
{
    m_pendingResolution = index;
    m_camera->stop();
    m_camera->setCameraFormat(m_resolutions[index].current());
    m_camera->start();
}

void MainWindow::setTargetColor(const QColor &color)
{
    m_targetColor = color;
    m_colorButton->setText(color.name());
    m_colorButton->setStyleSheet(QStringLiteral("background-color: %1; color: %2;")
                                     .arg(color.name(), color.lightness() > 128 ? "black" : "white"));
    updateParams();
}

void MainWindow::chooseColor()
{
    const QColor c = QColorDialog::getColor(m_targetColor, this, tr("Couleur du carré"));
    if (c.isValid())
        setTargetColor(c);
}

void MainWindow::onImageClicked(const QPoint &displayPos)
{
    if (m_lastFrame.isNull())
        return;
    // Le clic est fait sur l'image affichée, éventuellement en miroir.
    const QPoint pos = m_mirrorCheck->isChecked()
                           ? QPoint(m_lastFrame.width() - 1 - displayPos.x(), displayPos.y())
                           : displayPos;
    // Moyenne sur une zone 7x7 autour du clic pour limiter le bruit.
    int r = 0, g = 0, b = 0, n = 0;
    for (int dy = -3; dy <= 3; ++dy) {
        for (int dx = -3; dx <= 3; ++dx) {
            const QPoint p = pos + QPoint(dx, dy);
            if (!m_lastFrame.rect().contains(p))
                continue;
            const QRgb px = m_lastFrame.pixel(p);
            r += qRed(px);
            g += qGreen(px);
            b += qBlue(px);
            ++n;
        }
    }
    if (n == 0)
        return;
    const QColor c(r / n, g / n, b / n);
    setTargetColor(c);

    // Ajuste les seuils de saturation/luminosité sous la couleur cliquée.
    m_satSlider->setValue(std::max(30, c.hsvSaturation() * 6 / 10));
    m_valSlider->setValue(std::max(30, c.value() * 5 / 10));
}

void MainWindow::updateParams()
{
    if (!m_hueTolSlider || !m_fillSlider)
        return;
    SquareDetector::Params p = m_detector.params();
    p.hue = std::max(0, m_targetColor.hsvHue());
    p.hueTolerance = m_hueTolSlider->value();
    p.satMin = m_satSlider->value();
    p.valMin = m_valSlider->value();
    p.minArea = m_areaSlider->value();
    p.minFill = m_fillSlider->value() / 100.0;
    m_detector.setParams(p);
}

void MainWindow::onDictionaryChanged(int index)
{
    m_markerDetector.setDictionary(index);
}

void MainWindow::generateMarker()
{
    const int maxId = m_markerDetector.dictionarySize() - 1;
    bool ok = false;
    const int id = QInputDialog::getInt(this, tr("Générer un marqueur"),
                                        tr("Identifiant du marqueur (0 à %1) :").arg(maxId),
                                        0, 0, maxId, 1, &ok);
    if (!ok)
        return;

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Enregistrer le marqueur"),
        QStringLiteral("aruco_%1_%2.png").arg(m_dictCombo->currentText().section(' ', 0, 0)).arg(id),
        tr("Images PNG (*.png)"));
    if (path.isEmpty())
        return;

    if (!m_markerDetector.generateMarker(id, 600).save(path))
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'enregistrer %1").arg(path));
}

QString MainWindow::calibrationPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/calibration.yml");
}

void MainWindow::updateCalibrationStatus()
{
    const int n = m_calibration.captureCount();
    QString text = tr("Captures : <b>%1</b> (au moins 5, idéalement 15 à 25)").arg(n);
    if (m_calibration.isCalibrated()) {
        const QSize size = m_calibration.imageSize();
        text += tr("<br><b style='color:#080'>Caméra calibrée</b> (%1 x %2), erreur RMS %3 px"
                   "<br><small>%4</small>")
                    .arg(size.width())
                    .arg(size.height())
                    .arg(m_calibration.rmsError(), 0, 'f', 3)
                    .arg(calibrationPath().toHtmlEscaped());
    } else {
        text += tr("<br><b style='color:#c00'>Caméra non calibrée</b> : pas de pose 3D des marqueurs.");
    }
    m_calibLabel->setText(text);
    m_calibrateButton->setEnabled(n >= 5);
}

void MainWindow::onBoardChanged()
{
    CameraCalibration::Board board;
    board.cols = m_boardColsSpin->value();
    board.rows = m_boardRowsSpin->value();
    board.squareSize = m_squareSizeSpin->value();
    m_calibration.setBoard(board);
    updateCalibrationStatus();
}

void MainWindow::calibrate()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString error;
    const bool ok = m_calibration.calibrate(&error);
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::warning(this, tr("Calibration"), tr("La calibration a échoué :\n%1").arg(error));
        return;
    }
    const bool saved = m_calibration.save(calibrationPath());
    updateCalibrationStatus();

    QString msg = tr("Calibration réussie.\nErreur de reprojection RMS : %1 px")
                      .arg(m_calibration.rmsError(), 0, 'f', 3);
    if (m_calibration.rmsError() > 1.0)
        msg += tr("\n\nL'erreur est élevée (> 1 px) : refaites des captures plus nettes, "
                  "avec le damier bien à plat et vu sous des angles variés.");
    msg += saved ? tr("\n\nEnregistrée dans %1").arg(calibrationPath())
                 : tr("\n\nImpossible d'enregistrer %1").arg(calibrationPath());
    QMessageBox::information(this, tr("Calibration"), msg);
}

void MainWindow::generateBoard()
{
    const CameraCalibration::Board &board = m_calibration.board();
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Enregistrer le damier"),
        QStringLiteral("damier_%1x%2.png").arg(board.cols).arg(board.rows), tr("Images PNG (*.png)"));
    if (path.isEmpty())
        return;
    if (!CameraCalibration::generateBoard(board, 100).save(path))
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'enregistrer %1").arg(path));
}

void MainWindow::onFrame(const QVideoFrame &frame)
{
    QImage image = frame.toImage();
    if (image.isNull())
        return;
    // Les détections se font toujours sur l'image réelle de la caméra : un marqueur
    // ArUco ou un damier vu en miroir n'est pas reconnu, et les mesures doivent rester
    // dans le repère de la caméra. Le miroir ne s'applique qu'à l'affichage.
    image = image.convertToFormat(QImage::Format_RGB32);
    m_lastFrame = image;

    if (!m_resolutionSynced) {
        m_resolutionSynced = true;
        for (int i = 0; i < m_resolutions.size(); ++i) {
            if (m_resolutions[i].current().resolution() == image.size()) {
                const QSignalBlocker blocker(m_resolutionCombo);
                m_resolutionCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    // Certaines webcams envoient en MJPEG leur résolution maximale quelle que soit la
    // résolution demandée (l'image annonce la bonne taille, mais le JPEG décodé non).
    // Dans ce cas, on bascule sur le format non compressé de la même résolution.
    if (m_pendingResolution >= 0 && m_pendingResolution < m_resolutions.size()) {
        ResolutionChoice &choice = m_resolutions[m_pendingResolution];
        const QSize wanted = choice.current().resolution();
        if (frame.size() == wanted) {
            const int index = m_pendingResolution;
            m_pendingResolution = -1;
            if (image.size() != wanted && !choice.useRaw && !choice.raw.isNull()) {
                choice.useRaw = true;
                m_resolutionCombo->setItemText(index, resolutionLabel(index));
                saveResolution(index);
                QTimer::singleShot(0, this, [this, index] { applyResolution(index); });
                return;
            }
        }
    }

    const bool squaresOn = m_squareBox->isChecked();
    const bool markersOn = m_arucoBox->isChecked();
    const std::vector<DetectedSquare> squares =
        squaresOn ? m_detector.detect(image) : std::vector<DetectedSquare>();
    const std::vector<DetectedMarker> markers =
        markersOn ? m_markerDetector.detect(image) : std::vector<DetectedMarker>();

    // Mode calibration : recherche du damier et capture manuelle ou automatique.
    const bool calibOn = m_captureBox->isChecked();
    std::vector<QPointF> boardCorners;
    bool boardFound = false;
    bool captured = false;
    if (calibOn) {
        boardFound = m_calibration.findBoard(image, boardCorners);
        const bool autoDue = m_autoCaptureCheck->isChecked() && m_lastCapture.elapsed() > 2000;
        if (boardFound && (m_captureRequested || autoDue)) {
            m_calibration.addCapture(image, boardCorners);
            m_lastCapture.restart();
            captured = true;
            updateCalibrationStatus();
        }
    }
    m_captureRequested = false;

    ++m_frameCount;
    if (m_fpsTimer.elapsed() >= 1000) {
        m_fps = m_frameCount * 1000.0 / m_fpsTimer.restart();
        m_frameCount = 0;
    }

    QImage display = (squaresOn && m_maskCheck->isChecked())
                         ? m_detector.lastMask()
                               .scaled(image.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation)
                               .convertToFormat(QImage::Format_RGB32)
                         : image;

    const bool mirror = m_mirrorCheck->isChecked();
    if (mirror)
        display = display.flipped(Qt::Horizontal);
    const double width = display.width();
    auto toDisplay = [mirror, width](const QPointF &p) {
        return mirror ? QPointF(width - p.x(), p.y()) : p;
    };
    auto polyToDisplay = [&toDisplay](const QPolygonF &poly) {
        QPolygonF out;
        for (const QPointF &p : poly)
            out << toDisplay(p);
        return out;
    };

    QPainter painter(&display);
    painter.setRenderHint(QPainter::Antialiasing);
    const int penWidth = std::max(2, display.width() / 250);
    for (size_t i = 0; i < squares.size(); ++i) {
        const DetectedSquare &sq = squares[i];
        const bool best = (i == 0);
        painter.setPen(QPen(best ? QColor(0, 255, 0) : QColor(255, 200, 0), penWidth));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(polyToDisplay(sq.corners));
        painter.setBrush(best ? QColor(0, 255, 0) : QColor(255, 200, 0));
        painter.drawEllipse(toDisplay(sq.center), penWidth * 2, penWidth * 2);
    }
    QFont font = painter.font();
    font.setPixelSize(std::max(14, display.height() / 30));
    font.setBold(true);
    painter.setFont(font);
    if (!squares.empty()) {
        const DetectedSquare &sq = squares.front();
        painter.setPen(QColor(0, 255, 0));
        painter.drawText(polyToDisplay(sq.corners).boundingRect().topLeft() - QPointF(0, penWidth * 2),
                         QStringLiteral("(%1, %2)").arg(sq.center.x(), 0, 'f', 0).arg(sq.center.y(), 0, 'f', 0));
    }

    // Damier de calibration : coins reliés ligne par ligne, cadre blanc à chaque capture.
    if (calibOn) {
        if (boardFound) {
            painter.setPen(QPen(QColor(0, 200, 255), penWidth));
            const QPolygonF corners =
                polyToDisplay(QPolygonF(QList<QPointF>(boardCorners.begin(), boardCorners.end())));
            painter.drawPolyline(corners);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 200, 255));
            for (const QPointF &p : corners)
                painter.drawEllipse(p, penWidth * 1.5, penWidth * 1.5);
        }
        if (captured) {
            painter.setPen(QPen(Qt::white, penWidth * 6));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(display.rect());
        }
        painter.setPen(boardFound ? QColor(0, 200, 255) : QColor(255, 80, 80));
        painter.drawText(QPointF(penWidth * 4, font.pixelSize() * 1.5),
                         boardFound ? tr("Damier détecté — %1 capture(s)").arg(m_calibration.captureCount())
                                    : tr("Damier non détecté"));
    }

    // Marqueurs ArUco : contour, coin d'origine en rouge, flèche vers le bord supérieur.
    for (const DetectedMarker &m : markers) {
        const QColor color(255, 0, 255);
        painter.setPen(QPen(color, penWidth));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(polyToDisplay(m.corners));

        if (m.hasPose) {
            // Axes du marqueur : X rouge, Y vert, normale bleue.
            const QColor axisColors[3] = {Qt::red, Qt::green, QColor(0, 120, 255)};
            for (int k = 0; k < 3; ++k) {
                painter.setPen(QPen(axisColors[k], penWidth));
                painter.drawLine(toDisplay(m.axes[0]), toDisplay(m.axes[k + 1]));
            }
        } else {
            const QPointF top = (m.corners[0] + m.corners[1]) / 2;
            painter.drawLine(toDisplay(m.center), toDisplay(top));
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::red);
        painter.drawEllipse(toDisplay(m.corners[0]), penWidth * 2, penWidth * 2);

        painter.setPen(color);
        painter.drawText(toDisplay(m.center) + QPointF(penWidth * 3, -penWidth * 3), QString::number(m.id));
    }
    painter.end();
    m_video->setImage(display);

    QString status = tr("Image : %1 x %2 — %3 i/s<br>")
                         .arg(image.width())
                         .arg(image.height())
                         .arg(m_fps, 0, 'f', 1);
    if (!squaresOn) {
        status += tr("Carré : <i>désactivé</i>");
    } else if (squares.empty()) {
        status += tr("<b style='color:#c00'>Aucun carré détecté</b>");
    } else {
        const DetectedSquare &sq = squares.front();
        status += tr("<b style='color:#080'>Carré détecté</b> (%1 au total)<br>"
                     "Centre : (%2, %3) px<br>"
                     "Côté : %4 px<br>"
                     "Angle : %5°")
                      .arg(squares.size())
                      .arg(sq.center.x(), 0, 'f', 0)
                      .arg(sq.center.y(), 0, 'f', 0)
                      .arg(sq.side, 0, 'f', 0)
                      .arg(sq.angle, 0, 'f', 1);
    }

    status += QStringLiteral("<br><br>");
    if (!markersOn) {
        status += tr("ArUco : <i>désactivé</i>");
    } else if (markers.empty()) {
        status += tr("<b style='color:#c00'>Aucun marqueur ArUco</b>");
    } else {
        status += tr("<b style='color:#a0a'>%n marqueur(s) ArUco</b>", nullptr, int(markers.size()));
        for (const DetectedMarker &m : markers) {
            status += tr("<br>ID %1 : (%2, %3) px, côté %4 px, %5°")
                          .arg(m.id)
                          .arg(m.center.x(), 0, 'f', 0)
                          .arg(m.center.y(), 0, 'f', 0)
                          .arg(m.side, 0, 'f', 0)
                          .arg(m.angle, 0, 'f', 1);
            if (m.hasPose) {
                status += tr("<br>&nbsp;&nbsp;X %1  Y %2  Z %3 mm (distance %4 mm)"
                             "<br>&nbsp;&nbsp;rotation X %5°  Y %6°  Z %7°")
                              .arg(m.position.x(), 0, 'f', 0)
                              .arg(m.position.y(), 0, 'f', 0)
                              .arg(m.position.z(), 0, 'f', 0)
                              .arg(m.distance, 0, 'f', 0)
                              .arg(m.rotation.x(), 0, 'f', 1)
                              .arg(m.rotation.y(), 0, 'f', 1)
                              .arg(m.rotation.z(), 0, 'f', 1);
            }
        }
    }
    if (markersOn && m_calibration.isCalibrated() && !m_calibration.isValidFor(image.size())) {
        const QSize c = m_calibration.imageSize();
        status += tr("<br><b style='color:#c60'>Pas de pose 3D :</b> calibration faite en %1 x %2 (%3), "
                     "image en %4. Choisissez une résolution au format %3 ou recalibrez.")
                      .arg(c.width())
                      .arg(c.height())
                      .arg(aspectName(c))
                      .arg(aspectName(image.size()));
    }
    m_statusLabel->setText(status);
}
