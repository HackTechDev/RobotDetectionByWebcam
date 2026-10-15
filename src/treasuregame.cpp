#include "treasuregame.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QRandomGenerator>
#include <QVBoxLayout>

#include <cmath>

namespace {

// Rayon du trésor, en fraction du petit côté du terrain.
double radiusFraction(TreasureGame::Difficulty d)
{
    switch (d) {
    case TreasureGame::Easy: return 0.12;
    case TreasureGame::Hard: return 0.05;
    default: return 0.08;
    }
}

constexpr double kMarginFraction = 0.08;    // le trésor n'est pas caché tout au bord du terrain
constexpr double kMinNewDistance = 0.3;     // nouveau trésor à au moins 30 % de la diagonale du robot
constexpr qint64 kTrendIntervalMs = 700;    // comparaison des distances pour « se rapproche »
constexpr double kTrendMinChange = 0.015;   // variation minimale (fraction de la diagonale)

} // namespace

QRectF TreasureGame::terrain(const std::vector<Zone> &zones, const QSize &imageSize) const
{
    for (const Zone &z : zones) {
        if (z.id == m_terrainZoneId)
            return ZoneManager::imageRect(z, imageSize);
    }
    return QRectF(QPointF(0, 0), QSizeF(imageSize));
}

void TreasureGame::hideTreasure(const QRectF &area, const QPointF &avoid)
{
    const QRectF inner = area.adjusted(area.width() * kMarginFraction, area.height() * kMarginFraction,
                                       -area.width() * kMarginFraction, -area.height() * kMarginFraction);
    QRandomGenerator *rng = QRandomGenerator::global();
    // Quelques essais pour ne pas cacher le trésor sous le robot.
    for (int i = 0; i < 20; ++i) {
        m_treasure = QPointF(inner.left() + rng->generateDouble() * inner.width(),
                             inner.top() + rng->generateDouble() * inner.height());
        const QPointF d = m_treasure - avoid;
        if (avoid.isNull() || std::hypot(d.x(), d.y()) > kMinNewDistance * m_diagonal)
            break;
    }
    m_hasTreasure = true;
    m_lastDistance = -1;
    m_trend = 0;
}

void TreasureGame::start(qint64 timeMs)
{
    m_phase = Playing;
    m_startMs = timeMs;
    m_endMs = timeMs + m_durationMs;
    m_hasTreasure = false;
    m_found = 0;
    m_foundPositions.clear();
    m_hint = Unknown;
    m_trend = 0;
    m_distance = -1;
    m_robotFound = false;
}

void TreasureGame::stop()
{
    m_phase = Stopped;
}

double TreasureGame::remaining(qint64 timeMs) const
{
    if (m_phase != Playing)
        return 0.0;
    return std::max<qint64>(0, m_endMs - timeMs) / 1000.0;
}

std::vector<MotionEvent> TreasureGame::update(const TrackedObject *robot, const QString &robotName,
                                              const std::vector<Zone> &zones, const QSize &imageSize,
                                              qint64 timeMs)
{
    std::vector<MotionEvent> events;
    if (m_phase != Playing || imageSize.isEmpty())
        return events;
    m_robotFound = robot && (robot->found || robot->predicted);
    const QRectF area = terrain(zones, imageSize);
    m_diagonal = std::hypot(area.width(), area.height());
    m_radius = radiusFraction(m_difficulty) * std::min(area.width(), area.height());
    if (!m_hasTreasure)
        hideTreasure(area, m_robotFound ? robot->center : QPointF());

    auto makeEvent = [&](MotionEvent::Type type) -> MotionEvent & {
        MotionEvent e;
        e.type = type;
        e.timeMs = timeMs;
        if (robot) {
            e.onFloor = robot->hasFloor;
            e.position = robot->hasFloor ? robot->floorPosition : robot->center;
        }
        e.objectId = m_robotId;
        e.objectName = robotName;
        events.push_back(e);
        return events.back();
    };

    if (timeMs >= m_endMs) {
        m_phase = Over;
        MotionEvent &e = makeEvent(MotionEvent::GameOver);
        e.count = m_found;
        e.duration = m_durationMs / 1000.0;
        return events;
    }

    if (!m_robotFound) {
        m_hint = Unknown;
        m_distance = -1;
        return events;
    }
    const QPointF d = robot->center - m_treasure;
    m_distance = std::hypot(d.x(), d.y());
    if (m_distance <= m_radius) {
        // Trésor trouvé : marqué, puis un nouveau est caché ailleurs.
        ++m_found;
        m_lastFindMs = timeMs;
        m_foundPositions.push_back(m_treasure);
        makeEvent(MotionEvent::GameTreasure).count = m_found;
        hideTreasure(area, robot->center);
        const QPointF dn = robot->center - m_treasure;
        m_distance = std::hypot(dn.x(), dn.y());  // distance au nouveau trésor
        m_hint = Found;
        return events;
    }
    // Indice selon la distance (en fraction de la diagonale du terrain).
    const double f = m_distance / m_diagonal;
    m_hint = f < 0.12 ? Burning : f < 0.25 ? Hot : f < 0.4 ? Warm : f < 0.6 ? Cold : Freezing;
    // Tendance : comparée à la distance d'il y a un peu moins d'une seconde.
    if (m_lastDistance < 0) {
        m_lastDistance = m_distance;
        m_lastTrendMs = timeMs;
    } else if (timeMs - m_lastTrendMs >= kTrendIntervalMs) {
        const double change = (m_lastDistance - m_distance) / m_diagonal;
        m_trend = change > kTrendMinChange ? 1 : change < -kTrendMinChange ? -1 : 0;
        m_lastDistance = m_distance;
        m_lastTrendMs = timeMs;
    }
    return events;
}

QString TreasureGame::hintName(Hint hint)
{
    switch (hint) {
    case Found: return QObject::tr("TROUVÉ !");
    case Burning: return QObject::tr("BRÛLANT");
    case Hot: return QObject::tr("Chaud");
    case Warm: return QObject::tr("Tiède");
    case Cold: return QObject::tr("Froid");
    case Freezing: return QObject::tr("Glacial");
    case Unknown: break;
    }
    return QObject::tr("Robot non trouvé");
}

QString TreasureGame::bannerText(const QString &robotName, qint64 timeMs) const
{
    Q_UNUSED(robotName)
    if (m_phase == Over)
        return m_found == 1 ? QObject::tr("TERMINÉ ! 1 trésor trouvé")
                            : QObject::tr("TERMINÉ ! %1 trésors trouvés").arg(m_found);
    if (recentFind(timeMs))
        return QObject::tr("TRÉSOR TROUVÉ ! (%1)").arg(m_found);
    QString text = hintName(m_hint);
    if (m_hint != Unknown && m_trend != 0)
        text += m_trend > 0 ? QObject::tr(" ▲ plus chaud") : QObject::tr(" ▼ plus froid");
    const int s = int(std::ceil(remaining(timeMs)));
    return QObject::tr("%1 — %2:%3 — trésors : %4")
        .arg(text)
        .arg(s / 60)
        .arg(s % 60, 2, 10, QLatin1Char('0'))
        .arg(m_found);
}

QColor TreasureGame::bannerColor(qint64 timeMs) const
{
    if (m_phase == Over)
        return QColor(30, 100, 200);
    if (recentFind(timeMs))
        return QColor(200, 150, 0);
    switch (m_hint) {
    case Burning: return QColor(220, 30, 30);
    case Hot: return QColor(230, 110, 20);
    case Warm: return QColor(200, 160, 30);
    case Cold: return QColor(60, 140, 210);
    case Freezing: return QColor(40, 70, 170);
    default: return QColor(90, 90, 90);
    }
}

QJsonObject TreasureGame::toJson(const QString &robotName, qint64 timeMs) const
{
    static const char *phases[] = {"stopped", "playing", "over"};
    static const char *hints[] = {"found", "burning", "hot", "warm", "cold", "freezing", "unknown"};
    QJsonObject o{{QStringLiteral("mode"), QStringLiteral("treasure")},
                  {QStringLiteral("robot_id"), m_robotId},
                  {QStringLiteral("robot"), robotName},
                  {QStringLiteral("terrain_zone_id"), m_terrainZoneId},
                  {QStringLiteral("phase"), QString::fromLatin1(phases[m_phase])},
                  {QStringLiteral("hint"), QString::fromLatin1(hints[m_hint])},
                  {QStringLiteral("trend"), m_trend},
                  {QStringLiteral("found"), m_found},
                  {QStringLiteral("remaining_s"), std::round(remaining(timeMs) * 10) / 10}};
    if (m_distance >= 0)
        o.insert(QStringLiteral("distance_px"), std::round(m_distance));
    if (m_hasTreasure && m_phase == Playing) {
        // Pour un programme qui pilote le robot tout seul.
        o.insert(QStringLiteral("treasure_px"), QJsonArray{std::round(m_treasure.x()), std::round(m_treasure.y())});
        o.insert(QStringLiteral("radius_px"), std::round(m_radius));
    }
    return o;
}

TreasureGameDialog::TreasureGameDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Jeu : chasse au trésor"));
    auto *layout = new QVBoxLayout(this);
    auto *rules = new QLabel(tr("Un trésor invisible est caché sur le terrain. Guidé par « chaud / froid », le "
                                "robot doit passer dessus ; un nouveau trésor est alors caché ailleurs. Trouvez-en "
                                "le plus possible avant la fin du temps."),
                             this);
    rules->setWordWrap(true);
    layout->addWidget(rules);
    auto *form = new QFormLayout;
    m_robotCombo = new QComboBox(this);
    m_robotCombo->setToolTip(tr("Objet suivi qui joue le rôle du robot (onglet Suivi)"));
    form->addRow(tr("Robot"), m_robotCombo);
    m_zoneCombo = new QComboBox(this);
    m_zoneCombo->setToolTip(tr("Zone où les trésors sont cachés (onglet Zones), ou toute l'image"));
    form->addRow(tr("Terrain"), m_zoneCombo);
    m_durationCombo = new QComboBox(this);
    for (int minutes : {1, 2, 3, 5})
        m_durationCombo->addItem(minutes == 1 ? tr("1 minute") : tr("%1 minutes").arg(minutes), minutes * 60);
    m_durationCombo->setCurrentIndex(1);
    form->addRow(tr("Durée"), m_durationCombo);
    m_difficultyCombo = new QComboBox(this);
    m_difficultyCombo->addItem(tr("Facile (grand trésor)"), TreasureGame::Easy);
    m_difficultyCombo->addItem(tr("Normal"), TreasureGame::Normal);
    m_difficultyCombo->addItem(tr("Difficile (petit trésor)"), TreasureGame::Hard);
    m_difficultyCombo->setCurrentIndex(1);
    form->addRow(tr("Difficulté"), m_difficultyCombo);
    layout->addLayout(form);

    m_stateLabel = new QLabel(this);
    m_stateLabel->setAlignment(Qt::AlignCenter);
    m_stateLabel->setMinimumHeight(80);
    m_stateLabel->setWordWrap(true);
    QFont big = m_stateLabel->font();
    big.setPointSizeF(big.pointSizeF() * 2.0);
    big.setBold(true);
    m_stateLabel->setFont(big);
    layout->addWidget(m_stateLabel);
    m_countersLabel = new QLabel(this);
    m_countersLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_countersLabel);
    m_hintLabel = new QLabel(this);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setTextFormat(Qt::RichText);
    layout->addWidget(m_hintLabel);

    auto *buttons = new QDialogButtonBox(this);
    m_startButton = buttons->addButton(tr("Démarrer"), QDialogButtonBox::ActionRole);
    QPushButton *close = buttons->addButton(tr("Fermer"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);
    connect(m_startButton, &QPushButton::clicked, this, [this] {
        emit m_running ? stopRequested() : startRequested();
    });
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    setRunning(false);
    resize(460, sizeHint().height());
}

void TreasureGameDialog::setChoices(const std::vector<std::pair<int, QString>> &robots,
                                    const std::vector<std::pair<int, QString>> &zones)
{
    const int robot = selectedRobot(), zone = selectedZone();
    m_robotCombo->clear();
    for (const auto &[id, name] : robots)
        m_robotCombo->addItem(name, id);
    m_zoneCombo->clear();
    m_zoneCombo->addItem(tr("Toute l'image"), 0);
    for (const auto &[id, name] : zones)
        m_zoneCombo->addItem(name, id);
    m_robotCombo->setCurrentIndex(std::max(0, m_robotCombo->findData(robot)));
    m_zoneCombo->setCurrentIndex(std::max(0, m_zoneCombo->findData(zone)));
    m_hintLabel->setText(robots.empty() ? tr("Ajoutez d'abord le robot dans l'onglet <b>Suivi</b> "
                                             "(<i>Ajouter des objets</i>).")
                                        : QString());
    m_hintLabel->setVisible(robots.empty());
    m_hasRobots = !robots.empty();
    m_startButton->setEnabled(m_running || m_hasRobots);
}

int TreasureGameDialog::selectedRobot() const
{
    return m_robotCombo->currentData().toInt();
}

int TreasureGameDialog::selectedZone() const
{
    return m_zoneCombo->currentData().toInt();
}

int TreasureGameDialog::duration() const
{
    return m_durationCombo->currentData().toInt();
}

TreasureGame::Difficulty TreasureGameDialog::difficulty() const
{
    return TreasureGame::Difficulty(m_difficultyCombo->currentData().toInt());
}

void TreasureGameDialog::setRunning(bool running)
{
    m_running = running;
    m_startButton->setText(running ? tr("Arrêter") : tr("Démarrer"));
    m_startButton->setEnabled(running || m_hasRobots);
    for (QComboBox *combo : {m_robotCombo, m_zoneCombo, m_durationCombo, m_difficultyCombo})
        combo->setEnabled(!running);
    if (!running && m_stateLabel->text().isEmpty()) {
        m_stateLabel->setText(tr("Prêt"));
        m_stateLabel->setStyleSheet(QStringLiteral("background: #ddd; color: #444; border-radius: 6px;"));
    }
}

void TreasureGameDialog::showStopped()
{
    setRunning(false);
    m_stateLabel->setText(tr("Partie arrêtée"));
    m_stateLabel->setStyleSheet(QStringLiteral("background: #ddd; color: #444; border-radius: 6px;"));
    m_countersLabel->clear();
}

void TreasureGameDialog::showState(const TreasureGame &game, const QString &robotName, qint64 timeMs)
{
    QString text;
    if (game.phase() == TreasureGame::Over)
        text = tr("TERMINÉ !");
    else if (game.recentFind(timeMs))
        text = tr("TRÉSOR TROUVÉ !");
    else
        text = TreasureGame::hintName(game.hint());
    m_stateLabel->setText(text);
    m_stateLabel->setStyleSheet(QStringLiteral("background: %1; color: white; border-radius: 6px;")
                                    .arg(game.bannerColor(timeMs).name()));
    const int s = int(std::ceil(game.remaining(timeMs)));
    m_countersLabel->setText(tr("%1 — trésors trouvés : %2 — temps restant : %3:%4")
                                 .arg(robotName)
                                 .arg(game.found())
                                 .arg(s / 60)
                                 .arg(s % 60, 2, 10, QLatin1Char('0')));
}
