#pragma once

#include "objecttracker.h"
#include "zonemanager.h"

#include <QColor>
#include <QDialog>
#include <QJsonObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <vector>

class QComboBox;
class QLabel;
class QPushButton;

// Jeu « Chasse au trésor » : un trésor invisible est caché au hasard sur le terrain (une zone ou
// toute l'image). Le robot (un objet suivi) est guidé par « chaud / froid » selon sa distance au
// trésor ; quand son centre passe à moins du rayon du trésor, celui-ci est trouvé et un nouveau est
// caché ailleurs. But : en trouver le plus possible avant la fin du temps.
class TreasureGame
{
public:
    enum Phase { Stopped, Playing, Over };
    enum Difficulty { Easy, Normal, Hard };
    enum Hint { Found, Burning, Hot, Warm, Cold, Freezing, Unknown };

    void setRobot(int trackedId) { m_robotId = trackedId; }
    int robotId() const { return m_robotId; }
    void setTerrainZone(int zoneId) { m_terrainZoneId = zoneId; }  // 0 : toute l'image
    int terrainZoneId() const { return m_terrainZoneId; }
    void setDifficulty(Difficulty difficulty) { m_difficulty = difficulty; }
    void setDuration(int seconds) { m_durationMs = qint64(seconds) * 1000; }

    void start(qint64 timeMs);
    void stop();
    bool isRunning() const { return m_phase == Playing; }
    bool isActive() const { return m_phase != Stopped; }

    std::vector<MotionEvent> update(const TrackedObject *robot, const QString &robotName,
                                    const std::vector<Zone> &zones, const QSize &imageSize, qint64 timeMs);

    Phase phase() const { return m_phase; }
    Hint hint() const { return m_hint; }
    int trend() const { return m_trend; }  // +1 : se rapproche, -1 : s'éloigne, 0 : stable
    int found() const { return m_found; }
    double remaining(qint64 timeMs) const;  // en secondes
    bool recentFind(qint64 timeMs) const { return m_found > 0 && timeMs - m_lastFindMs < 1500; }
    bool hasTreasure() const { return m_hasTreasure; }
    QPointF treasure() const { return m_treasure; }     // en pixels de l'image
    double treasureRadius() const { return m_radius; }  // en pixels
    const std::vector<QPointF> &foundPositions() const { return m_foundPositions; }

    static QString hintName(Hint hint);
    QString bannerText(const QString &robotName, qint64 timeMs) const;
    QColor bannerColor(qint64 timeMs) const;
    QJsonObject toJson(const QString &robotName, qint64 timeMs) const;

private:
    QRectF terrain(const std::vector<Zone> &zones, const QSize &imageSize) const;
    void hideTreasure(const QRectF &terrain, const QPointF &avoid);

    int m_robotId = 0;
    int m_terrainZoneId = 0;
    Difficulty m_difficulty = Normal;
    qint64 m_durationMs = 120000;

    Phase m_phase = Stopped;
    qint64 m_startMs = 0;
    qint64 m_endMs = 0;
    bool m_hasTreasure = false;
    QPointF m_treasure;
    double m_radius = 0;
    double m_diagonal = 0;  // diagonale du terrain, en pixels
    Hint m_hint = Unknown;
    int m_trend = 0;
    double m_lastDistance = -1;
    qint64 m_lastTrendMs = 0;
    bool m_robotFound = false;
    double m_distance = -1;
    int m_found = 0;
    qint64 m_lastFindMs = 0;
    std::vector<QPointF> m_foundPositions;
};

// Fenêtre du jeu « Chasse au trésor » : robot, terrain, durée, difficulté, indice et score.
class TreasureGameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TreasureGameDialog(QWidget *parent = nullptr);

    void setChoices(const std::vector<std::pair<int, QString>> &robots,
                    const std::vector<std::pair<int, QString>> &zones);
    int selectedRobot() const;
    int selectedZone() const;
    int duration() const;  // en secondes
    TreasureGame::Difficulty difficulty() const;
    void showState(const TreasureGame &game, const QString &robotName, qint64 timeMs);
    void setRunning(bool running);
    void showStopped();

signals:
    void startRequested();
    void stopRequested();

private:
    QComboBox *m_robotCombo = nullptr;
    QComboBox *m_zoneCombo = nullptr;
    QComboBox *m_durationCombo = nullptr;
    QComboBox *m_difficultyCombo = nullptr;
    QLabel *m_stateLabel = nullptr;
    QLabel *m_countersLabel = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    bool m_running = false;
    bool m_hasRobots = false;
};
