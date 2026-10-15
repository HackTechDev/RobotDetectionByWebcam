#pragma once

#include "objecttracker.h"
#include "zonemanager.h"

#include <QColor>
#include <QDialog>
#include <QJsonObject>
#include <QPointF>
#include <QString>
#include <vector>

class QComboBox;
class QLabel;
class QPushButton;

// Jeu « 1, 2, 3 Soleil » : le robot (un objet suivi) doit atteindre la zone d'arrivée (entièrement
// dedans) en n'avançant qu'au feu vert. Les phases « Avancez ! » et « Soleil ! » alternent avec des
// durées tirées au hasard. Après un court temps de réaction, bouger pendant « Soleil ! » (plus que
// le seuil de mouvement du suivi) compte une faute, qui ajoute 5 s au score.
class SoleilGame
{
public:
    enum Phase { Stopped, Countdown, Green, Red, Won };
    enum Difficulty { Easy, Normal, Hard };

    void setRobot(int trackedId) { m_robotId = trackedId; }
    int robotId() const { return m_robotId; }
    void setFinishZone(int zoneId) { m_finishZoneId = zoneId; }
    int finishZoneId() const { return m_finishZoneId; }
    void setDifficulty(Difficulty difficulty) { m_difficulty = difficulty; }
    // Seuils de mouvement : en mm sur le sol (repère défini) ou en pixels.
    void setThresholds(double floorMm, double imagePx)
    {
        m_thresholdMm = floorMm;
        m_thresholdPx = imagePx;
    }

    void start(qint64 timeMs);
    void stop();
    bool isRunning() const { return m_phase != Stopped && m_phase != Won; }
    bool isActive() const { return m_phase != Stopped; }  // en cours ou terminé (résultat affiché)

    std::vector<MotionEvent> update(const TrackedObject *robot, const QString &robotName,
                                    const std::vector<Zone> &zones, const QSize &imageSize, qint64 timeMs);

    Phase phase() const { return m_phase; }
    bool robotFound() const { return m_robotFound; }
    double phaseRemaining(qint64 timeMs) const;  // en secondes
    double elapsed(qint64 timeMs) const;         // chrono, en secondes (figé à l'arrivée)
    int faults() const { return m_faults; }
    double penalty() const { return m_faults * kPenaltySeconds; }
    double score(qint64 timeMs) const { return elapsed(timeMs) + penalty(); }
    // Faute toute récente : à signaler en grand.
    bool recentFault(qint64 timeMs) const { return m_faults > 0 && timeMs - m_lastFaultMs < 1500; }

    // Texte et couleur du bandeau affiché sur l'image.
    QString bannerText(const QString &robotName, qint64 timeMs) const;
    QColor bannerColor(qint64 timeMs) const;
    QJsonObject toJson(const QString &robotName, qint64 timeMs) const;

    static constexpr double kPenaltySeconds = 5.0;

private:
    double randomDuration(double minSeconds, double maxSeconds) const;

    int m_robotId = 0;
    int m_finishZoneId = 0;
    Difficulty m_difficulty = Normal;
    double m_thresholdMm = 20.0;
    double m_thresholdPx = 15.0;

    Phase m_phase = Stopped;
    qint64 m_startMs = 0;        // départ du chrono (fin du compte à rebours)
    qint64 m_phaseStartMs = 0;
    qint64 m_phaseEndMs = 0;
    qint64 m_wonMs = 0;
    bool m_robotFound = false;
    int m_faults = 0;
    qint64 m_lastFaultMs = 0;
    bool m_faultThisRed = false;
    bool m_anchorValid = false;  // position de référence pendant « Soleil ! »
    bool m_anchorOnFloor = false;
    QPointF m_anchor;
    qint64 m_finishSinceMs = -1;  // robot entièrement dans l'arrivée depuis (confirmation)
};

// Fenêtre du jeu « 1, 2, 3 Soleil » : robot, zone d'arrivée, difficulté, état et chrono.
class SoleilGameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SoleilGameDialog(QWidget *parent = nullptr);

    void setChoices(const std::vector<std::pair<int, QString>> &robots,
                    const std::vector<std::pair<int, QString>> &zones);
    int selectedRobot() const;
    int selectedZone() const;
    SoleilGame::Difficulty difficulty() const;
    void showState(const SoleilGame &game, const QString &robotName, qint64 timeMs);
    void setRunning(bool running);
    // Partie interrompue (bouton Arrêter, ou autre jeu démarré).
    void showStopped();

signals:
    void startRequested();
    void stopRequested();

private:
    QComboBox *m_robotCombo = nullptr;
    QComboBox *m_zoneCombo = nullptr;
    QComboBox *m_difficultyCombo = nullptr;
    QLabel *m_stateLabel = nullptr;
    QLabel *m_countersLabel = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    bool m_running = false;
    bool m_hasChoices = false;
};
