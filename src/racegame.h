#pragma once

#include "gameframework.h"

#include <QPolygonF>

// Jeu « Course contre la montre » : des portes (zones) à franchir dans l'ordre, sur un ou plusieurs
// tours. Le chrono démarre au premier mouvement du robot et s'arrête à la dernière porte du dernier
// tour. Une mauvaise porte (pas celle attendue) coûte 5 s. Le meilleur temps est conservé.
class RaceGame : public Game
{
public:
    QString title() const override { return QObject::tr("Course contre la montre"); }
    QString key() const override { return QStringLiteral("race"); }
    QString rules() const override;
    std::vector<GameParam> params() const override;
    QString checkReady(const GameSettings &settings, const GameContext &context) const override;
    void draw(const GameDrawContext &draw, const GameContext &context) const override;
    QString bannerText(const GameContext &context) const override;
    QColor bannerColor(const GameContext &context) const override;
    QString countersText(const GameContext &context) const override;
    QJsonObject toJson(const GameContext &context) const override;

    static constexpr double kPenaltySeconds = 5.0;

protected:
    void onStart(const GameContext &context) override;
    std::vector<MotionEvent> onUpdate(const GameContext &context) override;

private:
    double elapsed(qint64 timeMs) const;  // chrono, pénalités comprises

    QList<int> m_gates;
    int m_laps = 1;
    int m_next = 0;          // rang de la prochaine porte
    int m_lap = 0;           // tours terminés
    int m_insideGate = -1;   // porte (rang) où se trouve le robot
    bool m_started = false;
    bool m_hasAnchor = false;
    QPointF m_anchor;
    qint64 m_startMs = 0;
    qint64 m_finishMs = 0;
    bool m_finished = false;
    int m_penalties = 0;
    qint64 m_lastPenaltyMs = -10000;
    double m_best = 0;       // meilleur temps du parcours en cours (0 : aucun)
    QMap<QString, double> m_bests;  // meilleur temps par parcours (portes et nombre de tours)
    QString m_course;
    bool m_newRecord = false;
    QPolygonF m_trail;       // tracé de la course en cours
};
