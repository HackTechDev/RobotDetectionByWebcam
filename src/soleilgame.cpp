#include "soleilgame.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QRandomGenerator>
#include <QVBoxLayout>

#include <cmath>

namespace {

constexpr qint64 kCountdownMs = 3000;
constexpr qint64 kFinishConfirmMs = 200;

struct Timing
{
    double greenMin, greenMax;  // durée du feu vert, en secondes
    double redMin, redMax;      // durée de « Soleil ! »
    double reaction;            // temps laissé pour s'arrêter après « Soleil ! »
};

Timing timingFor(SoleilGame::Difficulty d)
{
    switch (d) {
    case SoleilGame::Easy: return {4.0, 7.0, 2.0, 3.0, 0.8};
    case SoleilGame::Hard: return {1.0, 3.0, 2.0, 5.0, 0.3};
    default: return {2.0, 5.0, 2.0, 4.0, 0.5};
    }
}

QString seconds(double s)
{
    return QLocale().toString(s, 'f', 1);
}

} // namespace

double SoleilGame::randomDuration(double minSeconds, double maxSeconds) const
{
    return minSeconds + QRandomGenerator::global()->generateDouble() * (maxSeconds - minSeconds);
}

void SoleilGame::start(qint64 timeMs)
{
    m_phase = Countdown;
    m_phaseStartMs = timeMs;
    m_phaseEndMs = timeMs + kCountdownMs;
    m_startMs = m_phaseEndMs;
    m_faults = 0;
    m_faultThisRed = false;
    m_anchorValid = false;
    m_finishSinceMs = -1;
    m_robotFound = false;
}

void SoleilGame::stop()
{
    m_phase = Stopped;
}

double SoleilGame::phaseRemaining(qint64 timeMs) const
{
    return std::max<qint64>(0, m_phaseEndMs - timeMs) / 1000.0;
}

double SoleilGame::elapsed(qint64 timeMs) const
{
    if (m_phase == Stopped || m_phase == Countdown)
        return 0.0;
    return ((m_phase == Won ? m_wonMs : timeMs) - m_startMs) / 1000.0;
}

std::vector<MotionEvent> SoleilGame::update(const TrackedObject *robot, const QString &robotName,
                                            const std::vector<Zone> &zones, const QSize &imageSize, qint64 timeMs)
{
    std::vector<MotionEvent> events;
    if (!isRunning())
        return events;
    m_robotFound = robot && robot->found;
    const Timing timing = timingFor(m_difficulty);
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
    auto startGreen = [&] {
        m_phase = Green;
        m_phaseStartMs = timeMs;
        m_phaseEndMs = timeMs + qint64(randomDuration(timing.greenMin, timing.greenMax) * 1000);
        makeEvent(MotionEvent::GameGreen);
    };

    // Fin de phase : compte à rebours -> vert, vert -> « Soleil ! », « Soleil ! » -> vert.
    if (timeMs >= m_phaseEndMs) {
        if (m_phase == Red || m_phase == Countdown) {
            startGreen();
        } else if (m_phase == Green) {
            m_phase = Red;
            m_phaseStartMs = timeMs;
            m_phaseEndMs = timeMs + qint64(randomDuration(timing.redMin, timing.redMax) * 1000);
            m_faultThisRed = false;
            m_anchorValid = false;
            makeEvent(MotionEvent::GameRed);
        }
    }

    // Pendant « Soleil ! », après le temps de réaction : le robot ne doit plus bouger.
    if (m_phase == Red && m_robotFound && timeMs - m_phaseStartMs >= qint64(timing.reaction * 1000)) {
        const bool onFloor = robot->hasFloor;
        const QPointF pos = onFloor ? robot->floorPosition : robot->center;
        if (!m_anchorValid || m_anchorOnFloor != onFloor) {
            m_anchor = pos;
            m_anchorOnFloor = onFloor;
            m_anchorValid = true;
        } else if (!m_faultThisRed) {
            const QPointF d = pos - m_anchor;
            if (std::hypot(d.x(), d.y()) > (onFloor ? m_thresholdMm : m_thresholdPx)) {
                m_faultThisRed = true;
                ++m_faults;
                m_lastFaultMs = timeMs;
                makeEvent(MotionEvent::GameFault).count = m_faults;
            }
        }
    }

    // Arrivée : robot entièrement dans la zone, confirmé après 0,2 s.
    bool inFinish = false;
    if (m_phase != Countdown && m_robotFound) {
        for (const Zone &z : zones) {
            if (z.id == m_finishZoneId && ZoneManager::imageRect(z, imageSize).contains(robot->box))
                inFinish = true;
        }
    }
    if (!inFinish) {
        m_finishSinceMs = -1;
    } else if (m_finishSinceMs < 0) {
        m_finishSinceMs = timeMs;
    } else if (timeMs - m_finishSinceMs >= kFinishConfirmMs) {
        m_phase = Won;
        m_wonMs = timeMs;
        MotionEvent &e = makeEvent(MotionEvent::GameWin);
        e.count = m_faults;
        e.duration = elapsed(timeMs);
        for (const Zone &z : zones) {
            if (z.id == m_finishZoneId)
                e.zoneName = z.name;
        }
        e.zoneId = m_finishZoneId;
    }
    return events;
}

QString SoleilGame::bannerText(const QString &robotName, qint64 timeMs) const
{
    switch (m_phase) {
    case Countdown:
        return QObject::tr("1, 2, 3 Soleil — départ dans %1").arg(int(std::ceil(phaseRemaining(timeMs))));
    case Green:
        return recentFault(timeMs) ? QObject::tr("FAUTE ! +%1 s").arg(int(kPenaltySeconds)) : QObject::tr("AVANCEZ !");
    case Red:
        return recentFault(timeMs) ? QObject::tr("FAUTE ! %1 a bougé").arg(robotName) : QObject::tr("SOLEIL !");
    case Won:
        return QObject::tr("GAGNÉ ! %1 s — %2 faute(s) — score %3 s")
            .arg(seconds(elapsed(timeMs)))
            .arg(m_faults)
            .arg(seconds(score(timeMs)));
    case Stopped:
        break;
    }
    return QString();
}

QColor SoleilGame::bannerColor(qint64 timeMs) const
{
    if (recentFault(timeMs) && m_phase != Won)
        return QColor(150, 20, 140);
    switch (m_phase) {
    case Countdown: return QColor(90, 90, 90);
    case Green: return QColor(26, 154, 58);
    case Red: return QColor(201, 48, 44);
    case Won: return QColor(30, 100, 200);
    case Stopped: break;
    }
    return QColor(90, 90, 90);
}

QJsonObject SoleilGame::toJson(const QString &robotName, qint64 timeMs) const
{
    static const char *phases[] = {"stopped", "countdown", "green", "red", "won"};
    return QJsonObject{{QStringLiteral("mode"), QStringLiteral("soleil")},
                       {QStringLiteral("robot_id"), m_robotId},
                       {QStringLiteral("robot"), robotName},
                       {QStringLiteral("finish_zone_id"), m_finishZoneId},
                       {QStringLiteral("phase"), QString::fromLatin1(phases[m_phase])},
                       {QStringLiteral("phase_remaining_s"), std::round(phaseRemaining(timeMs) * 10) / 10},
                       {QStringLiteral("found"), m_robotFound},
                       {QStringLiteral("elapsed_s"), std::round(elapsed(timeMs) * 10) / 10},
                       {QStringLiteral("faults"), m_faults},
                       {QStringLiteral("score_s"), std::round(score(timeMs) * 10) / 10}};
}

SoleilGameDialog::SoleilGameDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Jeu : 1, 2, 3 Soleil"));
    auto *layout = new QVBoxLayout(this);
    auto *rules = new QLabel(tr("Le robot doit atteindre la zone d'arrivée (entièrement dedans) en n'avançant "
                                "qu'au feu vert. Bouger pendant « Soleil ! » compte une faute : +%1 s.")
                                 .arg(int(SoleilGame::kPenaltySeconds)),
                             this);
    rules->setWordWrap(true);
    layout->addWidget(rules);
    auto *form = new QFormLayout;
    m_robotCombo = new QComboBox(this);
    m_robotCombo->setToolTip(tr("Objet suivi qui joue le rôle du robot (onglet Suivi)"));
    form->addRow(tr("Robot"), m_robotCombo);
    m_zoneCombo = new QComboBox(this);
    m_zoneCombo->setToolTip(tr("Zone à atteindre (onglet Zones)"));
    form->addRow(tr("Arrivée"), m_zoneCombo);
    m_difficultyCombo = new QComboBox(this);
    m_difficultyCombo->addItem(tr("Facile (feu vert long, 0,8 s pour s'arrêter)"), SoleilGame::Easy);
    m_difficultyCombo->addItem(tr("Normal (0,5 s pour s'arrêter)"), SoleilGame::Normal);
    m_difficultyCombo->addItem(tr("Difficile (feu vert court, 0,3 s pour s'arrêter)"), SoleilGame::Hard);
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

void SoleilGameDialog::setChoices(const std::vector<std::pair<int, QString>> &robots,
                                  const std::vector<std::pair<int, QString>> &zones)
{
    const int robot = selectedRobot(), zone = selectedZone();
    m_robotCombo->clear();
    for (const auto &[id, name] : robots)
        m_robotCombo->addItem(name, id);
    m_zoneCombo->clear();
    for (const auto &[id, name] : zones)
        m_zoneCombo->addItem(name, id);
    m_robotCombo->setCurrentIndex(std::max(0, m_robotCombo->findData(robot)));
    m_zoneCombo->setCurrentIndex(std::max(0, m_zoneCombo->findData(zone)));
    QStringList hints;
    if (robots.empty())
        hints << tr("Ajoutez d'abord le robot dans l'onglet <b>Suivi</b> (<i>Ajouter des objets</i>).");
    if (zones.empty())
        hints << tr("Tracez d'abord la zone d'arrivée dans l'onglet <b>Zones</b> (<i>Ajouter une zone</i>).");
    m_hintLabel->setText(hints.join(QStringLiteral("<br>")));
    m_hintLabel->setVisible(!hints.isEmpty());
    m_hasChoices = !robots.empty() && !zones.empty();
    m_startButton->setEnabled(m_running || m_hasChoices);
}

int SoleilGameDialog::selectedRobot() const
{
    return m_robotCombo->currentData().toInt();
}

int SoleilGameDialog::selectedZone() const
{
    return m_zoneCombo->currentData().toInt();
}

SoleilGame::Difficulty SoleilGameDialog::difficulty() const
{
    return SoleilGame::Difficulty(m_difficultyCombo->currentData().toInt());
}

void SoleilGameDialog::setRunning(bool running)
{
    m_running = running;
    m_startButton->setText(running ? tr("Arrêter") : tr("Démarrer"));
    m_startButton->setEnabled(running || m_hasChoices);
    // Choix figés pendant la partie.
    m_robotCombo->setEnabled(!running);
    m_zoneCombo->setEnabled(!running);
    m_difficultyCombo->setEnabled(!running);
    if (!running && m_stateLabel->text().isEmpty()) {
        m_stateLabel->setText(tr("Prêt"));
        m_stateLabel->setStyleSheet(QStringLiteral("background: #ddd; color: #444; border-radius: 6px;"));
    }
}

void SoleilGameDialog::showStopped()
{
    setRunning(false);
    m_stateLabel->setText(tr("Partie arrêtée"));
    m_stateLabel->setStyleSheet(QStringLiteral("background: #ddd; color: #444; border-radius: 6px;"));
    m_countersLabel->clear();
}

void SoleilGameDialog::showState(const SoleilGame &game, const QString &robotName, qint64 timeMs)
{
    QString text = game.bannerText(robotName, timeMs);
    if (game.phase() == SoleilGame::Won)
        text = tr("GAGNÉ !");
    if (!game.robotFound() && game.isRunning() && game.phase() != SoleilGame::Countdown)
        text += tr("\n(robot non trouvé)");
    m_stateLabel->setText(text);
    m_stateLabel->setStyleSheet(QStringLiteral("background: %1; color: white; border-radius: 6px;")
                                    .arg(game.bannerColor(timeMs).name()));
    QString counters = tr("Chrono : %1 s — Fautes : %2 — Score : %3 s")
                           .arg(QLocale().toString(game.elapsed(timeMs), 'f', 1))
                           .arg(game.faults())
                           .arg(QLocale().toString(game.score(timeMs), 'f', 1));
    if (game.phase() == SoleilGame::Green || game.phase() == SoleilGame::Red)
        counters += tr("\nChangement dans %1 s").arg(QLocale().toString(game.phaseRemaining(timeMs), 'f', 1));
    m_countersLabel->setText(counters);
}
