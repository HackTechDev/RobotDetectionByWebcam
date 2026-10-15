#include "robotzonegame.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

constexpr qint64 kConfirmMs = 200;

} // namespace

void RobotZoneGame::start(qint64 timeMs)
{
    m_running = true;
    m_state = Outside;
    m_robotFound = false;
    m_insideZoneId = 0;
    m_insideZoneName.clear();
    m_candidateZoneId = 0;
    m_candidateSinceMs = timeMs;
    resetCounters(timeMs);
}

void RobotZoneGame::stop()
{
    m_running = false;
}

void RobotZoneGame::resetCounters(qint64 timeMs)
{
    m_accumulatedMs = 0;
    m_entries = 0;
    m_insideSinceMs = timeMs;
}

double RobotZoneGame::timeInside(qint64 timeMs) const
{
    qint64 ms = m_accumulatedMs;
    if (m_state == Inside)
        ms += timeMs - m_insideSinceMs;
    return ms / 1000.0;
}

std::vector<MotionEvent> RobotZoneGame::update(const TrackedObject *robot, const QString &robotName,
                                               const std::vector<Zone> &zones, const QSize &imageSize,
                                               qint64 timeMs)
{
    std::vector<MotionEvent> events;
    if (!m_running)
        return events;
    // Position prédite (cadre estimé) : ni entrée ni sortie, on garde l'état.
    if (robot && !robot->found && robot->predicted)
        return events;
    m_robotFound = robot && robot->found;

    // Zone qui contient entièrement le cadre du robot (0 : aucune).
    int zoneNow = 0;
    if (m_robotFound) {
        for (const Zone &z : zones) {
            if (m_zoneId != 0 && z.id != m_zoneId)
                continue;
            if (ZoneManager::imageRect(z, imageSize).contains(robot->box)) {
                zoneNow = z.id;
                break;
            }
        }
    }
    if (zoneNow != m_candidateZoneId) {
        m_candidateZoneId = zoneNow;
        m_candidateSinceMs = timeMs;
    }

    auto makeEvent = [&](MotionEvent::Type type, int zoneId, const QString &zoneName) {
        MotionEvent e;
        e.type = type;
        e.timeMs = timeMs;
        if (robot) {
            e.onFloor = robot->hasFloor;
            e.position = robot->hasFloor ? robot->floorPosition : robot->center;
        }
        e.objectId = m_robotId;
        e.objectName = robotName;
        e.zoneId = zoneId;
        e.zoneName = zoneName;
        events.push_back(e);
    };

    // Changement confirmé après 0,2 s de stabilité.
    if (timeMs - m_candidateSinceMs >= kConfirmMs && m_candidateZoneId != m_insideZoneId) {
        if (m_insideZoneId != 0) {
            makeEvent(MotionEvent::GameExit, m_insideZoneId, m_insideZoneName);
            m_accumulatedMs += timeMs - m_insideSinceMs;
        }
        m_insideZoneId = m_candidateZoneId;
        m_insideZoneName.clear();
        for (const Zone &z : zones) {
            if (z.id == m_insideZoneId)
                m_insideZoneName = z.name;
        }
        if (m_insideZoneId != 0) {
            makeEvent(MotionEvent::GameEnter, m_insideZoneId, m_insideZoneName);
            m_insideSinceMs = timeMs;
            ++m_entries;
        }
    }
    m_state = m_insideZoneId != 0 ? Inside : (m_robotFound ? Outside : Lost);
    return events;
}

RobotZoneGameDialog::RobotZoneGameDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Jeu : détection du robot dans une zone"));
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    m_robotCombo = new QComboBox(this);
    m_robotCombo->setToolTip(tr("Objet suivi qui joue le rôle du robot (onglet Suivi)"));
    form->addRow(tr("Robot"), m_robotCombo);
    m_zoneCombo = new QComboBox(this);
    m_zoneCombo->setToolTip(tr("Zone à atteindre (onglet Zones)"));
    form->addRow(tr("Zone"), m_zoneCombo);
    layout->addLayout(form);

    m_stateLabel = new QLabel(this);
    m_stateLabel->setAlignment(Qt::AlignCenter);
    m_stateLabel->setMinimumHeight(70);
    m_stateLabel->setWordWrap(true);
    QFont big = m_stateLabel->font();
    big.setPointSizeF(big.pointSizeF() * 1.8);
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
    QPushButton *reset = buttons->addButton(tr("Remettre à zéro"), QDialogButtonBox::ResetRole);
    reset->setToolTip(tr("Remet à zéro le temps passé dans la zone et le nombre d'entrées"));
    QPushButton *close = buttons->addButton(tr("Fermer"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);
    connect(m_startButton, &QPushButton::clicked, this, [this] {
        emit m_running ? stopRequested() : startRequested();
    });
    connect(reset, &QPushButton::clicked, this, &RobotZoneGameDialog::resetRequested);
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    connect(m_robotCombo, &QComboBox::currentIndexChanged, this, &RobotZoneGameDialog::selectionChanged);
    connect(m_zoneCombo, &QComboBox::currentIndexChanged, this, &RobotZoneGameDialog::selectionChanged);
    setRunning(false);
    resize(420, sizeHint().height());
}

void RobotZoneGameDialog::setChoices(const std::vector<std::pair<int, QString>> &robots,
                                     const std::vector<std::pair<int, QString>> &zones)
{
    const int robot = selectedRobot(), zone = selectedZone();
    {
        const QSignalBlocker b1(m_robotCombo), b2(m_zoneCombo);
        m_robotCombo->clear();
        for (const auto &[id, name] : robots)
            m_robotCombo->addItem(name, id);
        m_zoneCombo->clear();
        m_zoneCombo->addItem(tr("N'importe quelle zone"), 0);
        for (const auto &[id, name] : zones)
            m_zoneCombo->addItem(name, id);
        m_robotCombo->setCurrentIndex(std::max(0, m_robotCombo->findData(robot)));
        m_zoneCombo->setCurrentIndex(std::max(0, m_zoneCombo->findData(zone)));
    }
    QStringList hints;
    if (robots.empty())
        hints << tr("Ajoutez d'abord le robot dans l'onglet <b>Suivi</b> (<i>Ajouter des objets</i>).");
    if (zones.empty())
        hints << tr("Tracez d'abord une zone dans l'onglet <b>Zones</b> (<i>Ajouter une zone</i>).");
    m_hintLabel->setText(hints.join(QStringLiteral("<br>")));
    m_hintLabel->setVisible(!hints.isEmpty());
    m_startButton->setEnabled(m_running || (!robots.empty() && !zones.empty()));
    if (selectedRobot() != robot || selectedZone() != zone)
        emit selectionChanged();
}

int RobotZoneGameDialog::selectedRobot() const
{
    return m_robotCombo->currentData().toInt();
}

int RobotZoneGameDialog::selectedZone() const
{
    return m_zoneCombo->currentData().toInt();
}

void RobotZoneGameDialog::setRunning(bool running)
{
    m_running = running;
    m_startButton->setText(running ? tr("Arrêter") : tr("Démarrer"));
    if (!running) {
        m_stateLabel->setText(tr("Jeu arrêté"));
        m_stateLabel->setStyleSheet(QStringLiteral("background: #ddd; color: #444; border-radius: 6px;"));
        m_countersLabel->clear();
    }
}

void RobotZoneGameDialog::showState(const RobotZoneGame &game, const QString &robotName, qint64 timeMs)
{
    QString text, style;
    switch (game.state()) {
    case RobotZoneGame::Inside:
        text = tr("%1 est dans la zone « %2 »").arg(robotName, game.insideZoneName());
        style = QStringLiteral("background: #1a9a3a; color: white;");
        break;
    case RobotZoneGame::Outside:
        text = tr("%1 est hors zone").arg(robotName);
        style = QStringLiteral("background: #c9302c; color: white;");
        break;
    case RobotZoneGame::Lost:
        text = tr("%1 n'est pas trouvé").arg(robotName);
        style = QStringLiteral("background: #e08e0b; color: white;");
        break;
    }
    m_stateLabel->setText(text);
    m_stateLabel->setStyleSheet(style + QStringLiteral(" border-radius: 6px;"));
    m_countersLabel->setText(tr("Temps dans la zone : %1 s — Entrées : %2")
                                 .arg(QLocale().toString(game.timeInside(timeMs), 'f', 1))
                                 .arg(game.entries()));
}
