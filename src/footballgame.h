#pragma once

#include "gameframework.h"

// Jeu « Football de table » : le robot pousse une balle (objet suivi) vers les buts (deux zones).
// But : centre de la balle dans un but pendant 0,2 s ; il compte pour l'équipe adverse. Balle sortie
// de l'image (« hors du champ » du suivi) : touche. Après un but ou une touche, le jeu reprend quand
// la balle est replacée (hors des buts, visible) et immobile depuis 1 s.
class FootballGame : public Game
{
public:
    QString title() const override { return QObject::tr("Football de table"); }
    QString key() const override { return QStringLiteral("football"); }
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
    enum State { Play, Goal, Out, Finished };
    double remaining(qint64 timeMs) const;

    int m_ball = 0;
    int m_goal1 = 0, m_goal2 = 0;  // buts des équipes 1 et 2 (une balle dans le but 1 marque pour l'équipe 2)
    int m_score1 = 0, m_score2 = 0;
    State m_state = Play;
    qint64 m_startMs = 0;
    qint64 m_durationMs = 180000;
    qint64 m_stateMs = 0;
    int m_goalCandidate = 0;
    qint64 m_goalSinceMs = -1;
    QPointF m_stillPos;
    qint64 m_stillSinceMs = -1;
    int m_lastScorer = 0;
    int m_outs = 0;
};
