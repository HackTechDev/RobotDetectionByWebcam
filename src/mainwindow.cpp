#include "mainwindow.h"
#include "playerwindow.h"
#include "racegame.h"
#include "parkinggame.h"
#include "wiregame.h"
#include "curlinggame.h"
#include "catmousegame.h"
#include "footballgame.h"
#include "collectgame.h"
#include "simonsaysgame.h"
#include "trackermodels.h"
#include "videowidget.h"

#include <QApplication>
#include <QCamera>
#include <QCameraDevice>
#include <QCameraFormat>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QtConcurrent/QtConcurrentMap>
#include <QtConcurrent/QtConcurrentRun>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMediaCaptureSession>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QVideoFrameInput>
#include <QMediaDevices>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStatusBar>
#include <QToolButton>
#include <QMenu>
#include <QMenuBar>
#include <QActionGroup>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QUrl>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSettings>
#include <QTextStream>
#include <QShortcut>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabBar>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QVideoSink>

#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_session(new QMediaCaptureSession(this))
    , m_sink(new QVideoSink(this))
{
    setWindowTitle(tr("Détection de carré coloré et de marqueurs ArUco"));
    m_markerDetector.setCalibration(&m_calibration);
    m_floor.setCalibration(&m_calibration);

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
    auto *imageSettingsButton = new QPushButton(tr("Réglages de l'image..."), camBox);
    imageSettingsButton->setToolTip(tr("Exposition et balance des blancs"));
    // Capture d'une image : un clic l'enregistre aussitôt ; la flèche propose les autres choix.
    auto *snapshotButton = new QToolButton(camBox);
    snapshotButton->setText(tr("Capture"));
    snapshotButton->setToolTip(tr("Enregistre l'image de la webcam en PNG dans le dossier des captures (F12)"));
    snapshotButton->setPopupMode(QToolButton::MenuButtonPopup);
    auto *snapshotMenu = new QMenu(snapshotButton);
    snapshotMenu->addAction(tr("Image de la webcam (F12)"), this, [this] { saveCapture(false); });
    snapshotMenu->addAction(tr("Image avec les détections (Maj+F12)"), this, [this] { saveCapture(true); });
    snapshotMenu->addSeparator();
    snapshotMenu->addAction(tr("Choisir le dossier des captures..."), this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Dossier des captures"), captureFolder());
        if (!dir.isEmpty())
            QSettings(settingsPath(), QSettings::IniFormat).setValue(QStringLiteral("capture/folder"), dir);
    });
    snapshotMenu->addAction(tr("Ouvrir le dossier des captures"), this, [this] {
        QDir().mkpath(captureFolder());
        QDesktopServices::openUrl(QUrl::fromLocalFile(captureFolder()));
    });
    snapshotButton->setMenu(snapshotMenu);
    connect(snapshotButton, &QToolButton::clicked, this, [this] { saveCapture(false); });
    auto *captureShortcut = new QShortcut(QKeySequence(Qt::Key_F12), this);
    connect(captureShortcut, &QShortcut::activated, this, [this] { saveCapture(false); });
    auto *captureAllShortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F12), this);
    connect(captureAllShortcut, &QShortcut::activated, this, [this] { saveCapture(true); });
    // Vidéo : un clic démarre ou arrête l'enregistrement ; la flèche propose les autres choix.
    m_videoButton = new QToolButton(camBox);
    m_videoButton->setPopupMode(QToolButton::MenuButtonPopup);
    auto *videoMenu = new QMenu(m_videoButton);
    videoMenu->addAction(tr("Vidéo de la webcam (F9)"), this, [this] { toggleVideo(false); });
    videoMenu->addAction(tr("Vidéo avec les détections (Maj+F9)"), this, [this] { toggleVideo(true); });
    videoMenu->addSeparator();
    videoMenu->addAction(tr("Ouvrir le dossier des captures"), this, [this] {
        QDir().mkpath(captureFolder());
        QDesktopServices::openUrl(QUrl::fromLocalFile(captureFolder()));
    });
    m_videoButton->setMenu(videoMenu);
    connect(m_videoButton, &QToolButton::clicked, this, [this] { toggleVideo(false); });
    auto *videoShortcut = new QShortcut(QKeySequence(Qt::Key_F9), this);
    connect(videoShortcut, &QShortcut::activated, this, [this] { toggleVideo(false); });
    auto *videoAllShortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F9), this);
    connect(videoAllShortcut, &QShortcut::activated, this, [this] { toggleVideo(true); });
    m_recorder = new QMediaRecorder(this);
    m_session->setRecorder(m_recorder);
    m_overlaySession = new QMediaCaptureSession(this);
    m_frameInput = new QVideoFrameInput(this);
    m_overlaySession->setVideoFrameInput(m_frameInput);
    m_overlayRecorder = new QMediaRecorder(this);
    m_overlaySession->setRecorder(m_overlayRecorder);
    for (QMediaRecorder *recorder : {m_recorder, m_overlayRecorder}) {
        connect(recorder, &QMediaRecorder::durationChanged, this, &MainWindow::updateVideoButton);
        connect(recorder, &QMediaRecorder::recorderStateChanged, this, [this, recorder](QMediaRecorder::RecorderState state) {
            if (state == QMediaRecorder::StoppedState)
                m_videoStopping = false;
            updateVideoButton();

            if (state == QMediaRecorder::StoppedState && !m_videoPath.isEmpty()
                && recorder->error() == QMediaRecorder::NoError) {
                statusBar()->showMessage(tr("Vidéo enregistrée : %1").arg(QDir::toNativeSeparators(m_videoPath)), 10000);
                m_videoPath.clear();
            }
        });
        connect(recorder, &QMediaRecorder::errorOccurred, this, [this](QMediaRecorder::Error, const QString &message) {
            m_videoPath.clear();
            m_videoStopping = false;
            QMessageBox::warning(this, tr("Vidéo"), tr("L'enregistrement de la vidéo a échoué :\n%1").arg(message));
            updateVideoButton();
        });
    }
    updateVideoButton();
    // Fenêtre du joueur : un clic l'ouvre ou la ferme ; la flèche propose les options.
    m_playerButton = new QToolButton(camBox);
    m_playerButton->setText(tr("Joueur"));
    m_playerButton->setCheckable(true);
    m_playerButton->setToolTip(tr("Ouvre une seconde fenêtre avec la vidéo seule, en plein écran sur l'autre "
                                  "écran, pour le joueur"));
    m_playerButton->setPopupMode(QToolButton::MenuButtonPopup);
    auto *playerMenu = new QMenu(m_playerButton);
    m_playerDetectionsAction = playerMenu->addAction(tr("Afficher les détections"));
    m_playerDetectionsAction->setCheckable(true);
    m_playerDetectionsAction->setChecked(true);
    m_playerDetectionsAction->setToolTip(tr("Zones, marqueurs, objets suivis... comme dans la fenêtre principale"));
    m_playerFlipAction = playerMenu->addAction(tr("Retourner l'image (miroir)"));
    m_playerFlipAction->setCheckable(true);
    m_playerFullScreenAction = playerMenu->addAction(tr("Plein écran (F11)"));
    m_playerFullScreenAction->setCheckable(true);
    m_playerFullScreenAction->setChecked(true);
    QMenu *screenMenu = playerMenu->addMenu(tr("Écran"));
    connect(screenMenu, &QMenu::aboutToShow, this, [this, screenMenu] {
        // Liste des écrans, refaite à chaque ouverture (écran branché ou débranché).
        screenMenu->clear();
        auto *group = new QActionGroup(screenMenu);
        QAction *automatic = screenMenu->addAction(tr("Automatique (l'autre écran)"));
        automatic->setCheckable(true);
        automatic->setChecked(m_playerScreenName.isEmpty());
        group->addAction(automatic);
        connect(automatic, &QAction::triggered, this, [this] {
            m_playerScreenName.clear();
            if (m_playerWindow && m_playerWindow->isVisible())
                m_playerWindow->showOn(playerScreen(), m_playerFullScreenAction->isChecked());
        });
        const QList<QScreen *> screens = QGuiApplication::screens();
        for (int i = 0; i < screens.size(); ++i) {
            QScreen *screen = screens[i];
            QAction *action = screenMenu->addAction(tr("Écran %1 : %2 (%3 × %4)")
                                                        .arg(i + 1)
                                                        .arg(screen->name())
                                                        .arg(screen->size().width())
                                                        .arg(screen->size().height()));
            action->setCheckable(true);
            action->setChecked(screen->name() == m_playerScreenName);
            group->addAction(action);
            connect(action, &QAction::triggered, this, [this, name = screen->name()] {
                m_playerScreenName = name;
                if (m_playerWindow && m_playerWindow->isVisible())
                    m_playerWindow->showOn(playerScreen(), m_playerFullScreenAction->isChecked());
            });
        }
    });
    m_playerButton->setMenu(playerMenu);
    connect(m_playerButton, &QToolButton::toggled, this, &MainWindow::setPlayerWindowOpen);
    connect(m_playerFlipAction, &QAction::toggled, this, [this](bool on) {
        if (m_playerWindow)
            m_playerWindow->setFlipped(on);
    });
    connect(m_playerFullScreenAction, &QAction::triggered, this, [this](bool on) {
        if (m_playerWindow && m_playerWindow->isVisible())
            m_playerWindow->setFullScreenMode(on);
    });
    auto *camRow = new QWidget(camBox);
    auto *camRowLayout = new QHBoxLayout(camRow);
    camRowLayout->setContentsMargins(0, 0, 0, 0);
    camRowLayout->addWidget(m_mirrorCheck);
    camRowLayout->addStretch();
    camRowLayout->addWidget(snapshotButton);
    camRowLayout->addWidget(m_videoButton);
    camRowLayout->addWidget(m_playerButton);
    camRowLayout->addWidget(imageSettingsButton);
    camForm->addRow(camRow);
    m_cameraSettingsDialog = new CameraSettingsDialog(this);
    panel->addWidget(camBox);

    auto *tabs = new QTabWidget(central);
    m_tabs = tabs;
    panel->addWidget(tabs);

    auto *detBox = new QGroupBox(tr("Activer la détection"), tabs);
    detBox->setCheckable(true);
    detBox->setChecked(false);  // détections automatiques : désactivées à chaque lancement
    m_squareBox = detBox;
    auto *detForm = new QFormLayout(detBox);
    m_squareForm = detForm;
    m_colorButton = new QPushButton(detBox);
    m_colorButton->setMinimumHeight(28);
    detForm->addRow(tr("Couleur"), m_colorButton);
    m_colorModeCombo = new QComboBox(detBox);
    m_colorModeCombo->addItems({tr("Couleur vive (teinte)"), tr("Blanc, gris ou noir")});
    m_colorModeCombo->setToolTip(tr("Le blanc, le gris et le noir n'ont pas de teinte : ils sont\n"
                                    "détectés par leur luminosité et leur faible saturation."));
    detForm->addRow(tr("Type"), m_colorModeCombo);
    auto *hint = new QLabel(tr("<i>Astuce : cliquez sur l'image pour<br>choisir la couleur du carré.</i>"), detBox);
    detForm->addRow(hint);

    const SquareDetector::Params defaults;
    m_hueTolSlider = addSlider(detForm, tr("Tolérance teinte"), 1, 90, defaults.hueTolerance);
    m_satSlider = addSlider(detForm, tr("Saturation min"), 0, 255, defaults.satMin);
    m_valSlider = addSlider(detForm, tr("Luminosité min"), 0, 255, defaults.valMin);
    m_valTolSlider = addSlider(detForm, tr("Tolérance luminosité"), 5, 128, defaults.valueTolerance);
    m_satMaxSlider = addSlider(detForm, tr("Saturation max"), 0, 255, defaults.satMax);
    m_areaSlider = addSlider(detForm, tr("Surface min"), 10, 5000, defaults.minArea);
    m_fillSlider = addSlider(detForm, tr("Remplissage min (%)"), 50, 100, int(defaults.minFill * 100));
    m_maskCheck = new QCheckBox(tr("Afficher le masque"), detBox);
    detForm->addRow(m_maskCheck);
    tabs->addTab(detBox, tr("Carré coloré"));

    m_arucoBox = new QGroupBox(tr("Activer la détection"), tabs);
    m_arucoBox->setCheckable(true);
    m_arucoBox->setChecked(false);
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

    // Onglet sol : repère défini par 4 points cliqués aux coins d'un rectangle connu.
    auto *floorTab = new QWidget(tabs);
    auto *floorLayout = new QVBoxLayout(floorTab);
    auto *floorBox = new QGroupBox(tr("Rectangle de référence"), floorTab);
    auto *floorForm = new QFormLayout(floorBox);
    auto makeSizeSpin = [floorBox](double value) {
        auto *spin = new QDoubleSpinBox(floorBox);
        spin->setRange(10, 100000);
        spin->setDecimals(0);
        spin->setSuffix(QStringLiteral(" mm"));
        spin->setValue(value);
        return spin;
    };
    m_floorSizeXSpin = makeSizeSpin(m_floor.size().width());
    m_floorSizeYSpin = makeSizeSpin(m_floor.size().height());
    floorForm->addRow(tr("Longueur X (point 1 → 2)"), m_floorSizeXSpin);
    floorForm->addRow(tr("Longueur Y (point 1 → 4)"), m_floorSizeYSpin);
    m_floorPickButton = new QPushButton(floorBox);
    floorForm->addRow(m_floorPickButton);
    auto *floorClearButton = new QPushButton(tr("Effacer le repère"), floorBox);
    floorForm->addRow(floorClearButton);
    m_floorShowCheck = new QCheckBox(tr("Afficher la zone sur l'image"), floorBox);
    m_floorShowCheck->setChecked(true);
    floorForm->addRow(m_floorShowCheck);
    m_floorHeightSpin = new QDoubleSpinBox(floorBox);
    m_floorHeightSpin->setRange(0, 2000);
    m_floorHeightSpin->setDecimals(0);
    m_floorHeightSpin->setSuffix(QStringLiteral(" mm"));
    m_floorHeightSpin->setToolTip(tr("Hauteur des carrés et des marqueurs au-dessus du sol (par exemple le "
                                     "dessus du robot). Corrige le décalage de perspective ; nécessite la "
                                     "calibration de la caméra."));
    floorForm->addRow(tr("Hauteur des repères"), m_floorHeightSpin);
    floorLayout->addWidget(floorBox);
    m_floorLabel = new QLabel(floorTab);
    m_floorLabel->setWordWrap(true);
    m_floorLabel->setTextFormat(Qt::RichText);
    floorLayout->addWidget(m_floorLabel);
    auto *floorHelp = new QLabel(tr("<i>Marquez au sol les 4 coins d'un rectangle (ruban adhésif, coins "
                                    "d'une table...), puis cliquez-les dans l'ordre : 1 origine, 2 sur "
                                    "l'axe X, 3 coin opposé, 4 sur l'axe Y. La caméra ne doit plus "
                                    "bouger ensuite.</i>"),
                                 floorTab);
    floorHelp->setWordWrap(true);
    floorLayout->addWidget(floorHelp);
    floorLayout->addStretch();
    tabs->addTab(floorTab, tr("Sol"));

    // Onglet zones : rectangles fixes et nommés de l'image ; entrées et sorties signalées.
    auto *zoneTab = new QWidget(tabs);
    auto *zoneLayout = new QVBoxLayout(zoneTab);
    auto *zoneBox = new QGroupBox(tr("Zones"), zoneTab);
    auto *zoneBoxLayout = new QVBoxLayout(zoneBox);
    m_zoneAddButton = new QPushButton(tr("Ajouter une zone"), zoneBox);
    m_zoneAddButton->setToolTip(tr("Tracez ensuite un rectangle sur l'image, ajustez-le, puis validez"));
    zoneBoxLayout->addWidget(m_zoneAddButton);
    m_zoneConfirmButton = new QPushButton(tr("Valider la zone"), zoneBox);
    m_zoneConfirmButton->setToolTip(tr("Aussi avec Entrée ou un double-clic dans le cadre"));
    zoneBoxLayout->addWidget(m_zoneConfirmButton);
    m_zoneList = new QListWidget(zoneBox);
    m_zoneList->setToolTip(tr("Double-cliquez sur une zone pour la renommer"));
    m_zoneList->setMaximumHeight(120);
    zoneBoxLayout->addWidget(m_zoneList);
    auto *zoneButtons = new QHBoxLayout;
    m_zoneRemoveButton = new QPushButton(tr("Retirer"), zoneBox);
    m_zoneClearButton = new QPushButton(tr("Tout retirer"), zoneBox);
    zoneButtons->addWidget(m_zoneRemoveButton);
    zoneButtons->addWidget(m_zoneClearButton);
    zoneBoxLayout->addLayout(zoneButtons);
    m_zoneShowCheck = new QCheckBox(tr("Afficher les zones sur l'image"), zoneBox);
    m_zoneShowCheck->setChecked(true);
    zoneBoxLayout->addWidget(m_zoneShowCheck);
    m_zoneLabel = new QLabel(zoneBox);
    m_zoneLabel->setWordWrap(true);
    m_zoneLabel->setTextFormat(Qt::RichText);
    zoneBoxLayout->addWidget(m_zoneLabel);
    zoneLayout->addWidget(zoneBox);
    m_zoneOccupancyLabel = new QLabel(zoneTab);
    m_zoneOccupancyLabel->setWordWrap(true);
    m_zoneOccupancyLabel->setTextFormat(Qt::RichText);
    zoneLayout->addWidget(m_zoneOccupancyLabel);
    auto *zoneHelp = new QLabel(tr("<i>Une zone est fixe : elle reste au même endroit de l'image. Les objets "
                                   "suivis, les marqueurs ArUco et le carré principal qui y entrent ou en "
                                   "sortent sont inscrits dans le journal des événements (onglet Suivi) et "
                                   "envoyés en UDP. Chaque caméra a ses propres zones.</i>"),
                                zoneTab);
    zoneHelp->setWordWrap(true);
    zoneLayout->addWidget(zoneHelp);
    zoneLayout->addStretch();
    tabs->addTab(zoneTab, tr("Zones"));
    connect(m_zoneAddButton, &QPushButton::clicked, this, &MainWindow::toggleZonePicking);
    connect(m_zoneConfirmButton, &QPushButton::clicked, m_video, &VideoWidget::confirmSelection);
    connect(m_zoneRemoveButton, &QPushButton::clicked, this, &MainWindow::removeSelectedZone);
    connect(m_zoneClearButton, &QPushButton::clicked, this, &MainWindow::clearZones);
    connect(m_zoneList, &QListWidget::currentItemChanged, this, &MainWindow::updateZoneStatus);
    connect(m_zoneList, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        // Renommage dans la liste : nom vide refusé.
        const int id = item->data(Qt::UserRole).toInt();
        QString name = item->text().trimmed();
        if (name.isEmpty()) {
            for (const Zone &z : m_zones.zones())
                if (z.id == id)
                    name = z.name;
            const QSignalBlocker blocker(m_zoneList);
            item->setText(name);
        }
        waitForProcessing();
        m_zones.rename(id, name);
        saveZones();
    });

    // Onglet suivi : objets désignés à la main, chacun suivi par un tracker OpenCV.
    auto *trackTab = new QWidget(tabs);
    auto *trackLayout = new QVBoxLayout(trackTab);
    auto *trackBox = new QGroupBox(tr("Suivi d'objets"), trackTab);
    auto *trackForm = new QFormLayout(trackBox);
    m_trackAlgoCombo = new QComboBox(trackBox);
    m_trackAlgoCombo->addItems(ObjectTracker::algorithmNames());
    m_trackAlgoCombo->setToolTip(tr("CSRT suit bien l'objet, KCF est plus rapide.\n"
                                    "Nano et Vit (réseaux de neurones) résistent mieux aux changements d'aspect "
                                    "et aux pertes momentanées ; leurs modèles (1 à 2 Mo) sont "
                                    "téléchargés au premier choix.\n"
                                    "Le choix s'applique à la prochaine sélection."));
    connect(m_trackAlgoCombo, &QComboBox::activated, this, &MainWindow::onTrackAlgorithmChosen);
    trackForm->addRow(tr("Méthode"), m_trackAlgoCombo);
    m_trackSelectButton = new QPushButton(trackBox);
    trackForm->addRow(m_trackSelectButton);
    m_trackStartButton = new QPushButton(tr("Valider l'objet"), trackBox);
    m_trackStartButton->setToolTip(tr("Ajoute l'objet encadré et commence son suivi (aussi avec Entrée ou un "
                                      "double-clic dans le cadre)"));
    trackForm->addRow(m_trackStartButton);
    m_objectList = new QListWidget(trackBox);
    m_objectList->setMaximumHeight(110);
    m_objectList->setToolTip(tr("Objets suivis. Double-cliquez sur un nom pour le modifier."));
    trackForm->addRow(m_objectList);
    auto *objectButtons = new QWidget(trackBox);
    auto *objectButtonsLayout = new QHBoxLayout(objectButtons);
    objectButtonsLayout->setContentsMargins(0, 0, 0, 0);
    m_trackRemoveButton = new QPushButton(tr("Retirer"), objectButtons);
    m_trackRemoveButton->setToolTip(tr("Arrête le suivi de l'objet sélectionné dans la liste"));
    m_trackStopButton = new QPushButton(tr("Tout retirer"), objectButtons);
    objectButtonsLayout->addWidget(m_trackRemoveButton);
    objectButtonsLayout->addWidget(m_trackStopButton);
    trackForm->addRow(objectButtons);
    m_trackTrailCheck = new QCheckBox(tr("Afficher la trajectoire"), trackBox);
    m_trackTrailCheck->setChecked(true);
    trackForm->addRow(m_trackTrailCheck);
    m_trackTrailSpin = new QDoubleSpinBox(trackBox);
    m_trackTrailSpin->setRange(1, 120);
    m_trackTrailSpin->setDecimals(0);
    m_trackTrailSpin->setSuffix(QStringLiteral(" s"));
    m_trackTrailSpin->setValue(10);
    trackForm->addRow(tr("Durée de la trajectoire"), m_trackTrailSpin);
    auto makeThresholdSpin = [trackBox](double value, const QString &suffix) {
        auto *spin = new QDoubleSpinBox(trackBox);
        spin->setRange(1, 10000);
        spin->setDecimals(0);
        spin->setSuffix(suffix);
        spin->setValue(value);
        return spin;
    };
    m_motionMmSpin = makeThresholdSpin(20, QStringLiteral(" mm"));
    m_motionMmSpin->setToolTip(tr("Distance à partir de laquelle l'objet est « en mouvement », quand le "
                                  "repère du sol est défini."));
    m_motionPxSpin = makeThresholdSpin(15, QStringLiteral(" px"));
    m_motionPxSpin->setToolTip(tr("Distance à partir de laquelle l'objet est « en mouvement », sans "
                                  "repère du sol."));
    trackForm->addRow(tr("Seuil de mouvement (sol)"), m_motionMmSpin);
    trackForm->addRow(tr("Seuil de mouvement (image)"), m_motionPxSpin);
    trackLayout->addWidget(trackBox);

    // Lissage dans le temps des carrés et des marqueurs (filtre de Kalman).
    m_smoothBox = new QGroupBox(tr("Lissage des carrés et marqueurs"), trackTab);
    m_smoothBox->setCheckable(true);
    m_smoothBox->setChecked(true);
    auto *smoothForm = new QFormLayout(m_smoothBox);
    m_smoothStrengthSlider = addSlider(smoothForm, tr("Force du lissage"), 0, 100, m_smoother.strength());
    m_smoothStrengthSlider->setToolTip(tr("Plus fort : positions plus stables, mais en retard sur les "
                                          "mouvements rapides."));
    m_smoothHoldSpin = new QDoubleSpinBox(m_smoothBox);
    m_smoothHoldSpin->setRange(0, 5);
    m_smoothHoldSpin->setSingleStep(0.1);
    m_smoothHoldSpin->setDecimals(1);
    m_smoothHoldSpin->setSuffix(QStringLiteral(" s"));
    m_smoothHoldSpin->setValue(0.5);
    m_smoothHoldSpin->setToolTip(tr("Durée pendant laquelle un objet qui n'est plus détecté reste "
                                    "affiché à sa position prédite."));
    smoothForm->addRow(tr("Maintien après perte"), m_smoothHoldSpin);
    m_smoothTrailCheck = new QCheckBox(tr("Afficher les trajectoires"), m_smoothBox);
    m_smoothTrailCheck->setChecked(true);
    smoothForm->addRow(m_smoothTrailCheck);
    m_smoothTrailSpin = new QDoubleSpinBox(m_smoothBox);
    m_smoothTrailSpin->setRange(1, 120);
    m_smoothTrailSpin->setDecimals(0);
    m_smoothTrailSpin->setSuffix(QStringLiteral(" s"));
    m_smoothTrailSpin->setValue(10);
    smoothForm->addRow(tr("Durée des trajectoires"), m_smoothTrailSpin);
    trackLayout->addWidget(m_smoothBox);
    m_trackLabel = new QLabel(trackTab);
    m_trackLabel->setWordWrap(true);
    m_trackLabel->setTextFormat(Qt::RichText);
    trackLayout->addWidget(m_trackLabel);
    auto *trackHelp = new QLabel(tr("<i>Cliquez sur « Ajouter des objets », puis tracez un rectangle autour "
                                    "de chaque objet (ou cliquez dessus pour le délimiter automatiquement). "
                                    "Ajustez le cadre avec ses poignées et validez chaque objet avec Entrée, "
                                    "un double-clic ou « Valider l'objet ». Échap termine l'ajout.</i>"),
                                 trackTab);
    trackHelp->setWordWrap(true);
    trackLayout->addWidget(trackHelp);

    // Journal des événements de mouvement de l'objet suivi.
    auto *eventBox = new QGroupBox(tr("Journal des événements"), trackTab);
    auto *eventLayout = new QVBoxLayout(eventBox);
    m_eventList = new QListWidget(eventBox);
    m_eventList->setMinimumHeight(110);
    eventLayout->addWidget(m_eventList);
    auto *eventButtons = new QHBoxLayout;
    auto *clearEventsButton = new QPushButton(tr("Effacer"), eventBox);
    auto *saveEventsButton = new QPushButton(tr("Enregistrer..."), eventBox);
    eventButtons->addWidget(clearEventsButton);
    eventButtons->addWidget(saveEventsButton);
    eventLayout->addLayout(eventButtons);
    trackLayout->addWidget(eventBox);
    connect(clearEventsButton, &QPushButton::clicked, m_eventList, &QListWidget::clear);
    connect(saveEventsButton, &QPushButton::clicked, this, &MainWindow::saveEventLog);

    trackLayout->addStretch();
    // Onglet long : zone défilante pour ne pas repousser le panneau Résultat.
    auto *trackScroll = new QScrollArea(tabs);
    trackScroll->setWidget(trackTab);
    trackScroll->setWidgetResizable(true);
    trackScroll->setFrameShape(QFrame::NoFrame);
    tabs->addTab(trackScroll, tr("Suivi"));

    // Onglet envoi UDP : un message JSON par image traitée.
    auto *udpTab = new QWidget(tabs);
    auto *udpLayout = new QVBoxLayout(udpTab);
    m_udpBox = new QGroupBox(tr("Envoyer les positions"), udpTab);
    m_udpBox->setCheckable(true);
    m_udpBox->setChecked(false);
    auto *udpForm = new QFormLayout(m_udpBox);
    m_udpHostEdit = new QLineEdit(QStringLiteral("127.0.0.1"), m_udpBox);
    m_udpHostEdit->setToolTip(tr("Adresse IP ou nom de la machine qui reçoit les positions.\n"
                                 "127.0.0.1 = cet ordinateur ; 255.255.255.255 = tout le réseau local."));
    m_udpPortSpin = new QSpinBox(m_udpBox);
    m_udpPortSpin->setRange(1, 65535);
    m_udpPortSpin->setValue(5005);
    udpForm->addRow(tr("Adresse"), m_udpHostEdit);
    udpForm->addRow(tr("Port"), m_udpPortSpin);
    udpLayout->addWidget(m_udpBox);
    m_udpStatusLabel = new QLabel(udpTab);
    m_udpStatusLabel->setWordWrap(true);
    m_udpStatusLabel->setTextFormat(Qt::RichText);
    udpLayout->addWidget(m_udpStatusLabel);
    auto *udpHelp = new QLabel(tr("<i>Un message JSON est envoyé à chaque image : carrés et marqueurs "
                                  "détectés, avec leur position. Format détaillé dans le guide "
                                  "utilisateur. Pour tester : <tt>python3 tools/udp_receiver.py</tt></i>"),
                               udpTab);
    udpHelp->setWordWrap(true);
    udpLayout->addWidget(udpHelp);
    udpLayout->addStretch();
    tabs->addTab(udpTab, tr("Envoi UDP"));
    // Le panneau s'élargit pour que tous les titres d'onglets restent lisibles.
    tabs->setMinimumWidth(tabs->tabBar()->sizeHint().width() + 8);

    auto *resBox = new QGroupBox(tr("Résultat"), central);
    auto *resLayout = new QVBoxLayout(resBox);
    m_statusLabel = new QLabel(resBox);
    m_statusLabel->setTextFormat(Qt::RichText);
    m_statusLabel->setMinimumWidth(240);
    resLayout->addWidget(m_statusLabel);
    panel->addWidget(resBox);
    panel->addStretch();

    auto *quitButton = new QPushButton(tr("Quitter"), central);
    quitButton->setMinimumHeight(32);
    panel->addWidget(quitButton);
    connect(quitButton, &QPushButton::clicked, this, &QWidget::close);

    setCentralWidget(central);

    // Menu Jeu.
    QMenu *gameMenu = menuBar()->addMenu(tr("&Jeu"));
    gameMenu->addAction(tr("Détection robot dans zone..."), this, &MainWindow::openRobotZoneGame);
    gameMenu->addAction(tr("1, 2, 3 Soleil..."), this, &MainWindow::openSoleilGame);
    gameMenu->addAction(tr("Chasse au trésor..."), this, &MainWindow::openTreasureGame);
    registerFrameworkGames();
    for (int i = 0; i < int(m_frameworkGames.size()); ++i)
        gameMenu->addAction(m_frameworkGames[i].game->title() + QStringLiteral("..."), this,
                            [this, i] { openFrameworkGame(i); });
    resize(1100, 650);

    connect(m_colorButton, &QPushButton::clicked, this, &MainWindow::chooseColor);
    connect(m_colorModeCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onColorModeChanged);
    connect(m_video, &VideoWidget::imageClicked, this, &MainWindow::onImageClicked);
    connect(m_sink, &QVideoSink::videoFrameChanged, this, &MainWindow::onFrame);
    connect(&m_watcher, &QFutureWatcher<FrameResult>::finished, this, &MainWindow::onProcessingFinished);
    m_innerPool.setMaxThreadCount(std::max(2, QThread::idealThreadCount()));
    connect(m_cameraCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onCameraChanged);
    connect(m_resolutionCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onResolutionChanged);
    connect(imageSettingsButton, &QPushButton::clicked, this, [this] {
        m_cameraSettingsDialog->show();
        m_cameraSettingsDialog->raise();
        m_cameraSettingsDialog->activateWindow();
    });
    connect(m_cameraSettingsDialog, &CameraSettingsDialog::settingsChanged, this,
            &MainWindow::saveCameraSettings);
    connect(m_dictCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onDictionaryChanged);
    connect(generateButton, &QPushButton::clicked, this, &MainWindow::generateMarker);
    connect(m_markerLengthSpin, &QDoubleSpinBox::valueChanged, this,
            [this](double v) {
                waitForProcessing();
                m_markerDetector.setMarkerLength(v);
            });
    connect(m_boardColsSpin, &QSpinBox::valueChanged, this, &MainWindow::onBoardChanged);
    connect(m_boardRowsSpin, &QSpinBox::valueChanged, this, &MainWindow::onBoardChanged);
    connect(m_squareSizeSpin, &QDoubleSpinBox::valueChanged, this, &MainWindow::onBoardChanged);
    connect(generateBoardButton, &QPushButton::clicked, this, &MainWindow::generateBoard);
    connect(captureButton, &QPushButton::clicked, this, [this] { m_captureRequested = true; });
    connect(clearButton, &QPushButton::clicked, this, [this] {
        waitForProcessing();
        m_calibration.clearCaptures();
        updateCalibrationStatus();
    });
    connect(m_calibrateButton, &QPushButton::clicked, this, &MainWindow::calibrate);
    connect(m_trackSelectButton, &QPushButton::clicked, this, &MainWindow::toggleTrackSelection);
    connect(m_trackStartButton, &QPushButton::clicked, m_video, &VideoWidget::confirmSelection);
    connect(m_video, &VideoWidget::selectionPointClicked, this, &MainWindow::onSelectionPointClicked);
    connect(m_video, &VideoWidget::pendingRectChanged, this, &MainWindow::updateTrackStatus);
    connect(m_video, &VideoWidget::pendingRectChanged, this, &MainWindow::updateZoneStatus);
    // Échap annule la saisie en cours ; Entrée valide le cadre (raccourci actif seulement
    // quand un cadre attend, pour ne pas gêner la saisie dans les champs).
    auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, &MainWindow::cancelCurrentInput);
    m_confirmShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    m_confirmShortcutKeypad = new QShortcut(QKeySequence(Qt::Key_Enter), this);
    for (QShortcut *shortcut : {m_confirmShortcut, m_confirmShortcutKeypad}) {
        shortcut->setEnabled(false);
        connect(shortcut, &QShortcut::activated, m_video, &VideoWidget::confirmSelection);
    }
    connect(m_trackStopButton, &QPushButton::clicked, this, &MainWindow::stopTracking);
    connect(m_video, &VideoWidget::rectangleSelected, this, &MainWindow::onRectangleSelected);
    // La méthode s'applique aux prochains objets ; les autres réglages à tous les objets suivis.
    connect(m_trackTrailSpin, &QDoubleSpinBox::valueChanged, this, [this](double seconds) {
        waitForProcessing();
        for (TrackedItem &item : m_tracked)
            item.tracker->setTrailDuration(seconds);
    });
    for (QDoubleSpinBox *spin : {m_motionMmSpin, m_motionPxSpin}) {
        connect(spin, &QDoubleSpinBox::valueChanged, this, [this] {
            waitForProcessing();
            for (TrackedItem &item : m_tracked)
                item.tracker->setMotionThresholds(m_motionMmSpin->value(), m_motionPxSpin->value());
        });
    }
    connect(m_trackRemoveButton, &QPushButton::clicked, this, &MainWindow::removeSelectedObject);
    connect(m_objectList, &QListWidget::itemChanged, this, &MainWindow::onObjectRenamed);
    connect(m_objectList, &QListWidget::currentRowChanged, this, &MainWindow::updateTrackStatus);
    connect(m_smoothBox, &QGroupBox::toggled, this, [this] {
        waitForProcessing();
        m_smoother.clear();
        applySmoothingToTracker();
    });
    connect(m_smoothStrengthSlider, &QSlider::valueChanged, this, [this](int value) {
        waitForProcessing();
        m_smoother.setStrength(value);
        applySmoothingToTracker();
    });
    connect(m_smoothHoldSpin, &QDoubleSpinBox::valueChanged, this, [this](double seconds) {
        waitForProcessing();
        m_smoother.setHoldTime(seconds);
        applySmoothingToTracker();
    });
    connect(m_smoothTrailSpin, &QDoubleSpinBox::valueChanged, this, [this](double seconds) {
        waitForProcessing();
        m_smoother.setTrailDuration(seconds);
    });
    connect(m_floorPickButton, &QPushButton::clicked, this, &MainWindow::toggleFloorPicking);
    connect(floorClearButton, &QPushButton::clicked, this, &MainWindow::clearFloor);
    connect(m_floorSizeXSpin, &QDoubleSpinBox::valueChanged, this, &MainWindow::onFloorSizeChanged);
    connect(m_floorHeightSpin, &QDoubleSpinBox::valueChanged, this, [this](double mm) {
        waitForProcessing();
        m_floor.setObjectHeight(mm);
        saveFloorSettings();
        updateFloorStatus();
    });
    connect(m_floorSizeYSpin, &QDoubleSpinBox::valueChanged, this, &MainWindow::onFloorSizeChanged);
    connect(m_udpHostEdit, &QLineEdit::editingFinished, this, &MainWindow::applyUdpTarget);
    connect(m_udpPortSpin, &QSpinBox::valueChanged, this, &MainWindow::applyUdpTarget);

    applyUdpTarget();
    updateFloorStatus();
    m_clock.start();
    updateTrackStatus();
    m_lastCapture.start();
    updateCalibrationStatus();

    setTargetColor(QColor(220, 30, 30));
    onColorModeChanged();
    loadSettings();
    m_session->setVideoSink(m_sink);
    m_fpsTimer.start();
    populateCameras();
}

MainWindow::~MainWindow()
{
    waitForProcessing();  // ne pas détruire un objet en cours d'utilisation par le traitement
}

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
    // Caméra mémorisée si elle est branchée, sinon la caméra par défaut du système.
    const QString saved = QSettings(settingsPath(), QSettings::IniFormat)
                              .value(QStringLiteral("camera/device"))
                              .toString();
    const QCameraDevice def = QMediaDevices::defaultVideoInput();
    int defIndex = 0;
    int savedIndex = -1;
    for (int i = 0; i < cameras.size(); ++i) {
        m_cameraCombo->addItem(cameras[i].description(), QVariant::fromValue(cameras[i]));
        if (cameras[i] == def)
            defIndex = i;
        if (savedIndex < 0 && cameras[i].description() == saved)
            savedIndex = i;
    }
    if (savedIndex >= 0)
        defIndex = savedIndex;
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

    waitForProcessing();
    stopVideo();  // la vidéo ne vaut que pour cette caméra
    // La webcam quittée retrouve ses réglages d'origine.
    m_cameraSettingsDialog->restoreCamera();
    delete m_camera;
    m_camera = new QCamera(device, this);
    m_cameraKey = cameraKeyFor(device);
    migrateCameraKey(device);
    connect(m_camera, &QCamera::errorOccurred, this, [this](QCamera::Error, const QString &msg) {
        m_statusLabel->setText(tr("<b style='color:red'>Erreur caméra :</b> %1").arg(msg.toHtmlEscaped()));
    });
    // Exposition et balance des blancs : réappliquées à chaque démarrage de la caméra
    // (lancement, changement de résolution).
    connect(m_camera, &QCamera::activeChanged, this, [this](bool active) {
        if (active)
            CameraSettingsDialog::apply(m_camera, m_cameraSettingsDialog->settings());
    });
    m_session->setCamera(m_camera);
    populateResolutions();
    loadCameraSettings();
    loadCameraReference(m_cameraKey);
    m_cameraSettingsDialog->setCamera(m_camera);
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

QString MainWindow::settingsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/settings.ini");
}

void MainWindow::loadSettings()
{
    waitForProcessing();
    // Les widgets sont déjà connectés : chaque valeur restaurée se propage aux détecteurs.
    const QSettings s(settingsPath(), QSettings::IniFormat);

    restoreGeometry(s.value(QStringLiteral("window/geometry")).toByteArray());
    m_tabs->setCurrentIndex(s.value(QStringLiteral("window/tab"), 0).toInt());
    m_mirrorCheck->setChecked(s.value(QStringLiteral("camera/mirror"), false).toBool());
    m_playerDetectionsAction->setChecked(s.value(QStringLiteral("player/detections"), true).toBool());
    m_playerFlipAction->setChecked(s.value(QStringLiteral("player/flip"), false).toBool());
    m_playerFullScreenAction->setChecked(s.value(QStringLiteral("player/fullscreen"), true).toBool());
    m_playerScreenName = s.value(QStringLiteral("player/screen")).toString();
    if (s.value(QStringLiteral("player/open"), false).toBool()) {
        // Rouverte une fois la fenêtre principale affichée (écran connu).
        QTimer::singleShot(0, this, [this] { setPlayerWindowOpen(true); });
    }

    // Carré et ArUco ne sont pas réactivés : au lancement, seuls les objets encadrés à la main sont
    // suivis ; les détections automatiques se cochent pour la séance.
    m_squareBox->setChecked(false);
    const QColor color(s.value(QStringLiteral("square/color")).toString());
    if (color.isValid())
        setTargetColor(color);
    auto restoreSlider = [&s](QSlider *slider, const char *key) {
        slider->setValue(s.value(QStringLiteral("square/") + QLatin1String(key), slider->value()).toInt());
    };
    restoreSlider(m_hueTolSlider, "hueTolerance");
    restoreSlider(m_satSlider, "satMin");
    restoreSlider(m_valSlider, "valMin");
    restoreSlider(m_valTolSlider, "valueTolerance");
    restoreSlider(m_satMaxSlider, "satMax");
    m_colorModeCombo->setCurrentIndex(
        s.value(QStringLiteral("square/mode")).toString() == QLatin1String("achromatic") ? 1 : 0);
    restoreSlider(m_areaSlider, "minArea");
    restoreSlider(m_fillSlider, "minFill");

    m_arucoBox->setChecked(false);
    const int dict = m_dictCombo->findText(s.value(QStringLiteral("aruco/dictionary")).toString());
    if (dict >= 0)
        m_dictCombo->setCurrentIndex(dict);
    m_markerLengthSpin->setValue(
        s.value(QStringLiteral("aruco/markerLength"), m_markerLengthSpin->value()).toDouble());

    // Le repère du sol est propre à chaque caméra (loadCameraReference()) ; seul son affichage
    // est un réglage général.
    m_floorShowCheck->setChecked(s.value(QStringLiteral("floor/show"), true).toBool());
    m_zoneShowCheck->setChecked(s.value(QStringLiteral("zones/show"), true).toBool());

    m_smoothBox->setChecked(s.value(QStringLiteral("smoothing/enabled"), true).toBool());
    m_smoothStrengthSlider->setValue(s.value(QStringLiteral("smoothing/strength"), m_smoothStrengthSlider->value()).toInt());
    m_smoothHoldSpin->setValue(s.value(QStringLiteral("smoothing/holdSeconds"), m_smoothHoldSpin->value()).toDouble());
    m_smoothTrailCheck->setChecked(s.value(QStringLiteral("smoothing/showTrails"), true).toBool());
    m_smoothTrailSpin->setValue(s.value(QStringLiteral("smoothing/trailSeconds"), m_smoothTrailSpin->value()).toDouble());
    applySmoothingToTracker();
    // Modèle supprimé depuis la dernière fois : retour à CSRT.
    int algorithm = std::clamp(s.value(QStringLiteral("tracking/algorithm"), 0).toInt(), 0,
                               int(ObjectTracker::algorithmNames().size()) - 1);
    if (!ObjectTracker::isAvailable(ObjectTracker::Algorithm(algorithm)))
        algorithm = ObjectTracker::Csrt;
    m_trackAlgoCombo->setCurrentIndex(algorithm);
    m_trackAlgoIndex = algorithm;
    m_trackTrailCheck->setChecked(s.value(QStringLiteral("tracking/showTrail"), true).toBool());
    m_trackTrailSpin->setValue(s.value(QStringLiteral("tracking/trailSeconds"), m_trackTrailSpin->value()).toDouble());
    m_motionMmSpin->setValue(s.value(QStringLiteral("tracking/motionThresholdMm"), m_motionMmSpin->value()).toDouble());
    m_motionPxSpin->setValue(s.value(QStringLiteral("tracking/motionThresholdPx"), m_motionPxSpin->value()).toDouble());

    m_udpHostEdit->setText(s.value(QStringLiteral("udp/host"), m_udpHostEdit->text()).toString());
    m_udpPortSpin->setValue(s.value(QStringLiteral("udp/port"), m_udpPortSpin->value()).toInt());
    applyUdpTarget();
    m_udpBox->setChecked(s.value(QStringLiteral("udp/enabled"), false).toBool());

    m_boardColsSpin->setValue(s.value(QStringLiteral("board/cols"), m_boardColsSpin->value()).toInt());
    m_boardRowsSpin->setValue(s.value(QStringLiteral("board/rows"), m_boardRowsSpin->value()).toInt());
    m_squareSizeSpin->setValue(
        s.value(QStringLiteral("board/squareSize"), m_squareSizeSpin->value()).toDouble());
}

void MainWindow::saveSettings() const
{
    QSettings s(settingsPath(), QSettings::IniFormat);
    s.setValue(QStringLiteral("window/geometry"), saveGeometry());
    s.setValue(QStringLiteral("window/tab"), m_tabs->currentIndex());
    if (m_camera)
        s.setValue(QStringLiteral("camera/device"), m_camera->cameraDevice().description());
    s.setValue(QStringLiteral("camera/mirror"), m_mirrorCheck->isChecked());
    s.setValue(QStringLiteral("player/open"), m_playerWindow && m_playerWindow->isVisible());
    s.setValue(QStringLiteral("player/detections"), m_playerDetectionsAction->isChecked());
    s.setValue(QStringLiteral("player/flip"), m_playerFlipAction->isChecked());
    s.setValue(QStringLiteral("player/fullscreen"), m_playerFullScreenAction->isChecked());
    s.setValue(QStringLiteral("player/screen"), m_playerScreenName);

    s.remove(QStringLiteral("square/enabled"));  // plus mémorisé (anciennes versions)
    s.setValue(QStringLiteral("square/color"), m_targetColor.name());
    s.setValue(QStringLiteral("square/hueTolerance"), m_hueTolSlider->value());
    s.setValue(QStringLiteral("square/satMin"), m_satSlider->value());
    s.setValue(QStringLiteral("square/valMin"), m_valSlider->value());
    s.setValue(QStringLiteral("square/valueTolerance"), m_valTolSlider->value());
    s.setValue(QStringLiteral("square/satMax"), m_satMaxSlider->value());
    s.setValue(QStringLiteral("square/mode"), m_colorModeCombo->currentIndex() == 1
                                                  ? QStringLiteral("achromatic")
                                                  : QStringLiteral("color"));
    s.setValue(QStringLiteral("square/minArea"), m_areaSlider->value());
    s.setValue(QStringLiteral("square/minFill"), m_fillSlider->value());

    s.remove(QStringLiteral("aruco/enabled"));
    s.setValue(QStringLiteral("aruco/dictionary"), m_dictCombo->currentText());
    s.setValue(QStringLiteral("aruco/markerLength"), m_markerLengthSpin->value());

    s.setValue(QStringLiteral("floor/show"), m_floorShowCheck->isChecked());
    s.setValue(QStringLiteral("zones/show"), m_zoneShowCheck->isChecked());
    s.setValue(QStringLiteral("smoothing/enabled"), m_smoothBox->isChecked());
    s.setValue(QStringLiteral("smoothing/strength"), m_smoothStrengthSlider->value());
    s.setValue(QStringLiteral("smoothing/holdSeconds"), m_smoothHoldSpin->value());
    s.setValue(QStringLiteral("smoothing/showTrails"), m_smoothTrailCheck->isChecked());
    s.setValue(QStringLiteral("smoothing/trailSeconds"), m_smoothTrailSpin->value());
    s.setValue(QStringLiteral("tracking/algorithm"), m_trackAlgoCombo->currentIndex());
    s.setValue(QStringLiteral("tracking/motionThresholdMm"), m_motionMmSpin->value());
    s.setValue(QStringLiteral("tracking/motionThresholdPx"), m_motionPxSpin->value());
    s.setValue(QStringLiteral("tracking/showTrail"), m_trackTrailCheck->isChecked());
    s.setValue(QStringLiteral("tracking/trailSeconds"), m_trackTrailSpin->value());
    s.setValue(QStringLiteral("udp/enabled"), m_udpBox->isChecked());
    s.setValue(QStringLiteral("udp/host"), m_udpHostEdit->text());
    s.setValue(QStringLiteral("udp/port"), m_udpPortSpin->value());

    s.setValue(QStringLiteral("board/cols"), m_boardColsSpin->value());
    s.setValue(QStringLiteral("board/rows"), m_boardRowsSpin->value());
    s.setValue(QStringLiteral("board/squareSize"), m_squareSizeSpin->value());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    waitForProcessing();
    // Confirmation, que la fermeture vienne du bouton « Quitter » ou de la barre de titre.
    QMessageBox box(QMessageBox::Question, tr("Quitter"),
                    tr("Voulez-vous vraiment quitter l'application ?"), QMessageBox::NoButton, this);
    QPushButton *quit = box.addButton(tr("Quitter"), QMessageBox::AcceptRole);
    QPushButton *cancel = box.addButton(tr("Annuler"), QMessageBox::RejectRole);
    box.setDefaultButton(cancel);
    box.setEscapeButton(cancel);
    box.exec();
    if (box.clickedButton() != quit) {
        event->ignore();
        return;
    }

    saveSettings();
    // Vidéo en cours : terminée proprement, sinon le fichier serait illisible.
    stopVideo();
    QElapsedTimer waitStop;
    waitStop.start();
    while ((m_recorder->recorderState() != QMediaRecorder::StoppedState
            || m_overlayRecorder->recorderState() != QMediaRecorder::StoppedState)
           && waitStop.elapsed() < 5000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    // Les réglages restent mémorisés dans la webcam : on la rend aux autres applications en
    // automatique, avec ses réglages d'origine. Ils seront réappliqués au prochain lancement.
    m_cameraSettingsDialog->restoreCamera();
    if (m_playerWindow) {
        const QSignalBlocker blocker(m_playerWindow);
        m_playerWindow->close();
    }
    QMainWindow::closeEvent(event);
}

namespace {

// Réduit un texte aux lettres ASCII, chiffres et « _ », que QSettings écrit tels quels.
QString readableKey(const QString &text)
{
    const QString plain = text.normalized(QString::NormalizationForm_D);  // é -> e + accent
    QString out;
    for (const QChar c : plain) {
        if (c.unicode() < 128 && c.isLetterOrNumber())
            out += c;
        else if (c.category() != QChar::Mark_NonSpacing && !out.endsWith(QLatin1Char('_')))
            out += QLatin1Char('_');
    }
    while (out.startsWith(QLatin1Char('_')))
        out.remove(0, 1);
    while (out.endsWith(QLatin1Char('_')))
        out.chop(1);
    return out;
}

// Nom de fichier sûr pour la calibration d'une caméra.
QString calibrationFileName(const QString &cameraKey)
{
    QString name = cameraKey;
    for (QChar &c : name) {
        if (!c.isLetterOrNumber() && c != QLatin1Char('-') && c != QLatin1Char('.'))
            c = QLatin1Char('_');
    }
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/calibrations/")
           + name + QStringLiteral(".yml");
}

bool hasNamesake(const QCameraDevice &device)
{
    for (const QCameraDevice &other : QMediaDevices::videoInputs()) {
        if (other.id() != device.id() && other.description() == device.description())
            return true;
    }
    return false;
}

// Ancienne clé (nom complet de la caméra), que QSettings encodait : « UVC%20Camera%20%28... ».
QString legacyCameraKeyFor(const QCameraDevice &device)
{
    QString key = device.description();
    if (hasNamesake(device))
        key += QStringLiteral(" (%1)").arg(QString::fromUtf8(device.id()));
    return key.replace(QLatin1Char('/'), QLatin1Char('_')).replace(QLatin1Char('\\'), QLatin1Char('_'));
}

} // namespace

QString MainWindow::cameraKeyFor(const QCameraDevice &device)
{
    // Une entrée par modèle de caméra, lisible dans settings.ini : le nom réduit aux lettres et
    // aux chiffres, avec l'identifiant USB s'il y figure (« UVC Camera (046d:0825) » ->
    // « UVC_Camera_046d_0825 »). Si deux caméras branchées portent le même nom, on ajoute
    // l'identifiant système pour les distinguer (« ..._video2 »).
    QString name = device.description();
    QString usb;
    static const QRegularExpression usbId(QStringLiteral("\\b([0-9a-fA-F]{4}):([0-9a-fA-F]{4})\\b"));
    const QRegularExpressionMatch m = usbId.match(name);
    if (m.hasMatch()) {
        usb = (m.captured(1) + QLatin1Char('_') + m.captured(2)).toLower();
        name.remove(m.capturedStart(), m.capturedLength());
    }
    QStringList parts;
    for (const QString &part : {readableKey(name), usb}) {
        if (!part.isEmpty())
            parts << part;
    }
    if (parts.isEmpty())
        parts << QStringLiteral("camera");
    if (hasNamesake(device)) {
        const QString id = readableKey(QString::fromUtf8(device.id()).section(QLatin1Char('/'), -1));
        if (!id.isEmpty())
            parts << id;
    }
    return parts.join(QLatin1Char('_'));
}

void MainWindow::migrateCameraKey(const QCameraDevice &device)
{
    // Réglages et calibration enregistrés sous l'ancienne clé (nom complet de la caméra) :
    // déplacés une fois sous la nouvelle.
    const QString legacy = legacyCameraKeyFor(device);
    if (legacy == m_cameraKey)
        return;
    QSettings s(settingsPath(), QSettings::IniFormat);
    const QString oldGroup = QStringLiteral("cameras/") + legacy + QLatin1Char('/');
    const QString newGroup = resolutionSettingsKey() + QLatin1Char('/');
    bool hasNew = false;
    QStringList oldKeys;
    for (const QString &key : s.allKeys()) {
        if (key.startsWith(oldGroup))
            oldKeys << key;
        else if (key.startsWith(newGroup))
            hasNew = true;
    }
    if (!hasNew) {
        for (const QString &key : oldKeys)
            s.setValue(newGroup + key.mid(oldGroup.size()), s.value(key));
    }
    for (const QString &key : oldKeys)
        s.remove(key);
    const QString oldCalibration = calibrationFileName(legacy);
    if (QFileInfo::exists(oldCalibration) && !QFileInfo::exists(calibrationPath()))
        QFile::rename(oldCalibration, calibrationPath());
}

QString MainWindow::resolutionSettingsKey() const
{
    return QStringLiteral("cameras/") + m_cameraKey;
}

void MainWindow::migrateLegacyReference()
{
    // Avant la gestion de plusieurs caméras, la calibration (calibration.yml) et le repère du sol
    // (groupe « floor ») étaient uniques : on les rattache une fois à la caméra active.
    QSettings s(settingsPath(), QSettings::IniFormat);
    if (s.value(QStringLiteral("legacy/referenceMigrated")).toBool() || m_cameraKey.isEmpty())
        return;
    const QString legacyCalibration =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/calibration.yml");
    if (QFileInfo::exists(legacyCalibration) && !QFileInfo::exists(calibrationPath())) {
        QDir().mkpath(QFileInfo(calibrationPath()).absolutePath());
        QFile::copy(legacyCalibration, calibrationPath());
    }
    const QString group = resolutionSettingsKey() + QStringLiteral("/floor/");
    for (const char *key : {"points", "imageSize", "sizeX", "sizeY", "objectHeight"}) {
        const QString legacyKey = QStringLiteral("floor/") + QLatin1String(key);
        if (s.contains(legacyKey)) {
            s.setValue(group + QLatin1String(key), s.value(legacyKey));
            s.remove(legacyKey);
        }
    }
    s.setValue(QStringLiteral("legacy/referenceMigrated"), true);
}

void MainWindow::loadCameraReference(const QString &key)
{
    waitForProcessing();
    m_cameraKey = key;
    migrateLegacyReference();

    // Calibration propre à cette caméra (aucune : caméra non calibrée).
    m_calibration.reset();
    m_calibration.load(calibrationPath());

    // Repère du sol propre à cette caméra.
    const QSettings s(settingsPath(), QSettings::IniFormat);
    const QString group = resolutionSettingsKey() + QStringLiteral("/floor/");
    {
        // Sans signaux : les changements réenregistreraient le repère pendant son chargement.
        const QSignalBlocker bx(m_floorSizeXSpin), by(m_floorSizeYSpin), bh(m_floorHeightSpin);
        m_floorSizeXSpin->setValue(s.value(group + QStringLiteral("sizeX"), 1000.0).toDouble());
        m_floorSizeYSpin->setValue(s.value(group + QStringLiteral("sizeY"), 1000.0).toDouble());
        m_floorHeightSpin->setValue(s.value(group + QStringLiteral("objectHeight"), 0.0).toDouble());
    }
    m_floor.setSize(QSizeF(m_floorSizeXSpin->value(), m_floorSizeYSpin->value()));
    m_floor.setObjectHeight(m_floorHeightSpin->value());
    m_floor.clearPoints();
    const QVariantList points = s.value(group + QStringLiteral("points")).toList();
    if (points.size() == 8) {
        std::vector<QPointF> pts;
        for (int i = 0; i < 8; i += 2)
            pts.emplace_back(points[i].toDouble(), points[i + 1].toDouble());
        m_floor.setPoints(pts, s.value(group + QStringLiteral("imageSize")).toSize());
    }
    m_floorPicking = false;
    m_floorPicked.clear();

    // Zones propres à cette caméra.
    {
        QSettings zs(settingsPath(), QSettings::IniFormat);
        m_zones.load(zs, resolutionSettingsKey());
    }
    if (m_zonePicking) {
        m_zonePicking = false;
        m_video->setSelectionMode(false);
    }
    updateZoneList();
    updateZoneStatus();

    // Les objets suivis et les positions lissées valaient pour l'autre caméra.
    stopTracking();
    m_smoother.clear();
    updateCalibrationStatus();
    updateFloorStatus();
}

void MainWindow::loadCameraSettings()
{
    // Réglages mémorisés pour ce modèle de caméra (valeurs par défaut sinon).
    const QSettings settings(settingsPath(), QSettings::IniFormat);
    const QString key = resolutionSettingsKey();
    CameraSettingsDialog::Settings s;
    s.manualExposure = settings.value(key + QStringLiteral("/manualExposure"), s.manualExposure).toBool();
    s.exposureMs = settings.value(key + QStringLiteral("/exposureMs"), s.exposureMs).toDouble();
    s.manualWhiteBalance =
        settings.value(key + QStringLiteral("/manualWhiteBalance"), s.manualWhiteBalance).toBool();
    s.colorTemperature = settings.value(key + QStringLiteral("/colorTemperature"), s.colorTemperature).toInt();
    // Autres réglages modifiés (luminosité, gain...) : « controls/0x00980900=140 ».
    const QString controlsGroup = key + QStringLiteral("/controls/");
    for (const QString &name : settings.allKeys()) {
        if (!name.startsWith(controlsGroup))
            continue;
        bool ok = false;
        const quint32 id = name.mid(controlsGroup.size()).remove(QStringLiteral("0x")).toUInt(&ok, 16);
        if (ok)
            s.controls.insert(id, settings.value(name).toInt());
    }
    m_cameraSettingsDialog->setSettings(s);
}

void MainWindow::saveCameraSettings() const
{
    if (!m_camera)
        return;
    const CameraSettingsDialog::Settings s = m_cameraSettingsDialog->settings();
    QSettings settings(settingsPath(), QSettings::IniFormat);
    const QString key = resolutionSettingsKey();
    settings.setValue(key + QStringLiteral("/manualExposure"), s.manualExposure);
    settings.setValue(key + QStringLiteral("/exposureMs"), s.exposureMs);
    settings.setValue(key + QStringLiteral("/manualWhiteBalance"), s.manualWhiteBalance);
    settings.setValue(key + QStringLiteral("/colorTemperature"), s.colorTemperature);
    settings.remove(key + QStringLiteral("/controls"));
    for (auto it = s.controls.cbegin(); it != s.controls.cend(); ++it)
        settings.setValue(QStringLiteral("%1/controls/0x%2").arg(key).arg(it.key(), 8, 16, QLatin1Char('0')), it.value());
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
    stopVideo();  // la taille d'image change : la vidéo en cours est terminée
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
    if (c.isValid()) {
        setTargetColor(c);
        selectColorModeFor(c);
    }
}

void MainWindow::onImageClicked(const QPoint &displayPos)
{
    waitForProcessing();
    if (m_lastFrame.isNull())
        return;
    // Le clic est fait sur l'image affichée, éventuellement en miroir.
    const QPoint pos = m_mirrorCheck->isChecked()
                           ? QPoint(m_lastFrame.width() - 1 - displayPos.x(), displayPos.y())
                           : displayPos;

    // Définition du repère du sol : le clic place un point au lieu de choisir une couleur.
    if (m_floorPicking) {
        m_floorPicked.push_back(QPointF(pos));
        if (m_floorPicked.size() == 4) {
            m_floor.setPoints(m_floorPicked, m_lastFrame.size());
            m_floorPicking = false;
            m_floorPicked.clear();
            saveFloorSettings();
        }
        updateFloorStatus();
        return;
    }
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
    selectColorModeFor(c);

    // Ajuste les seuils autour de la couleur cliquée.
    if (m_colorModeCombo->currentIndex() == 0) {
        m_satSlider->setValue(std::max(30, c.hsvSaturation() * 6 / 10));
        m_valSlider->setValue(std::max(30, c.value() * 5 / 10));
    } else {
        // Marge au-dessus de la saturation mesurée : un blanc sous éclairage chaud est
        // légèrement coloré.
        m_satMaxSlider->setValue(std::clamp(c.hsvSaturation() + 40, 40, 255));
    }
}

void MainWindow::selectColorModeFor(const QColor &color)
{
    // En dessous de ce seuil de saturation, la teinte n'est pas fiable (blanc, gris, noir).
    constexpr int kAchromaticSaturation = 64;
    m_colorModeCombo->setCurrentIndex(color.hsvSaturation() < kAchromaticSaturation ? 1 : 0);
}

void MainWindow::onColorModeChanged()
{
    const bool achromatic = m_colorModeCombo->currentIndex() == 1;
    m_squareForm->setRowVisible(m_hueTolSlider->parentWidget(), !achromatic);
    m_squareForm->setRowVisible(m_satSlider->parentWidget(), !achromatic);
    m_squareForm->setRowVisible(m_valSlider->parentWidget(), !achromatic);
    m_squareForm->setRowVisible(m_valTolSlider->parentWidget(), achromatic);
    m_squareForm->setRowVisible(m_satMaxSlider->parentWidget(), achromatic);
    updateParams();
}

void MainWindow::updateParams()
{
    waitForProcessing();
    if (!m_hueTolSlider || !m_fillSlider || !m_satMaxSlider)
        return;
    SquareDetector::Params p = m_detector.params();
    p.hue = std::max(0, m_targetColor.hsvHue());
    p.hueTolerance = m_hueTolSlider->value();
    p.satMin = m_satSlider->value();
    p.valMin = m_valSlider->value();
    p.achromatic = m_colorModeCombo->currentIndex() == 1;
    p.value = m_targetColor.value();
    p.valueTolerance = m_valTolSlider->value();
    p.satMax = m_satMaxSlider->value();
    p.minArea = m_areaSlider->value();
    p.minFill = m_fillSlider->value() / 100.0;
    m_detector.setParams(p);
}

void MainWindow::onDictionaryChanged(int index)
{
    waitForProcessing();
    m_markerDetector.setDictionary(index);
}

void MainWindow::generateMarker()
{
    waitForProcessing();
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

QString MainWindow::captureFolder() const
{
    const QString fallback = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                             + QStringLiteral("/RobotDetectionByWebcam");
    return QSettings(settingsPath(), QSettings::IniFormat).value(QStringLiteral("capture/folder"), fallback).toString();
}

void MainWindow::saveCapture(bool withDetections)
{
    // Image de la webcam telle que filmée (sans miroir), ou image affichée avec les détections.
    const QImage image = withDetections ? m_lastDisplay : m_lastFrame;
    if (image.isNull()) {
        statusBar()->showMessage(tr("Aucune image à capturer : la webcam ne filme pas encore."), 5000);
        return;
    }
    const QString folder = captureFolder();
    QDir().mkpath(folder);
    const QString name = QStringLiteral("capture_%1%2.png")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")),
                                  withDetections ? QStringLiteral("_detections") : QString());
    const QString path = QDir(folder).filePath(name);
    if (image.save(path))
        statusBar()->showMessage(tr("Capture enregistrée : %1").arg(QDir::toNativeSeparators(path)), 8000);
    else
        QMessageBox::warning(this, tr("Capture"), tr("Impossible d'enregistrer %1").arg(path));
}

QScreen *MainWindow::playerScreen() const
{
    // Écran choisi s'il est branché ; sinon un autre écran que celui de la fenêtre principale.
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (screen->name() == m_playerScreenName)
            return screen;
    }
    QScreen *mainScreen = screen();
    for (QScreen *screen : screens) {
        if (screen != mainScreen)
            return screen;
    }
    return mainScreen;
}

void MainWindow::setPlayerWindowOpen(bool open)
{
    if (open) {
        if (!m_playerWindow) {
            m_playerWindow = new PlayerWindow(this);
            m_playerWindow->setFlipped(m_playerFlipAction->isChecked());
            connect(m_playerWindow, &PlayerWindow::closed, this, [this] {
                const QSignalBlocker blocker(m_playerButton);
                m_playerButton->setChecked(false);
            });
            connect(m_playerWindow, &PlayerWindow::fullScreenChanged, m_playerFullScreenAction, &QAction::setChecked);
        }
        if (!m_lastDisplay.isNull())
            m_playerWindow->setImage(m_playerDetectionsAction->isChecked() ? m_lastDisplay : m_lastFrame);
        // Seul écran : fenêtre normale, pour ne pas cacher la fenêtre principale.
        const bool alone = playerScreen() == screen();
        m_playerWindow->showOn(playerScreen(), m_playerFullScreenAction->isChecked() && !alone);
    } else if (m_playerWindow) {
        const QSignalBlocker blocker(m_playerWindow);  // pas de signal « closed » en retour
        m_playerWindow->close();
    }
    const QSignalBlocker blocker(m_playerButton);
    m_playerButton->setChecked(open);
}

void MainWindow::toggleVideo(bool withDetections)
{
    if (m_videoStopping)
        return;  // fichier précédent en cours de finalisation
    if (m_recorder->recorderState() != QMediaRecorder::StoppedState
        || m_overlayRecorder->recorderState() != QMediaRecorder::StoppedState)
        stopVideo();
    else
        startVideo(withDetections);
}

void MainWindow::startVideo(bool withDetections)
{
    if (!m_camera || !m_camera->isActive() || m_lastFrame.isNull()) {
        statusBar()->showMessage(tr("Pas de vidéo possible : la webcam ne filme pas encore."), 5000);
        return;
    }
    const QString folder = captureFolder();
    QDir().mkpath(folder);
    m_videoPath = QDir(folder).filePath(
        QStringLiteral("video_%1%2.mp4")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")),
                 withDetections ? QStringLiteral("_detections") : QString()));
    QMediaRecorder *recorder = withDetections ? m_overlayRecorder : m_recorder;
    QMediaFormat format(QMediaFormat::MPEG4);
    format.setVideoCodec(QMediaFormat::VideoCodec::H264);
    recorder->setMediaFormat(format);
    recorder->setQuality(QMediaRecorder::HighQuality);
    recorder->setOutputLocation(QUrl::fromLocalFile(m_videoPath));
    if (withDetections) {
        recorder->setVideoResolution(m_lastDisplay.size());
        recorder->setVideoFrameRate(std::max(1.0, std::round(m_fps)));
        m_overlayClock.start();
    }
    recorder->record();
    updateVideoButton();
}

void MainWindow::stopVideo()
{
    for (QMediaRecorder *recorder : {m_recorder, m_overlayRecorder}) {
        if (recorder && recorder->recorderState() != QMediaRecorder::StoppedState) {
            m_videoStopping = true;
            recorder->stop();
        }
    }
    if (m_videoButton)
        updateVideoButton();
}

void MainWindow::updateVideoButton()
{
    QMediaRecorder *recorder = m_overlayRecorder->recorderState() != QMediaRecorder::StoppedState
                                   ? m_overlayRecorder
                                   : m_recorder;
    m_videoButton->setEnabled(!m_videoStopping);
    if (m_videoStopping) {
        // Le codeur termine les images en attente et ferme le fichier (une à deux secondes).
        m_videoButton->setText(tr("Finalisation..."));
        m_videoButton->setStyleSheet(QString());
        return;
    }
    if (recorder->recorderState() == QMediaRecorder::StoppedState) {
        m_videoButton->setText(tr("Vidéo"));
        m_videoButton->setStyleSheet(QString());
        m_videoButton->setToolTip(tr("Enregistre la vidéo de la webcam en MP4 dans le dossier des captures (F9) ; "
                                     "un nouveau clic l'arrête"));
        return;
    }
    const qint64 s = recorder->duration() / 1000;
    m_videoButton->setText(tr("■ Arrêter %1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0')));
    m_videoButton->setStyleSheet(QStringLiteral("color: #c00; font-weight: bold;"));
    m_videoButton->setToolTip(tr("Enregistrement en cours dans %1 : cliquez (ou F9) pour l'arrêter")
                                  .arg(QDir::toNativeSeparators(m_videoPath)));
}

QString MainWindow::calibrationPath() const
{
    // Un fichier par caméra, nommé d'après son identifiant (caractères sûrs uniquement).
    return m_cameraKey.isEmpty() ? QString() : calibrationFileName(m_cameraKey);
}

void MainWindow::updateCalibrationStatus()
{
    waitForProcessing();
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
    waitForProcessing();
    CameraCalibration::Board board;
    board.cols = m_boardColsSpin->value();
    board.rows = m_boardRowsSpin->value();
    board.squareSize = m_squareSizeSpin->value();
    m_calibration.setBoard(board);
    updateCalibrationStatus();
}

void MainWindow::calibrate()
{
    waitForProcessing();
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
    waitForProcessing();
    const CameraCalibration::Board &board = m_calibration.board();
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Enregistrer le damier"),
        QStringLiteral("damier_%1x%2.png").arg(board.cols).arg(board.rows), tr("Images PNG (*.png)"));
    if (path.isEmpty())
        return;
    if (!CameraCalibration::generateBoard(board, 100).save(path))
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'enregistrer %1").arg(path));
}

void MainWindow::applyUdpTarget()
{
    if (m_sender.setTarget(m_udpHostEdit->text(), quint16(m_udpPortSpin->value())))
        m_udpStatusLabel->setText(tr("Destination : <b>%1</b>").arg(m_sender.targetDescription()));
    else
        m_udpStatusLabel->setText(tr("<b style='color:#c00'>%1</b>").arg(m_sender.lastError().toHtmlEscaped()));
}

void MainWindow::toggleFloorPicking()
{
    waitForProcessing();
    if (m_video->selectionMode()) {  // une seule saisie à la fois
        m_video->setSelectionMode(false);
        m_zonePicking = false;
        updateTrackStatus();
        updateZoneStatus();
    }
    m_floorPicking = !m_floorPicking;
    m_floorPicked.clear();
    updateFloorStatus();
}

void MainWindow::clearFloor()
{
    waitForProcessing();
    m_floorPicking = false;
    m_floorPicked.clear();
    m_floor.clearPoints();
    saveFloorSettings();
    updateFloorStatus();
}

void MainWindow::onFloorSizeChanged()
{
    waitForProcessing();
    m_floor.setSize(QSizeF(m_floorSizeXSpin->value(), m_floorSizeYSpin->value()));
    saveFloorSettings();
    updateFloorStatus();
}

namespace {

// Vitesse lissée : en mm/s sur le sol si le repère est défini, sinon en pixels par seconde.
QString speedText(const SmoothedTrack &t)
{
    if (t.hasFloor)
        return QObject::tr("%1 mm/s, direction %2°").arg(t.floorSpeed, 0, 'f', 0).arg(t.floorDirection, 0, 'f', 0);
    return QObject::tr("%1 px/s").arg(t.speed, 0, 'f', 0);
}

QString floorPointName(int index, const QSizeF &size)
{
    const QString w = QString::number(size.width(), 'f', 0);
    const QString h = QString::number(size.height(), 'f', 0);
    switch (index) {
    case 0: return QObject::tr("origine (0, 0)");
    case 1: return QObject::tr("sur l'axe X (%1, 0)").arg(w);
    case 2: return QObject::tr("coin opposé (%1, %2)").arg(w, h);
    default: return QObject::tr("sur l'axe Y (0, %1)").arg(h);
    }
}

} // namespace

void MainWindow::updateFloorStatus()
{
    waitForProcessing();
    m_floorPickButton->setText(m_floorPicking ? tr("Annuler") : tr("Définir les 4 points"));
    if (m_floorPicking) {
        const int n = int(m_floorPicked.size());
        m_floorLabel->setText(tr("<b style='color:#c60'>Cliquez sur le point %1/4</b> : %2")
                                  .arg(n + 1)
                                  .arg(floorPointName(n, m_floor.size())));
        return;
    }
    if (!m_floor.isDefined()) {
        m_floorLabel->setText(tr("<b style='color:#c00'>Repère du sol non défini</b> : pas de position "
                                 "au sol."));
        return;
    }
    const QSize s = m_floor.pointsImageSize();
    QString text = tr("<b style='color:#080'>Repère du sol défini</b> (points pris en %1 x %2)")
                       .arg(s.width())
                       .arg(s.height());
    if (!m_lastFrame.isNull() && m_calibration.isValidFor(m_lastFrame.size()))
        text += tr("<br>Distorsion de l'objectif corrigée par la calibration.");
    if (m_floor.hasCameraPose()) {
        text += tr("<br>Caméra à <b>%1 mm</b> du sol (vérifiable au mètre ruban).").arg(m_floor.cameraHeight(), 0, 'f', 0);
        if (m_floor.objectHeight() > 0)
            text += tr("<br>Positions corrigées pour des repères à %1 mm du sol.").arg(m_floor.objectHeight(), 0, 'f', 0);
    } else if (m_floor.objectHeight() > 0) {
        text += tr("<br><b style='color:#c60'>Hauteur des repères ignorée</b> : la correction nécessite la "
                   "calibration de la caméra (onglet Calibration), dans le même format d'image.");
    }
    m_floorLabel->setText(text);
}

void MainWindow::saveFloorSettings() const
{
    waitForProcessing();
    if (m_cameraKey.isEmpty())
        return;
    QSettings s(settingsPath(), QSettings::IniFormat);
    const QString group = resolutionSettingsKey() + QStringLiteral("/floor/");
    QVariantList points;
    for (const QPointF &p : m_floor.points())
        points << p.x() << p.y();
    s.setValue(group + QStringLiteral("points"), points);
    s.setValue(group + QStringLiteral("imageSize"), m_floor.pointsImageSize());
    s.setValue(group + QStringLiteral("sizeX"), m_floor.size().width());
    s.setValue(group + QStringLiteral("sizeY"), m_floor.size().height());
    s.setValue(group + QStringLiteral("objectHeight"), m_floor.objectHeight());
}

void MainWindow::toggleTrackSelection()
{
    if (m_floorPicking) {  // une seule saisie à la fois
        m_floorPicking = false;
        m_floorPicked.clear();
        updateFloorStatus();
    }
    if (m_zonePicking) {  // tracé de zone abandonné : on passe à l'ajout d'objets
        m_zonePicking = false;
        m_video->setSelectionMode(true);
        updateZoneStatus();
    } else {
        m_video->setSelectionMode(!m_video->selectionMode());
    }
    updateTrackStatus();
}

void MainWindow::toggleZonePicking()
{
    if (m_floorPicking) {  // une seule saisie à la fois
        m_floorPicking = false;
        m_floorPicked.clear();
        updateFloorStatus();
    }
    m_zonePicking = !m_zonePicking;
    m_video->setSelectionMode(m_zonePicking);
    updateTrackStatus();
    updateZoneStatus();
}

void MainWindow::addZone(const QRect &displayRect)
{
    m_zonePicking = false;
    if (m_lastFrame.isNull()) {
        updateZoneStatus();
        return;
    }
    // Le rectangle est tracé sur l'image affichée, éventuellement en miroir.
    QRect rect = displayRect;
    if (m_mirrorCheck->isChecked())
        rect.moveLeft(m_lastFrame.width() - displayRect.right() - 1);
    if (rect.width() < 4 || rect.height() < 4) {
        updateZoneStatus();
        return;
    }
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Nouvelle zone"), tr("Nom de la zone :"), QLineEdit::Normal,
                                         m_zones.nextName(), &ok)
                       .trimmed();
    if (!ok) {
        updateZoneStatus();
        return;
    }
    if (name.isEmpty())
        name = m_zones.nextName();
    waitForProcessing();
    m_zones.add(name, rect, m_lastFrame.size());
    saveZones();
    updateZoneList();
    updateZoneStatus();
}

void MainWindow::removeSelectedZone()
{
    QListWidgetItem *item = m_zoneList->currentItem();
    if (!item)
        return;
    waitForProcessing();
    m_zones.remove(item->data(Qt::UserRole).toInt());
    saveZones();
    updateZoneList();
    updateZoneStatus();
}

void MainWindow::clearZones()
{
    if (m_zones.zones().empty())
        return;
    if (QMessageBox::question(this, tr("Zones"), tr("Retirer toutes les zones de cette caméra ?"))
        != QMessageBox::Yes)
        return;
    waitForProcessing();
    m_zones.clear();
    saveZones();
    updateZoneList();
    updateZoneStatus();
}

void MainWindow::updateZoneList()
{
    const QSignalBlocker blocker(m_zoneList);
    m_zoneList->clear();
    for (const Zone &z : m_zones.zones()) {
        QPixmap pixmap(14, 14);
        pixmap.fill(z.color);
        auto *item = new QListWidgetItem(QIcon(pixmap), z.name);
        item->setData(Qt::UserRole, z.id);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_zoneList->addItem(item);
    }
    refreshGameChoices();
}

void MainWindow::updateZoneStatus()
{
    const bool pending = m_zonePicking && m_video->hasPendingRect();
    m_zoneAddButton->setText(m_zonePicking ? tr("Annuler le tracé") : tr("Ajouter une zone"));
    m_zoneConfirmButton->setEnabled(pending);
    m_zoneRemoveButton->setEnabled(m_zoneList->currentItem() != nullptr);
    m_zoneClearButton->setEnabled(!m_zones.zones().empty());
    if (pending) {
        m_zoneLabel->setText(tr("<b style='color:#c60'>Ajustez la zone</b> (poignées, glisser), puis validez : "
                                "« Valider la zone », Entrée ou double-clic dans le cadre."));
    } else if (m_zonePicking) {
        m_zoneLabel->setText(tr("<b style='color:#c60'>Tracez un rectangle</b> sur l'image (cliquer-glisser). "
                                "Échap pour annuler."));
    } else {
        m_zoneLabel->setText(m_zones.zones().empty()
                                 ? tr("Aucune zone.")
                                 : tr("<b style='color:#080'>%n zone(s)</b>", nullptr, int(m_zones.zones().size())));
    }
}

void MainWindow::openRobotZoneGame()
{
    if (!m_gameDialog) {
        m_gameDialog = new RobotZoneGameDialog(this);
        auto applySelection = [this] {
            m_game.setRobot(m_gameDialog->selectedRobot());
            m_game.setZone(m_gameDialog->selectedZone());
        };
        connect(m_gameDialog, &RobotZoneGameDialog::startRequested, this, [this, applySelection] {
            stopOtherGames(GameKind::RobotZone);
            applySelection();
            m_game.start(m_clock.elapsed());
            m_gameDialog->setRunning(true);
        });
        connect(m_gameDialog, &RobotZoneGameDialog::stopRequested, this, [this] {
            m_game.stop();
            m_gameDialog->setRunning(false);
            refreshGameChoices();
        });
        connect(m_gameDialog, &RobotZoneGameDialog::resetRequested, this,
                [this] { m_game.resetCounters(m_clock.elapsed()); });
        connect(m_gameDialog, &RobotZoneGameDialog::selectionChanged, this, [this, applySelection] {
            // Autre robot ou autre zone pendant la partie : on repart de zéro.
            if (m_game.isRunning()) {
                applySelection();
                m_game.start(m_clock.elapsed());
            }
        });
    }
    refreshGameChoices();
    m_gameDialog->show();
    m_gameDialog->raise();
    m_gameDialog->activateWindow();
}

void MainWindow::openSoleilGame()
{
    if (!m_soleilDialog) {
        m_soleilDialog = new SoleilGameDialog(this);
        connect(m_soleilDialog, &SoleilGameDialog::startRequested, this, [this] {
            stopOtherGames(GameKind::Soleil);
            m_soleil.setRobot(m_soleilDialog->selectedRobot());
            m_soleil.setFinishZone(m_soleilDialog->selectedZone());
            m_soleil.setDifficulty(m_soleilDialog->difficulty());
            // Même seuil de mouvement que le suivi (onglet Suivi).
            m_soleil.setThresholds(m_motionMmSpin->value(), m_motionPxSpin->value());
            m_soleil.start(m_clock.elapsed());
            m_soleilDialog->setRunning(true);
        });
        connect(m_soleilDialog, &SoleilGameDialog::stopRequested, this, [this] {
            m_soleil.stop();
            m_soleilDialog->showStopped();
            refreshGameChoices();
        });
    }
    refreshGameChoices();
    m_soleilDialog->show();
    m_soleilDialog->raise();
    m_soleilDialog->activateWindow();
}

void MainWindow::registerFrameworkGames()
{
    // Ordre du menu Jeu.
    m_frameworkGames.push_back({std::make_unique<RaceGame>(), nullptr});
    m_frameworkGames.push_back({std::make_unique<ParkingGame>(), nullptr});
    m_frameworkGames.push_back({std::make_unique<WireGame>(), nullptr});
    m_frameworkGames.push_back({std::make_unique<CurlingGame>(), nullptr});
    m_frameworkGames.push_back({std::make_unique<CatMouseGame>(), nullptr});
    m_frameworkGames.push_back({std::make_unique<FootballGame>(), nullptr});
    m_frameworkGames.push_back({std::make_unique<CollectGame>(), nullptr});
    m_frameworkGames.push_back({std::make_unique<SimonSaysGame>(), nullptr});
}

GameContext MainWindow::gameContext(const std::vector<DetectedMarker> &markers, const QSize &imageSize) const
{
    GameContext context;
    for (const TrackedItem &item : m_tracked)
        context.tracked.push_back({item.id, item.name, &item.tracker->state()});
    context.markers = &markers;
    context.zones = &m_zones.zones();
    context.imageSize = imageSize;
    context.timeMs = m_clock.elapsed();
    context.motionPx = m_motionPxSpin->value();
    context.motionMm = m_motionMmSpin->value();
    return context;
}

void MainWindow::openFrameworkGame(int index)
{
    FrameworkGame &fg = m_frameworkGames[index];
    if (!fg.dialog) {
        fg.dialog = new GameDialog(fg.game.get(), this);
        connect(fg.dialog, &GameDialog::startRequested, this, [this, index] {
            FrameworkGame &g = m_frameworkGames[index];
            const std::vector<DetectedMarker> noMarkers;
            const GameContext context = gameContext(noMarkers, m_lastFrame.size());
            const GameSettings settings = g.dialog->settings();
            const QString problem = g.game->checkReady(settings, context);
            if (!problem.isEmpty()) {
                QMessageBox::information(g.dialog, g.game->title(), problem);
                return;
            }
            stopOtherGames(GameKind::Framework, index);
            g.game->start(settings, context);
            g.dialog->setRunning(true);
        });
        connect(fg.dialog, &GameDialog::stopRequested, this, [this, index] {
            FrameworkGame &g = m_frameworkGames[index];
            g.game->stop();
            g.dialog->showStopped();
            refreshGameChoices();
        });
        connect(fg.dialog, &GameDialog::actionRequested, this, [this, index] {
            const std::vector<DetectedMarker> noMarkers;
            m_frameworkGames[index].game->action(gameContext(noMarkers, m_lastFrame.size()));
        });
    }
    refreshGameChoices();
    fg.dialog->show();
    fg.dialog->raise();
    fg.dialog->activateWindow();
}

void MainWindow::stopOtherGames(GameKind keep, int frameworkIndex)
{
    for (int i = 0; i < int(m_frameworkGames.size()); ++i) {
        FrameworkGame &g = m_frameworkGames[i];
        if ((keep != GameKind::Framework || i != frameworkIndex) && g.game->isActive()) {
            g.game->stop();
            if (g.dialog)
                g.dialog->showStopped();
        }
    }
    if (keep != GameKind::RobotZone && m_game.isRunning()) {
        m_game.stop();
        if (m_gameDialog)
            m_gameDialog->setRunning(false);
    }
    if (keep != GameKind::Soleil && m_soleil.isActive()) {
        if (m_soleilDialog)
            m_soleilDialog->showStopped();
        m_soleil.stop();
    }
    if (keep != GameKind::Treasure && m_treasure.isActive()) {
        if (m_treasureDialog)
            m_treasureDialog->showStopped();
        m_treasure.stop();
    }
}

void MainWindow::openTreasureGame()
{
    if (!m_treasureDialog) {
        m_treasureDialog = new TreasureGameDialog(this);
        connect(m_treasureDialog, &TreasureGameDialog::startRequested, this, [this] {
            stopOtherGames(GameKind::Treasure);
            m_treasure.setRobot(m_treasureDialog->selectedRobot());
            m_treasure.setTerrainZone(m_treasureDialog->selectedZone());
            m_treasure.setDuration(m_treasureDialog->duration());
            m_treasure.setDifficulty(m_treasureDialog->difficulty());
            m_treasure.start(m_clock.elapsed());
            m_treasureDialog->setRunning(true);
        });
        connect(m_treasureDialog, &TreasureGameDialog::stopRequested, this, [this] {
            m_treasure.stop();
            m_treasureDialog->showStopped();
            refreshGameChoices();
        });
    }
    refreshGameChoices();
    m_treasureDialog->show();
    m_treasureDialog->raise();
    m_treasureDialog->activateWindow();
}

void MainWindow::refreshGameChoices()
{
    bool anyDialog = m_gameDialog || m_soleilDialog || m_treasureDialog;
    for (const FrameworkGame &g : m_frameworkGames)
        anyDialog = anyDialog || g.dialog;
    if (!anyDialog)
        return;
    std::vector<std::pair<int, QString>> robots, zones;
    for (const TrackedItem &item : m_tracked)
        robots.emplace_back(item.id, item.name);
    for (const Zone &z : m_zones.zones())
        zones.emplace_back(z.id, z.name);
    if (m_gameDialog)
        m_gameDialog->setChoices(robots, zones);
    // Pendant une partie, les choix sont figés.
    if (m_soleilDialog && !m_soleil.isRunning())
        m_soleilDialog->setChoices(robots, zones);
    if (m_treasureDialog && !m_treasure.isRunning())
        m_treasureDialog->setChoices(robots, zones);
    for (FrameworkGame &g : m_frameworkGames) {
        if (g.dialog)
            g.dialog->setChoices(robots, zones);
    }
}

void MainWindow::saveZones() const
{
    if (m_cameraKey.isEmpty())
        return;
    QSettings s(settingsPath(), QSettings::IniFormat);
    m_zones.save(s, resolutionSettingsKey());
}

namespace {

// Couleurs des objets suivis, distinctes de celles des autres détections (vert, jaune, magenta,
// orange, rouge, bleu clair).
QColor trackColor(int id)
{
    static const QColor palette[] = {QColor(0, 255, 255),   QColor(255, 255, 255), QColor(170, 130, 255),
                                     QColor(255, 190, 120), QColor(120, 255, 190), QColor(100, 170, 255),
                                     QColor(255, 140, 190), QColor(200, 255, 100)};
    return palette[(id - 1) % (sizeof(palette) / sizeof(palette[0]))];
}

QIcon colorIcon(const QColor &color)
{
    // Contour sombre : les couleurs claires (blanc) restent visibles sur fond clair.
    QPixmap pixmap(14, 14);
    pixmap.fill(color);
    QPainter painter(&pixmap);
    painter.setPen(QColor(60, 60, 60));
    painter.drawRect(0, 0, 13, 13);
    return QIcon(pixmap);
}

} // namespace

void MainWindow::configureTracker(ObjectTracker &tracker) const
{
    tracker.setAlgorithm(ObjectTracker::Algorithm(m_trackAlgoCombo->currentIndex()));
    tracker.setTrailDuration(m_trackTrailSpin->value());
    tracker.setMotionThresholds(m_motionMmSpin->value(), m_motionPxSpin->value());
    // Mêmes réglages que le lissage des carrés et des marqueurs. Sans lissage, la position est celle
    // du tracker et rien n'est prédit ; la vitesse reste filtrée (force 50).
    const bool on = m_smoothBox->isChecked();
    tracker.setSmoothing(on ? m_smoothStrengthSlider->value() : 50, on);
    tracker.setHoldTime(on ? m_smoothHoldSpin->value() : 0.0);
}

void MainWindow::onTrackAlgorithmChosen(int index)
{
    if (ensureTrackerModels(index)) {
        m_trackAlgoIndex = index;
    } else {
        const QSignalBlocker blocker(m_trackAlgoCombo);
        m_trackAlgoCombo->setCurrentIndex(m_trackAlgoIndex);
    }
}

bool MainWindow::ensureTrackerModels(int algorithm)
{
    // Nano et Vit ont besoin de leurs modèles : on propose de les télécharger une fois.
    if (algorithm != ObjectTracker::Nano && algorithm != ObjectTracker::Vit)
        return true;
    const auto model = algorithm == ObjectTracker::Nano ? TrackerModels::Nano : TrackerModels::Vit;
    const std::vector<TrackerModels::File> missing = TrackerModels::missing(model);
    if (missing.empty())
        return true;

    qint64 size = 0;
    QStringList names;
    for (const TrackerModels::File &f : missing) {
        size += f.size;
        names << QStringLiteral("<li><a href='%1'>%2</a></li>").arg(f.url.toString(), f.name);
    }
    QMessageBox box(QMessageBox::Question, tr("Modèle à télécharger"),
                    tr("<p>La méthode <b>%1</b> utilise un réseau de neurones dont le modèle n'est pas encore "
                       "installé (%2 ko) :</p><ul>%3</ul><p>Le télécharger maintenant dans <tt>%4</tt> ?</p>"
                       "<p><small>Sans connexion, vous pouvez aussi placer ces fichiers vous-même dans ce dossier, "
                       "ou dans un dossier <tt>models</tt> à côté de l'application.</small></p>")
                        .arg(m_trackAlgoCombo->itemText(algorithm).toHtmlEscaped())
                        .arg(size / 1024)
                        .arg(names.join(QString()), TrackerModels::downloadDirectory().toHtmlEscaped()),
                    QMessageBox::NoButton, this);
    box.setTextFormat(Qt::RichText);
    box.setTextInteractionFlags(Qt::TextBrowserInteraction);  // liens cliquables
    QPushButton *download = box.addButton(tr("Télécharger"), QMessageBox::AcceptRole);
    box.addButton(tr("Annuler"), QMessageBox::RejectRole);
    box.setDefaultButton(download);
    box.exec();
    if (box.clickedButton() != download)
        return false;

    QString error;
    if (!TrackerModels::download(missing, this, &error)) {
        if (!error.isEmpty())
            QMessageBox::warning(this, tr("Modèle à télécharger"),
                                 tr("Le téléchargement a échoué :\n%1").arg(error));
        return false;
    }
    return true;
}

void MainWindow::onRectangleSelected(const QRect &displayRect)
{
    if (m_zonePicking) {
        addZone(displayRect);
        updateTrackStatus();
        return;
    }
    waitForProcessing();
    if (m_lastFrame.isNull()) {
        updateTrackStatus();
        return;
    }
    // Le rectangle est tracé sur l'image affichée, éventuellement en miroir.
    QRect rect = displayRect;
    if (m_mirrorCheck->isChecked())
        rect.moveLeft(m_lastFrame.width() - displayRect.right() - 1);

    TrackedItem item;
    item.id = m_nextTrackId;
    item.name = tr("Objet %1").arg(item.id);
    item.color = trackColor(item.id);
    item.tracker = std::make_unique<ObjectTracker>();
    configureTracker(*item.tracker);
    const bool started = item.tracker->start(m_lastFrame, rect, m_clock.elapsed());
    if (started) {
        ++m_nextTrackId;
        if (m_tracked.empty())
            m_trackImageSize = m_lastFrame.size();
        auto *listItem = new QListWidgetItem(colorIcon(item.color), item.name);
        listItem->setData(Qt::UserRole, item.id);
        listItem->setFlags(listItem->flags() | Qt::ItemIsEditable);
        const QSignalBlocker blocker(m_objectList);
        m_objectList->addItem(listItem);
        m_tracked.push_back(std::move(item));
    }
    // On reste en mode sélection pour ajouter d'autres objets (Échap ou « Terminer » pour finir).
    m_video->setSelectionMode(true);
    updateTrackStatus();
    if (!started) {
        m_trackLabel->setText(tr("<b style='color:#c00'>Sélection trop petite</b> : tracez un rectangle "
                                 "plus grand autour de l'objet."));
    }
}

void MainWindow::removeSelectedObject()
{
    waitForProcessing();
    QListWidgetItem *listItem = m_objectList->currentItem();
    if (!listItem)
        return;
    const int id = listItem->data(Qt::UserRole).toInt();
    m_tracked.erase(std::remove_if(m_tracked.begin(), m_tracked.end(),
                                   [id](const TrackedItem &item) { return item.id == id; }),
                    m_tracked.end());
    delete listItem;
    updateTrackStatus();
}

void MainWindow::onObjectRenamed(QListWidgetItem *listItem)
{
    waitForProcessing();
    const int id = listItem->data(Qt::UserRole).toInt();
    for (TrackedItem &item : m_tracked) {
        if (item.id == id) {
            const QString name = listItem->text().trimmed();
            item.name = name.isEmpty() ? tr("Objet %1").arg(id) : name;
            if (name.isEmpty()) {
                const QSignalBlocker blocker(m_objectList);
                listItem->setText(item.name);
            }
        }
    }
    refreshGameChoices();
}

void MainWindow::onSelectionPointClicked(const QPoint &displayPos)
{
    if (m_lastFrame.isNull() || m_zonePicking)  // une zone se trace, elle ne se délimite pas d'un clic
        return;
    // Délimitation automatique sur l'image réelle (le clic est fait sur l'image affichée).
    const bool mirror = m_mirrorCheck->isChecked();
    const int w = m_lastFrame.width();
    const QPoint pos = mirror ? QPoint(w - 1 - displayPos.x(), displayPos.y()) : displayPos;
    bool uncertain = false;
    QRect rect = ObjectTracker::delineateAt(m_lastFrame, pos, &uncertain);
    if (mirror)
        rect.moveLeft(w - rect.right() - 1);
    m_video->setPendingRect(rect);
    if (uncertain) {
        m_trackLabel->setText(tr("<b style='color:#c60'>Objet mal délimité</b> : ajustez le cadre avec ses "
                                 "poignées, ou tracez un rectangle, puis validez."));
    }
}

void MainWindow::cancelCurrentInput()
{
    if (m_video->selectionMode()) {
        m_video->setSelectionMode(false);
        m_zonePicking = false;
        updateTrackStatus();
        updateZoneStatus();
    } else if (m_floorPicking) {
        toggleFloorPicking();
    }
}

void MainWindow::stopTracking()
{
    waitForProcessing();
    // « Tout retirer » : plus aucun objet suivi. La numérotation repart de 1.
    m_video->setSelectionMode(false);
    m_tracked.clear();
    m_objectList->clear();
    m_nextTrackId = 1;
    updateTrackStatus();
}

void MainWindow::applySmoothingToTracker()
{
    waitForProcessing();
    for (TrackedItem &item : m_tracked)
        configureTracker(*item.tracker);
}

void MainWindow::updateTrackStatus()
{
    waitForProcessing();
    const bool selecting = m_video->selectionMode() && !m_zonePicking;
    const bool pending = selecting && m_video->hasPendingRect();
    m_trackSelectButton->setText(selecting ? tr("Terminer l'ajout") : tr("Ajouter des objets"));
    m_trackStartButton->setEnabled(pending);
    // Entrée valide le cadre en attente, objet ou zone.
    const bool anyPending = m_video->selectionMode() && m_video->hasPendingRect();
    m_confirmShortcut->setEnabled(anyPending);
    m_confirmShortcutKeypad->setEnabled(anyPending);
    m_trackStopButton->setEnabled(!m_tracked.empty());
    m_trackRemoveButton->setEnabled(m_objectList->currentItem() != nullptr);

    int lost = 0;
    for (const TrackedItem &item : m_tracked)
        lost += !item.tracker->state().found && !item.tracker->state().predicted;
    const QString count = m_tracked.empty() ? tr("Aucun objet suivi.")
                                            : tr("<b style='color:#080'>%n objet(s) suivi(s)</b>", nullptr,
                                                 int(m_tracked.size()));
    QString text;
    if (pending) {
        text = tr("<b style='color:#c60'>Ajustez le cadre</b> (poignées, glisser), puis validez : « Valider "
                  "l'objet », Entrée ou double-clic dans le cadre.");
    } else if (selecting) {
        text = tr("<b style='color:#c60'>Tracez un rectangle</b> autour de l'objet à ajouter (ou cliquez dessus). "
                  "Échap ou « Terminer l'ajout » pour finir.");
    }
    if (!m_tracked.empty() || text.isEmpty())
        text += (text.isEmpty() ? QString() : QStringLiteral("<br>")) + count;
    if (lost > 0) {
        text += tr("<br><b style='color:#c00'>%n perdu(s)</b> : recherche en cours dans toute l'image.", nullptr,
                   lost);
    }
    if (m_tracked.size() >= 3) {
        text += tr("<br><i>Chaque objet ajoute environ 25 ms par image avec CSRT (5 ms avec KCF) : surveillez "
                   "le nombre d'images par seconde, ou choisissez KCF pour les prochains objets.</i>");
    }
    m_trackLabel->setText(text);
    refreshGameChoices();
}

namespace {

QString eventText(const MotionEvent &e)
{
    const QString unit = e.onFloor ? QStringLiteral("mm") : QStringLiteral("px");
    const QString where = QStringLiteral("(%1, %2) %3").arg(e.position.x(), 0, 'f', 0).arg(e.position.y(), 0, 'f', 0).arg(unit);
    switch (e.type) {
    case MotionEvent::MotionStart:
        return QObject::tr("Début de mouvement depuis %1").arg(where);
    case MotionEvent::MotionStop:
        return QObject::tr("Fin de mouvement en %1 — déplacement %2 %3, parcours %4 %3, durée %5 s")
            .arg(where)
            .arg(e.distance, 0, 'f', 0)
            .arg(unit)
            .arg(e.travelled, 0, 'f', 0)
            .arg(e.duration, 0, 'f', 1);
    case MotionEvent::Lost:
        return QObject::tr("Objet perdu (dernière position %1)").arg(where);
    case MotionEvent::Found:
        return QObject::tr("Objet retrouvé en %1").arg(where);
    case MotionEvent::ZoneEnter:
        return QObject::tr("Entre dans la zone « %1 » en %2").arg(e.zoneName, where);
    case MotionEvent::ZoneExit:
        return QObject::tr("Sort de la zone « %1 » (dernière position %2)").arg(e.zoneName, where);
    case MotionEvent::GameEnter:
        return QObject::tr("Jeu : entièrement dans la zone « %1 » en %2").arg(e.zoneName, where);
    case MotionEvent::GameExit:
        return QObject::tr("Jeu : n'est plus entièrement dans la zone « %1 »").arg(e.zoneName);
    case MotionEvent::GameGreen:
        return QObject::tr("1, 2, 3 Soleil : feu vert");
    case MotionEvent::GameRed:
        return QObject::tr("1, 2, 3 Soleil : « Soleil ! »");
    case MotionEvent::GameFault:
        return QObject::tr("1, 2, 3 Soleil : faute n° %1, a bougé en %2").arg(e.count).arg(where);
    case MotionEvent::GameWin:
        return QObject::tr("1, 2, 3 Soleil : arrivé dans « %1 » en %2 s, %3 faute(s), score %4 s")
            .arg(e.zoneName)
            .arg(e.duration, 0, 'f', 1)
            .arg(e.count)
            .arg(e.duration + e.count * SoleilGame::kPenaltySeconds, 0, 'f', 1);
    case MotionEvent::GameTreasure:
        return QObject::tr("Chasse au trésor : trésor n° %1 trouvé en %2").arg(e.count).arg(where);
    case MotionEvent::GameInfo:
        return e.gameText;
    case MotionEvent::GameOver:
        return QObject::tr("Chasse au trésor : terminé, %1 trésor(s) trouvé(s) en %2 s")
            .arg(e.count)
            .arg(e.duration, 0, 'f', 0);
    }
    return {};
}

QString eventTypeKey(MotionEvent::Type type)
{
    switch (type) {
    case MotionEvent::MotionStart: return QStringLiteral("motion_start");
    case MotionEvent::MotionStop: return QStringLiteral("motion_stop");
    case MotionEvent::Lost: return QStringLiteral("lost");
    case MotionEvent::Found: return QStringLiteral("found");
    case MotionEvent::ZoneEnter: return QStringLiteral("zone_enter");
    case MotionEvent::ZoneExit: return QStringLiteral("zone_exit");
    case MotionEvent::GameEnter: return QStringLiteral("game_zone_enter");
    case MotionEvent::GameExit: return QStringLiteral("game_zone_exit");
    case MotionEvent::GameGreen: return QStringLiteral("game_green");
    case MotionEvent::GameRed: return QStringLiteral("game_red");
    case MotionEvent::GameFault: return QStringLiteral("game_fault");
    case MotionEvent::GameWin: return QStringLiteral("game_win");
    case MotionEvent::GameTreasure: return QStringLiteral("game_treasure");
    case MotionEvent::GameOver: return QStringLiteral("game_over");
    case MotionEvent::GameInfo: return QStringLiteral("game_event");
    }
    return {};
}

} // namespace

void MainWindow::logEvents(const std::vector<MotionEvent> &events)
{
    // Le plus récent en haut ; on garde les 500 derniers.
    for (const MotionEvent &e : events) {
        const QString time = QDateTime::fromMSecsSinceEpoch(e.wallTimeMs).toString(QStringLiteral("HH:mm:ss.z"));
        auto *item = new QListWidgetItem(QStringLiteral("%1  %2 : %3").arg(time, e.objectName, eventText(e)));
        if (e.type == MotionEvent::ZoneEnter || e.type == MotionEvent::ZoneExit || e.type == MotionEvent::GameEnter
            || e.type == MotionEvent::GameExit) {
            item->setIcon(colorIcon(m_zones.colorOf(e.zoneId)));
        } else {
            for (const TrackedItem &tracked : m_tracked) {
                if (tracked.id == e.objectId)
                    item->setIcon(colorIcon(tracked.color));
            }
        }
        // Données brutes, pour l'enregistrement en CSV.
        item->setData(Qt::UserRole, QVariantList{e.wallTimeMs, eventTypeKey(e.type), e.position.x(), e.position.y(),
                                                 e.onFloor ? QStringLiteral("mm") : QStringLiteral("px"),
                                                 e.distance, e.travelled, e.duration, e.objectId, e.objectName,
                                                 e.zoneName, e.count});
        m_eventList->insertItem(0, item);
    }
    while (m_eventList->count() > 500)
        delete m_eventList->takeItem(m_eventList->count() - 1);
}

void MainWindow::saveEventLog()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Enregistrer le journal"),
                                                      QStringLiteral("evenements.csv"), tr("Fichiers CSV (*.csv)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'enregistrer %1").arg(path));
        return;
    }
    QTextStream out(&file);
    out << "heure;horodatage_ms;objet_id;objet;type;x;y;unite;deplacement;parcours;duree_s;zone;compte\n";
    for (int i = m_eventList->count() - 1; i >= 0; --i) {  // du plus ancien au plus récent
        const QVariantList d = m_eventList->item(i)->data(Qt::UserRole).toList();
        out << QDateTime::fromMSecsSinceEpoch(d[0].toLongLong()).toString(Qt::ISODateWithMs) << ';' << d[0].toLongLong()
            << ';' << d[8].toInt() << ';' << d[9].toString().replace(';', ',') << ';' << d[1].toString() << ';' << QString::number(d[2].toDouble(), 'f', 1) << ';'
            << QString::number(d[3].toDouble(), 'f', 1) << ';' << d[4].toString() << ';'
            << QString::number(d[5].toDouble(), 'f', 1) << ';' << QString::number(d[6].toDouble(), 'f', 1) << ';'
            << QString::number(d[7].toDouble(), 'f', 2) << ';' << d.value(10).toString().replace(';', ',') << ';' << d.value(11).toInt() << '\n';
    }
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

    // Un traitement est en cours : on garde l'image la plus récente, traitée dès qu'il se termine.
    if (m_watcher.isRunning()) {
        m_pendingImage = image;
        return;
    }
    startProcessing(image);
}

void MainWindow::waitForProcessing() const
{
    if (m_watcher.isRunning())
        const_cast<QFutureWatcher<FrameResult> &>(m_watcher).waitForFinished();
}

void MainWindow::startProcessing(const QImage &image)
{
    ProcessingOptions options;
    options.squaresOn = m_squareBox->isChecked();
    options.markersOn = m_arucoBox->isChecked();
    options.smoothingOn = m_smoothBox->isChecked();
    options.calibOn = m_captureBox->isChecked();
    options.captureNow = m_captureRequested || (m_autoCaptureCheck->isChecked() && m_lastCapture.elapsed() > 2000);
    options.timeMs = m_clock.elapsed();
    m_captureRequested = false;
    m_watcher.setFuture(QtConcurrent::run([this, image, options] { return processFrame(image, options); }));
}

FrameResult MainWindow::processFrame(const QImage &image, const ProcessingOptions &options)
{
    // Fil de traitement : pas d'accès aux widgets. Les objets utilisés ici ne sont modifiés par
    // l'interface qu'après waitForProcessing().
    FrameResult r;
    r.image = image;
    r.options = options;
    const bool floorReady = m_floor.prepare(image.size());

    // La détection des marqueurs et le suivi de chaque objet sont indépendants : ils tournent en
    // parallèle sur les cœurs disponibles, pendant que ce fil détecte les carrés.
    QFuture<std::vector<DetectedMarker>> markersFuture;
    if (options.markersOn)
        markersFuture = QtConcurrent::run(&m_innerPool, [this, &image] { return m_markerDetector.detect(image); });
    std::vector<char> wasFound;
    QFuture<void> trackersFuture;
    if (!m_tracked.empty()) {
        if (image.size() != m_trackImageSize) {
            r.trackSizeChanged = true;  // les cadres n'ont plus de sens dans une autre résolution
        } else {
            for (const TrackedItem &item : m_tracked)
                wasFound.push_back(item.tracker->state().found);
            trackersFuture = QtConcurrent::map(&m_innerPool, m_tracked, [this, &image, &options](TrackedItem &item) {
                item.tracker->update(image, options.timeMs, &m_floor);
            });
        }
    }
    if (options.squaresOn)
        r.squares = m_detector.detect(image);
    if (options.markersOn)
        r.markers = markersFuture.result();
    trackersFuture.waitForFinished();
    if (floorReady)
        m_floor.apply(r.squares, r.markers);
    for (size_t i = 0; i < wasFound.size(); ++i) {
        TrackedItem &item = m_tracked[i];
        r.trackStateChanged |= bool(item.tracker->state().found) != bool(wasFound[i]);
        for (MotionEvent &e : item.tracker->takeEvents()) {
            e.objectId = item.id;
            e.objectName = item.name;
            r.events.push_back(e);
        }
    }
    if (options.smoothingOn) {
        if (image.size() != m_smoothImageSize) {
            m_smoother.clear();  // positions en pixels d'une autre résolution
            m_smoothImageSize = image.size();
        }
        m_smoother.update(options.timeMs, r.squares, r.markers, &m_floor);
    }

    // Mode calibration : recherche du damier et capture manuelle ou automatique.
    if (options.calibOn) {
        r.boardFound = m_calibration.findBoard(image, r.boardCorners);
        if (r.boardFound && options.captureNow) {
            m_calibration.addCapture(image, r.boardCorners);
            m_lastCapture.restart();
            r.captured = true;
        }
    }
    return r;
}

void MainWindow::onProcessingFinished()
{
    presentFrame(m_watcher.result());
    // Image arrivée pendant le traitement : on la traite maintenant (après l'affichage, pour que
    // l'affichage lise les objets partagés sans conflit).
    if (!m_pendingImage.isNull()) {
        const QImage next = m_pendingImage;
        m_pendingImage = QImage();
        startProcessing(next);
    }
}

void MainWindow::presentFrame(const FrameResult &result)
{
    // Fil principal ; le fil de traitement est à l'arrêt pendant l'affichage.
    const QImage &image = result.image;
    // Image affichée : c'est sur elle que portent les clics (couleur, sélection, points du sol).
    m_lastFrame = image;
    const bool squaresOn = result.options.squaresOn;
    const bool markersOn = result.options.markersOn;
    const std::vector<DetectedSquare> &squares = result.squares;
    const std::vector<DetectedMarker> &markers = result.markers;
    std::vector<MotionEvent> frameEvents = result.events;
    // Zones : ce qui s'y trouve (objets suivis, marqueurs, carré principal), entrées et sorties.
    if (!m_zones.zones().empty()) {
        std::vector<ZoneOccupant> occupants;
        for (const TrackedItem &item : m_tracked) {
            const TrackedObject &t = item.tracker->state();
            if (t.found || t.predicted)
                occupants.push_back({QStringLiteral("tracked:%1").arg(item.id), item.name, item.id, t.center,
                                     t.hasFloor, t.floorPosition});
        }
        for (const DetectedMarker &m : result.markers)
            occupants.push_back({QStringLiteral("marker:%1").arg(m.id), tr("ArUco %1").arg(m.id), m.id, m.center,
                                 m.hasFloor, m.floorPosition});
        if (!result.squares.empty()) {
            const DetectedSquare &sq = result.squares.front();
            occupants.push_back({QStringLiteral("square"), tr("Carré"), 0, sq.center, sq.hasFloor, sq.floorPosition});
        }
        const std::vector<MotionEvent> zoneEvents = m_zones.update(occupants, image.size(), m_clock.elapsed());
        frameEvents.insert(frameEvents.end(), zoneEvents.begin(), zoneEvents.end());
        QStringList lines;
        for (const Zone &z : m_zones.zones()) {
            lines << tr("<b style='color:%1'>■</b> <b>%2</b> : %3")
                         .arg(z.color.darker(130).name(), z.name.toHtmlEscaped(),
                              z.occupants.isEmpty() ? tr("vide") : z.occupants.join(QStringLiteral(", ")).toHtmlEscaped());
        }
        const QString text = lines.join(QStringLiteral("<br>"));
        if (m_zoneOccupancyLabel->text() != text)
            m_zoneOccupancyLabel->setText(text);
    } else if (!m_zoneOccupancyLabel->text().isEmpty()) {
        m_zoneOccupancyLabel->clear();
    }
    // Jeu « Détection robot dans zone ».
    QString gameRobotName;
    if (m_game.isRunning()) {
        const TrackedObject *robot = nullptr;
        for (const TrackedItem &item : m_tracked) {
            if (item.id == m_game.robotId()) {
                robot = &item.tracker->state();
                gameRobotName = item.name;
            }
        }
        if (!robot)
            gameRobotName = tr("Le robot");
        const std::vector<MotionEvent> gameEvents =
            m_game.update(robot, gameRobotName, m_zones.zones(), image.size(), m_clock.elapsed());
        frameEvents.insert(frameEvents.end(), gameEvents.begin(), gameEvents.end());
        if (m_gameDialog)
            m_gameDialog->showState(m_game, gameRobotName, m_clock.elapsed());
    }
    // Jeu « 1, 2, 3 Soleil ».
    QString soleilRobotName;
    if (m_soleil.isActive()) {
        const TrackedObject *robot = nullptr;
        for (const TrackedItem &item : m_tracked) {
            if (item.id == m_soleil.robotId()) {
                robot = &item.tracker->state();
                soleilRobotName = item.name;
            }
        }
        if (!robot)
            soleilRobotName = tr("Le robot");
        const std::vector<MotionEvent> soleilEvents =
            m_soleil.update(robot, soleilRobotName, m_zones.zones(), image.size(), m_clock.elapsed());
        frameEvents.insert(frameEvents.end(), soleilEvents.begin(), soleilEvents.end());
        if (m_soleilDialog) {
            m_soleilDialog->showState(m_soleil, soleilRobotName, m_clock.elapsed());
            if (m_soleil.phase() == SoleilGame::Won)
                m_soleilDialog->setRunning(false);  // résultat affiché, nouvelle partie possible
        }
    }
    // Jeux du cadre commun.
    const Game *frameworkGame = nullptr;
    GameContext frameworkContext;
    for (FrameworkGame &g : m_frameworkGames) {
        if (!g.game->isActive())
            continue;
        frameworkContext = gameContext(result.markers, image.size());
        if (g.game->isRunning()) {
            const std::vector<MotionEvent> events = g.game->update(frameworkContext);
            frameEvents.insert(frameEvents.end(), events.begin(), events.end());
        }
        if (g.dialog)
            g.dialog->showState(frameworkContext);
        frameworkGame = g.game.get();
    }
    // Jeu « Chasse au trésor ».
    QString treasureRobotName;
    if (m_treasure.isActive()) {
        const TrackedObject *robot = nullptr;
        for (const TrackedItem &item : m_tracked) {
            if (item.id == m_treasure.robotId()) {
                robot = &item.tracker->state();
                treasureRobotName = item.name;
            }
        }
        if (!robot)
            treasureRobotName = tr("Le robot");
        const std::vector<MotionEvent> treasureEvents =
            m_treasure.update(robot, treasureRobotName, m_zones.zones(), image.size(), m_clock.elapsed());
        frameEvents.insert(frameEvents.end(), treasureEvents.begin(), treasureEvents.end());
        if (m_treasureDialog) {
            m_treasureDialog->showState(m_treasure, treasureRobotName, m_clock.elapsed());
            if (m_treasure.phase() == TreasureGame::Over)
                m_treasureDialog->setRunning(false);  // résultat affiché, nouvelle partie possible
        }
    }
    const bool calibOn = result.options.calibOn;
    const std::vector<QPointF> &boardCorners = result.boardCorners;
    const bool boardFound = result.boardFound;
    const bool captured = result.captured;

    if (m_floor.hasCameraPose() != m_floorPoseShown) {
        m_floorPoseShown = m_floor.hasCameraPose();
        updateFloorStatus();
    }
    if (result.trackSizeChanged) {
        stopTracking();
        m_trackLabel->setText(tr("<b style='color:#c60'>Suivi arrêté</b> : la résolution a changé. "
                                 "Ajoutez de nouveau les objets."));
    } else if (result.trackStateChanged) {
        updateTrackStatus();
    }
    if (!frameEvents.empty()) {
        // Heure réelle de chaque événement, à partir de l'horloge du suivi.
        const qint64 nowWall = QDateTime::currentMSecsSinceEpoch(), nowClock = m_clock.elapsed();
        for (MotionEvent &e : frameEvents)
            e.wallTimeMs = nowWall - (nowClock - e.timeMs);
        logEvents(frameEvents);
    }
    if (captured)
        updateCalibrationStatus();

    const std::vector<const SmoothedTrack *> tracks =
        result.options.smoothingOn ? m_smoother.tracks() : std::vector<const SmoothedTrack *>();
    auto trackFor = [&tracks](SmoothedTrack::Kind kind, int id) -> const SmoothedTrack * {
        for (const SmoothedTrack *t : tracks)
            if (t->kind == kind && t->id == id)
                return t;
        return nullptr;
    };
    if (m_floorPicking && !m_floorPicked.empty() && image.size() != m_lastFrameSizeAtPick) {
        m_floorPicked.clear();  // la résolution a changé pendant la saisie : on recommence
        updateFloorStatus();
    }
    m_lastFrameSizeAtPick = image.size();

    if (m_udpBox->isChecked() && m_sender.hasTarget()) {
        std::vector<TrackedEntry> trackedEntries;
        for (const TrackedItem &item : m_tracked)
            trackedEntries.push_back({item.id, item.name, &item.tracker->state()});
        QJsonObject gameJson;
        const QJsonObject *gamePtr = nullptr;
        if (m_game.isRunning()) {
            gameJson = m_game.toJson(gameRobotName, m_clock.elapsed());
            gamePtr = &gameJson;
        } else if (m_soleil.isActive()) {
            gameJson = m_soleil.toJson(soleilRobotName, m_clock.elapsed());
            gamePtr = &gameJson;
        } else if (m_treasure.isActive()) {
            gameJson = m_treasure.toJson(treasureRobotName, m_clock.elapsed());
            gamePtr = &gameJson;
        } else if (frameworkGame) {
            gameJson = frameworkGame->toJson(frameworkContext);
            gamePtr = &gameJson;
        }
        std::vector<ZoneEntry> zoneEntries;
        const bool floorReady = m_floor.isReady();
        for (const Zone &z : m_zones.zones()) {
            ZoneEntry entry{&z, ZoneManager::imageRect(z, image.size()), {}};
            if (floorReady) {
                const QRectF &b = entry.box;
                for (const QPointF &p : {b.topLeft(), b.topRight(), b.bottomRight(), b.bottomLeft()})
                    entry.floorCorners.push_back(m_floor.toFloor(p));
            }
            zoneEntries.push_back(entry);
        }
        m_sender.send(image.size(), squaresOn ? &squares : nullptr, markersOn ? &markers : nullptr,
                      trackedEntries.empty() ? nullptr : &trackedEntries,
                      m_smoothBox->isChecked() ? &tracks : nullptr, frameEvents.empty() ? nullptr : &frameEvents,
                      zoneEntries.empty() ? nullptr : &zoneEntries, gamePtr);
        QString text = tr("Destination : <b>%1</b><br>Messages envoyés : %2 (dernier : %3 octets)")
                           .arg(m_sender.targetDescription())
                           .arg(m_sender.sentCount())
                           .arg(m_sender.lastSize());
        if (!m_sender.lastError().isEmpty())
            text += tr("<br><b style='color:#c00'>Erreur :</b> %1").arg(m_sender.lastError().toHtmlEscaped());
        m_udpStatusLabel->setText(text);
    }

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

    // Zones : rectangle teinté et nom (avec le nombre d'occupants).
    if (m_zoneShowCheck->isChecked()) {
        for (const Zone &z : m_zones.zones()) {
            QRectF r = ZoneManager::imageRect(z, image.size());
            if (mirror)
                r.moveLeft(width - r.right());
            QColor fill = z.color;
            fill.setAlpha(z.occupants.isEmpty() ? 35 : 80);
            painter.setPen(QPen(z.color, penWidth));
            painter.setBrush(fill);
            painter.drawRect(r);
            painter.setPen(z.color);
            const QString label = z.occupants.isEmpty() ? z.name : QStringLiteral("%1 (%2)").arg(z.name).arg(z.occupants.size());
            painter.drawText(r.topLeft() + QPointF(penWidth * 2, font.pixelSize() + penWidth), label);
        }
        painter.setBrush(Qt::NoBrush);
    }
    // Jeux : zone visée mise en évidence, et bandeau d'état en haut de l'image (visible aussi dans la
    // fenêtre du joueur).
    QString banner;
    QColor bannerColor;
    if (m_game.isRunning()) {
        for (const Zone &z : m_zones.zones()) {
            if (z.id != m_game.insideZoneId())
                continue;
            QRectF r = ZoneManager::imageRect(z, image.size());
            if (mirror)
                r.moveLeft(width - r.right());
            painter.setPen(QPen(QColor(40, 220, 80), penWidth * 3));
            painter.drawRect(r);
        }
        switch (m_game.state()) {
        case RobotZoneGame::Inside:
            banner = tr("%1 : DANS LA ZONE « %2 »").arg(gameRobotName, m_game.insideZoneName());
            bannerColor = QColor(26, 154, 58);
            break;
        case RobotZoneGame::Outside:
            banner = tr("%1 : hors zone").arg(gameRobotName);
            bannerColor = QColor(201, 48, 44);
            break;
        case RobotZoneGame::Lost:
            banner = tr("%1 : non trouvé").arg(gameRobotName);
            bannerColor = QColor(224, 142, 11);
            break;
        }
    } else if (m_soleil.isActive()) {
        // Zone d'arrivée : contour blanc épais en pointillés (« ligne d'arrivée »).
        for (const Zone &z : m_zones.zones()) {
            if (z.id != m_soleil.finishZoneId())
                continue;
            QRectF r = ZoneManager::imageRect(z, image.size());
            if (mirror)
                r.moveLeft(width - r.right());
            painter.setPen(QPen(Qt::white, penWidth * 2, Qt::DashLine));
            painter.drawRect(r);
        }
        banner = m_soleil.bannerText(soleilRobotName, m_clock.elapsed());
        bannerColor = m_soleil.bannerColor(m_clock.elapsed());
    } else if (m_treasure.isActive()) {
        // Terrain (si c'est une zone) en pointillés dorés, et une étoile par trésor trouvé.
        const QColor gold(255, 200, 0);
        for (const Zone &z : m_zones.zones()) {
            if (z.id != m_treasure.terrainZoneId())
                continue;
            QRectF r = ZoneManager::imageRect(z, image.size());
            if (mirror)
                r.moveLeft(width - r.right());
            painter.setPen(QPen(gold, penWidth * 2, Qt::DashLine));
            painter.drawRect(r);
        }
        const double starRadius = std::max(8.0, m_treasure.treasureRadius() * 0.5);
        for (const QPointF &p : m_treasure.foundPositions()) {
            const QPointF c = toDisplay(p);
            QPolygonF star;
            for (int k = 0; k < 10; ++k) {
                const double a = -M_PI / 2 + k * M_PI / 5;
                const double rr = k % 2 ? starRadius * 0.45 : starRadius;
                star << c + QPointF(std::cos(a) * rr, std::sin(a) * rr);
            }
            painter.setPen(QPen(QColor(120, 80, 0), std::max(1, penWidth / 2)));
            painter.setBrush(gold);
            painter.drawPolygon(star);
        }
        painter.setBrush(Qt::NoBrush);
        banner = m_treasure.bannerText(treasureRobotName, m_clock.elapsed());
        bannerColor = m_treasure.bannerColor(m_clock.elapsed());
    } else if (frameworkGame) {
        GameDrawContext drawContext;
        drawContext.painter = &painter;
        drawContext.toDisplay = toDisplay;
        drawContext.rectToDisplay = [mirror, width](const QRectF &r) {
            QRectF out = r;
            if (mirror)
                out.moveLeft(width - r.right());
            return out;
        };
        drawContext.penWidth = penWidth;
        drawContext.font = font;
        frameworkGame->draw(drawContext, frameworkContext);
        painter.setBrush(Qt::NoBrush);
        painter.setFont(font);
        banner = frameworkGame->bannerText(frameworkContext);
        bannerColor = frameworkGame->bannerColor(frameworkContext);
    }
    if (!banner.isEmpty()) {
        QFont bannerFont = font;
        bannerFont.setPixelSize(font.pixelSize() * 2);
        bannerFont.setBold(true);
        painter.setFont(bannerFont);
        const QFontMetrics fm(bannerFont);
        const QRectF box(display.width() / 2.0 - fm.horizontalAdvance(banner) / 2.0 - penWidth * 4, penWidth * 2,
                         fm.horizontalAdvance(banner) + penWidth * 8, fm.height() + penWidth * 2);
        bannerColor.setAlpha(210);
        painter.setPen(Qt::NoPen);
        painter.setBrush(bannerColor);
        painter.drawRoundedRect(box, penWidth * 2, penWidth * 2);
        painter.setPen(Qt::white);
        painter.drawText(box, Qt::AlignCenter, banner);
        painter.setBrush(Qt::NoBrush);
        painter.setFont(font);
    }

    // Repère du sol : contour de la zone et noms des axes, ou points en cours de saisie.
    const QColor floorColor(255, 140, 0);
    if (m_floor.isDefined() && m_floorShowCheck->isChecked() && !m_floorPicking) {
        const std::vector<QPointF> pts = m_floor.pointsFor(image.size());
        if (pts.size() == 4) {
            QPolygonF zone;
            for (const QPointF &p : pts)
                zone << toDisplay(p);
            painter.setPen(QPen(floorColor, penWidth, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawPolygon(zone);
            painter.setPen(QPen(Qt::red, penWidth * 1.5));
            painter.drawLine(zone[0], zone[1]);
            painter.setPen(QPen(Qt::green, penWidth * 1.5));
            painter.drawLine(zone[0], zone[3]);
            painter.setPen(floorColor);
            const QPointF offset(penWidth * 2, -penWidth * 2);
            painter.drawText(zone[0] + offset, tr("O"));
            painter.drawText(zone[1] + offset, tr("X"));
            painter.drawText(zone[3] + offset, tr("Y"));
        }
    }
    if (m_floorPicking) {
        painter.setBrush(floorColor);
        for (size_t i = 0; i < m_floorPicked.size(); ++i) {
            const QPointF p = toDisplay(m_floorPicked[i]);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(p, penWidth * 2.5, penWidth * 2.5);
            painter.setPen(floorColor);
            painter.drawText(p + QPointF(penWidth * 3, -penWidth * 3), QString::number(i + 1));
        }
        painter.setPen(floorColor);
        painter.drawText(QPointF(penWidth * 4, display.height() - font.pixelSize()),
                         tr("Sol : cliquez sur le point %1/4 — %2")
                             .arg(m_floorPicked.size() + 1)
                             .arg(floorPointName(int(m_floorPicked.size()), m_floor.size())));
    }

    // Positions lissées : trajectoires, et position prédite des objets momentanément perdus.
    for (const SmoothedTrack *t : tracks) {
        const bool isSquare = t->kind == SmoothedTrack::Square;
        const QColor color = isSquare ? QColor(140, 255, 140) : QColor(255, 140, 255);
        if (m_smoothTrailCheck->isChecked() && t->trail.size() > 1) {
            QPolygonF trail;
            for (const QPointF &p : t->trail)
                trail << toDisplay(p);
            painter.setPen(QPen(color, std::max(1, penWidth / 2)));
            painter.setBrush(Qt::NoBrush);
            painter.drawPolyline(trail);
        }
        const QPointF p = toDisplay(t->position);
        if (t->predicted) {
            painter.setPen(QPen(color, penWidth, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(p, penWidth * 4, penWidth * 4);
            painter.setPen(color);
            painter.drawText(p + QPointF(penWidth * 5, penWidth * 2),
                             isSquare ? tr("carré (prédit)") : tr("%1 (prédit)").arg(t->id));
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawEllipse(p, penWidth * 1.2, penWidth * 1.2);
        }
    }

    // Objets suivis : trajectoire, cadre et flèche de direction, dans la couleur de chaque objet.
    // Cadre plein : trouvé ; pointillés : position prédite ; rouge en pointillés : perdu.
    for (const TrackedItem &item : m_tracked) {
        const TrackedObject &t = item.tracker->state();
        const QColor color = item.color;
        if (m_trackTrailCheck->isChecked() && item.tracker->trail().size() > 1) {
            // Une reprise automatique commence un nouveau tronçon : pas de trait à travers l'image.
            painter.setPen(QPen(color, std::max(1, penWidth / 2)));
            painter.setBrush(Qt::NoBrush);
            QPolygonF segment;
            for (const ObjectTracker::TrailPoint &p : item.tracker->trail()) {
                if (p.newSegment && segment.size() > 1)
                    painter.drawPolyline(segment);
                if (p.newSegment)
                    segment.clear();
                segment << toDisplay(p.position);
            }
            if (segment.size() > 1)
                painter.drawPolyline(segment);
        }
        QRectF box = t.box;
        if (mirror)
            box.moveLeft(width - t.box.right());
        const bool lostNow = !(t.found || t.predicted);
        const QColor boxColor = lostNow ? QColor(255, 60, 60) : color;
        painter.setPen(QPen(boxColor, penWidth, t.found ? Qt::SolidLine : Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(box);
        const QPointF center = toDisplay(t.center);
        if (t.moving && t.speed > 0) {
            // Flèche dans le sens du déplacement, plus longue quand l'objet va plus vite.
            QPointF dir = toDisplay(t.center + t.velocity) - center;
            dir /= std::hypot(dir.x(), dir.y());
            const double length = std::clamp(t.speed * 0.3, penWidth * 8.0, penWidth * 40.0);
            const QPointF tip = center + dir * length;
            const QPointF side(-dir.y(), dir.x());
            painter.setPen(QPen(color, penWidth));
            painter.drawLine(center, tip);
            painter.drawLine(tip, tip - dir * penWidth * 4 + side * penWidth * 3);
            painter.drawLine(tip, tip - dir * penWidth * 4 - side * penWidth * 3);
        }
        QString label = item.name;
        if (t.found) {
            label += t.hasFloor ? tr(" %1 mm/s").arg(t.floorSpeed, 0, 'f', 0) : tr(" %1 px/s").arg(t.speed, 0, 'f', 0);
            if (t.confidence < 0.6)
                label += tr(" (confiance faible)");
        } else {
            label += t.predicted ? tr(" (prédit)") : t.outOfView ? tr(" (hors du champ)") : tr(" (perdu)");
        }
        painter.setPen(boxColor);
        painter.drawText(box.topLeft() - QPointF(0, penWidth * 2), label);
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
    m_lastDisplay = display;
    if (m_playerWindow && m_playerWindow->isVisible())
        m_playerWindow->setImage(m_playerDetectionsAction->isChecked() ? display : image);
    if (m_overlayRecorder->recorderState() == QMediaRecorder::RecordingState) {
        // Image horodatée d'après l'heure réelle : la vidéo garde la vitesse du traitement. Si
        // l'enregistreur est en retard, l'image est sautée.
        QVideoFrame frame(display);
        const qint64 us = m_overlayClock.nsecsElapsed() / 1000;
        frame.setStartTime(us);
        frame.setEndTime(us + qint64(1e6 / std::max(1.0, m_fps)));
        m_frameInput->sendVideoFrame(frame);
    }

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
        if (sq.hasFloor) {
            status += tr("<br>Sol : X %1  Y %2 mm, angle %3°")
                          .arg(sq.floorPosition.x(), 0, 'f', 0)
                          .arg(sq.floorPosition.y(), 0, 'f', 0)
                          .arg(sq.floorHeading, 0, 'f', 1);
        }
        if (const SmoothedTrack *t = trackFor(SmoothedTrack::Square, -1))
            status += tr("<br>Vitesse : %1").arg(speedText(*t));
    }
    if (const SmoothedTrack *t = trackFor(SmoothedTrack::Square, -1); t && t->predicted) {
        status += tr("<br>Carré non détecté, position prédite (%1, %2) px")
                      .arg(t->position.x(), 0, 'f', 0)
                      .arg(t->position.y(), 0, 'f', 0);
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
            if (m.hasFloor) {
                status += tr("<br>&nbsp;&nbsp;sol X %1  Y %2 mm, cap %3°")
                              .arg(m.floorPosition.x(), 0, 'f', 0)
                              .arg(m.floorPosition.y(), 0, 'f', 0)
                              .arg(m.floorHeading, 0, 'f', 1);
            }
            if (const SmoothedTrack *t = trackFor(SmoothedTrack::Marker, m.id))
                status += tr("<br>&nbsp;&nbsp;vitesse %1").arg(speedText(*t));
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
    for (const SmoothedTrack *t : tracks) {
        if (t->kind == SmoothedTrack::Marker && t->predicted) {
            status += tr("<br>ID %1 : non détecté, position prédite (%2, %3) px")
                          .arg(t->id)
                          .arg(t->position.x(), 0, 'f', 0)
                          .arg(t->position.y(), 0, 'f', 0);
        }
    }
    for (const TrackedItem &item : m_tracked) {
        const TrackedObject &t = item.tracker->state();
        status += tr("<br><br><b style='color:%1'>■</b> <b>%2</b>").arg(item.color.darker(130).name(), item.name.toHtmlEscaped());
        if (t.predicted) {
            status += tr(" : <b style='color:#c60'>non trouvé</b>, position prédite (%1, %2) px")
                          .arg(t.center.x(), 0, 'f', 0)
                          .arg(t.center.y(), 0, 'f', 0);
        } else if (t.outOfView) {
            status += tr(" : <b style='color:#c00'>hors du champ</b> (sorti en (%1, %2) px)")
                          .arg(t.center.x(), 0, 'f', 0)
                          .arg(t.center.y(), 0, 'f', 0);
            status += tr("<br>Recherche le long des bords de l'image, par où il reviendra...");
        } else if (!t.found) {
            status += tr(" : <b style='color:#c00'>perdu</b> (dernière position (%1, %2) px)")
                          .arg(t.center.x(), 0, 'f', 0)
                          .arg(t.center.y(), 0, 'f', 0);
            if (t.drifted)
                status += tr("<br>Le cadre ne ressemble plus à l'objet sélectionné.");
            status += tr("<br>Recherche dans toute l'image...");
        } else {
            status += tr(" : %1<br>Position : (%2, %3) px, vitesse %4 px/s, direction %5°")
                          .arg(t.moving ? tr("en mouvement") : tr("immobile"))
                          .arg(t.center.x(), 0, 'f', 0)
                          .arg(t.center.y(), 0, 'f', 0)
                          .arg(t.speed, 0, 'f', 0)
                          .arg(t.direction, 0, 'f', 0);
            status += tr("<br>Confiance : %1 %").arg(t.confidence * 100, 0, 'f', 0);
            if (t.reacquisitions > 0)
                status += tr(" — retrouvé automatiquement %n fois", nullptr, t.reacquisitions);
            if (t.hasFloor) {
                status += tr("<br>Sol : X %1  Y %2 mm, vitesse %3 mm/s, direction %4°")
                              .arg(t.floorPosition.x(), 0, 'f', 0)
                              .arg(t.floorPosition.y(), 0, 'f', 0)
                              .arg(t.floorSpeed, 0, 'f', 0)
                              .arg(t.floorDirection, 0, 'f', 0);
            }
        }
    }
    if (m_floor.isDefined() && !m_floor.isValidFor(image.size())) {
        const QSize z = m_floor.pointsImageSize();
        status += tr("<br><b style='color:#c60'>Pas de position au sol :</b> repère défini en %1 x %2 (%3), "
                     "image en %4. Choisissez une résolution au format %3 ou redéfinissez le repère.")
                      .arg(z.width())
                      .arg(z.height())
                      .arg(aspectName(z))
                      .arg(aspectName(image.size()));
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
