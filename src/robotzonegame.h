#pragma once

#include "objecttracker.h"
#include "zonemanager.h"

#include <QDialog>
#include <QString>
#include <vector>

class QComboBox;
class QLabel;
class QPushButton;

// Jeu « Détection robot dans zone » : le robot (un objet suivi) est dans la zone quand son cadre y
// est entièrement contenu. L'état ne change qu'après 0,2 s de stabilité, pour qu'un cadre qui
// tremble sur le bord ne déclenche pas une suite d'entrées et de sorties.
class RobotZoneGame
{
public:
    enum State { Outside, Inside, Lost };

    void setRobot(int trackedId) { m_robotId = trackedId; }
    int robotId() const { return m_robotId; }
    // Zone visée (0 = n'importe quelle zone).
    void setZone(int zoneId) { m_zoneId = zoneId; }
    int zoneId() const { return m_zoneId; }

    void start(qint64 timeMs);
    void stop();
    void resetCounters(qint64 timeMs);
    bool isRunning() const { return m_running; }

    // Met à jour l'état ; « robot » est nul si l'objet n'est plus suivi. Retourne les entrées et
    // sorties survenues (événements GameEnter et GameExit).
    std::vector<MotionEvent> update(const TrackedObject *robot, const QString &robotName,
                                    const std::vector<Zone> &zones, const QSize &imageSize, qint64 timeMs);

    State state() const { return m_state; }
    bool robotFound() const { return m_robotFound; }
    int insideZoneId() const { return m_insideZoneId; }
    QString insideZoneName() const { return m_insideZoneName; }
    double timeInside(qint64 timeMs) const;  // en secondes, depuis le départ
    int entries() const { return m_entries; }

private:
    int m_robotId = 0;
    int m_zoneId = 0;
    bool m_running = false;
    State m_state = Outside;
    bool m_robotFound = false;
    int m_insideZoneId = 0;
    QString m_insideZoneName;
    int m_candidateZoneId = 0;      // zone où le robot est entièrement, à confirmer
    qint64 m_candidateSinceMs = 0;
    qint64 m_insideSinceMs = 0;
    qint64 m_accumulatedMs = 0;     // temps dans la zone, hors séjour en cours
    int m_entries = 0;
};

// Fenêtre du jeu : choix du robot et de la zone, état en grand, compteurs.
class RobotZoneGameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RobotZoneGameDialog(QWidget *parent = nullptr);

    // Objets suivis et zones disponibles : (numéro, nom). La sélection est conservée si possible.
    void setChoices(const std::vector<std::pair<int, QString>> &robots,
                    const std::vector<std::pair<int, QString>> &zones);
    int selectedRobot() const;
    int selectedZone() const;
    void showState(const RobotZoneGame &game, const QString &robotName, qint64 timeMs);
    void setRunning(bool running);

signals:
    void startRequested();
    void stopRequested();
    void resetRequested();
    void selectionChanged();

private:
    QComboBox *m_robotCombo = nullptr;
    QComboBox *m_zoneCombo = nullptr;
    QLabel *m_stateLabel = nullptr;
    QLabel *m_countersLabel = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    bool m_running = false;
};
