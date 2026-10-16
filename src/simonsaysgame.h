#pragma once

#include "gameframework.h"

// Jeu « Jacques a dit » : l'application annonce des ordres (avance, recule, tourne à gauche, tourne à
// droite) que le robot (marqueur ArUco, qui donne le cap) doit exécuter dans le temps imparti. Un ordre
// sans « Jacques a dit » est un piège : le robot ne doit pas bouger. Chaque ordre est jugé à la fin du
// temps, en comparant la position et le cap du robot au moment de l'annonce et à la fin.
class SimonSaysGame : public Game
{
public:
    QString title() const override { return QObject::tr("Jacques a dit"); }
    QString key() const override { return QStringLiteral("simon_says"); }
    QString rules() const override;
    std::vector<GameParam> params() const override;
    QString bannerText(const GameContext &context) const override;
    QColor bannerColor(const GameContext &context) const override;
    QString countersText(const GameContext &context) const override;
    QJsonObject toJson(const GameContext &context) const override;

protected:
    void onStart(const GameContext &context) override;
    std::vector<MotionEvent> onUpdate(const GameContext &context) override;

private:
    enum Order { Forward, Backward, TurnLeft, TurnRight };
    enum Phase { Pause, Announce, Result, Finished };
    static QString orderText(Order order);
    void announce(qint64 timeMs);

    int m_marker = 0;
    int m_ordersTotal = 10;
    qint64 m_windowMs = 4000;
    int m_done = 0;
    int m_score = 0;
    Phase m_phase = Pause;
    qint64 m_phaseMs = 0;
    Order m_order = Forward;
    bool m_simon = true;      // l'ordre commence par « Jacques a dit »
    bool m_hasReference = false;
    QPointF m_refPos;         // pose à l'annonce
    double m_refAngle = 0;
    double m_refSide = 0;
    bool m_lastGood = false;
    QString m_result;
    bool m_seen = false;
};
