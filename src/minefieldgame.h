#pragma once

#include "gameframework.h"

#include <QPolygonF>
#include <QVector>

// Jeu « Champ de mines » : des mines invisibles, placées au hasard à chaque partie sur le terrain (hors
// du départ et de l'arrivée), séparent le départ de l'arrivée. Le robot doit traverser sans en toucher
// (centre du robot à moins du rayon d'une mine). Une explosion reste marquée sur l'image, pour s'en
// souvenir, et il faut revenir au départ pour un nouvel essai. Option : détecteur qui « bipe » à
// l'approche. À la fin, toutes les mines et les trajectoires des essais sont dévoilées.
class MinefieldGame : public Game
{
public:
    QString title() const override { return QObject::tr("Champ de mines"); }
    QString key() const override { return QStringLiteral("minefield"); }
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
    enum Phase { Waiting, Ready, Crossing, Boom, Won };
    void placeMines(const GameContext &context);

    int m_robot = 0;
    int m_start = 0, m_finish = 0;
    bool m_detector = false;
    Phase m_phase = Waiting;
    QVector<QPointF> m_mines;
    double m_radius = 20;
    QVector<QPointF> m_explosions;    // mines qui ont explosé (restent marquées)
    QVector<QPolygonF> m_attempts;    // trajectoire de chaque essai
    int m_beep = 0;                   // 0 : rien, 1 : bip, 2 : bip bip
    qint64 m_boomMs = -10000;
    bool m_seen = false;
};
