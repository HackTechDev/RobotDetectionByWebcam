#pragma once

#include "gameframework.h"

#include <QMap>

// Jeu « Ramassage » : le robot pousse le plus d'objets possible dans la zone de collecte avant la fin
// du temps. Les objets sont tous les objets suivis autres que le robot ; un objet est ramassé quand
// son centre est dans la zone (entrées et sorties confirmées après 0,3 s). Le compte se fait sur la
// position des objets, pas sur celle du robot.
class CollectGame : public Game
{
public:
    QString title() const override { return QObject::tr("Ramassage"); }
    QString key() const override { return QStringLiteral("collect"); }
    QString rules() const override;
    std::vector<GameParam> params() const override;
    QString checkReady(const GameSettings &settings, const GameContext &context) const override;
    void draw(const GameDrawContext &draw, const GameContext &context) const override;
    QString bannerText(const GameContext &context) const override;
    QColor bannerColor(const GameContext &context) const override;
    QString countersText(const GameContext &context) const override;
    QJsonObject toJson(const GameContext &context) const override;

protected:
    void onStart(const GameContext &context) override;
    std::vector<MotionEvent> onUpdate(const GameContext &context) override;

private:
    double remaining(qint64 timeMs) const;
    int collected() const;

    int m_robot = 0;
    int m_zone = 0;
    qint64 m_startMs = 0;
    qint64 m_durationMs = 120000;
    struct ItemState
    {
        bool inside = false;
        bool candidate = false;
        qint64 sinceMs = 0;
    };
    QMap<int, ItemState> m_items;  // objets à ramasser (numéro d'objet suivi -> état)
    int m_total = 0;
    bool m_finished = false;
    qint64 m_lastChangeMs = -10000;
    QString m_lastChange;
    bool m_lastGood = true;
};
