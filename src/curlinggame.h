#pragma once

#include "gameframework.h"

#include <QPointF>
#include <QVector>

// Jeu « Curling » : chaque joueur, à son tour, lance le robot vers la cible (une zone, avec trois
// anneaux : 10, 5 et 2 points). Seule la position à l'arrêt compte. Entre deux lancers, on replace
// le robot : le lancer suivant n'est accepté qu'après 1 s d'immobilité. Plusieurs manches ; le
// classement s'affiche à la fin.
class CurlingGame : public Game
{
public:
    QString title() const override { return QObject::tr("Curling"); }
    QString key() const override { return QStringLiteral("curling"); }
    QString rules() const override;
    std::vector<GameParam> params() const override;
    void draw(const GameDrawContext &draw, const GameContext &context) const override;
    QString bannerText(const GameContext &context) const override;
    QColor bannerColor(const GameContext &context) const override;
    QString countersText(const GameContext &context) const override;
    QJsonObject toJson(const GameContext &context) const override;

protected:
    void onStart(const GameContext &context) override;
    std::vector<MotionEvent> onUpdate(const GameContext &context) override;

private:
    enum State { Placing, Ready, Moving, Result, Finished };
    int pointsAt(const QPointF &p, const GameContext &context) const;
    QString playerName(int player) const;
    QString ranking() const;

    int m_players = 1;
    int m_rounds = 1;
    int m_throw = 0;          // lancers joués
    State m_state = Placing;
    QPointF m_stillPos;
    qint64 m_stillSinceMs = -1;
    QPointF m_anchor;         // position de départ du lancer
    qint64 m_stateMs = 0;
    qint64 m_lostSinceMs = -1;
    QVector<int> m_scores;
    int m_lastPoints = 0;
    struct Stone
    {
        QPointF pos;
        int player;
    };
    QVector<Stone> m_stones;  // positions d'arrêt des lancers de la manche
    bool m_seen = false;
};
