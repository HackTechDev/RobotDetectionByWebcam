#pragma once

#include "gameframework.h"

// Jeu « Chat et souris » : deux objets suivis jouent l'un contre l'autre. Le chat marque un point
// quand les deux cadres se touchent, sauf si la souris est au refuge (zone facultative). Après une
// prise, il faut se séparer (et 3 s de répit) avant une nouvelle prise. Les rôles s'inversent à
// intervalle régulier ; le plus de points à la fin du temps gagne.
class CatMouseGame : public Game
{
public:
    QString title() const override { return QObject::tr("Chat et souris"); }
    QString key() const override { return QStringLiteral("cat_mouse"); }
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
    int catId() const { return m_swapped ? m_b : m_a; }
    int mouseId() const { return m_swapped ? m_a : m_b; }
    QString nameOf(const GameContext &context, int id) const;
    double remaining(qint64 timeMs) const;
    double nextSwap(qint64 timeMs) const;

    int m_a = 0, m_b = 0;       // objets suivis des deux joueurs (a commence chat)
    int m_scoreA = 0, m_scoreB = 0;
    bool m_swapped = false;
    qint64 m_startMs = 0;
    qint64 m_durationMs = 120000;
    qint64 m_swapMs = 30000;
    qint64 m_lastSwapMs = 0;
    bool m_contact = false;     // cadres en contact
    bool m_needSeparation = false;
    qint64 m_lastCatchMs = -10000;
    bool m_mouseSafe = false;   // souris au refuge
    bool m_finished = false;
    QString m_catchText;
};
