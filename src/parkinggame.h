#pragma once

#include "gameframework.h"

// Jeu « Le parking » : garer le robot (marqueur ArUco, qui donne le cap) dans une case avec le bon
// cap. Le stationnement est jugé à l'arrêt : marqueur entièrement dans la case cible et cap à moins
// de la tolérance. Cinq niveaux : la case cible rétrécit (de 100 % à 40 % de la zone) et la
// tolérance de cap se resserre (de 30° à 10°). Les points dépendent de la précision.
class ParkingGame : public Game
{
public:
    QString title() const override { return QObject::tr("Le parking"); }
    QString key() const override { return QStringLiteral("parking"); }
    QString rules() const override;
    std::vector<GameParam> params() const override;
    void draw(const GameDrawContext &draw, const GameContext &context) const override;
    QString bannerText(const GameContext &context) const override;
    QColor bannerColor(const GameContext &context) const override;
    QString countersText(const GameContext &context) const override;
    QJsonObject toJson(const GameContext &context) const override;

    static constexpr int kLevels = 5;

protected:
    void onStart(const GameContext &context) override;
    std::vector<MotionEvent> onUpdate(const GameContext &context) override;

private:
    QRectF target(const GameContext &context) const;  // case cible du niveau en cours
    double tolerance() const;                         // tolérance de cap du niveau, en degrés
    double targetHeading() const;                     // cap demandé (angle du marqueur)
    static double angleDiff(double a, double b);      // écart absolu, en degrés

    int m_level = 1;
    int m_score = 0;
    int m_attempts = 0;      // stationnements ratés
    bool m_finished = false;
    bool m_armed = true;     // le robot a bougé depuis le dernier jugement
    QPointF m_judgedAt;      // position et cap du dernier jugement
    double m_judgedAngle = 0;
    QPointF m_stillPos;      // position et cap au début de l'immobilité
    double m_stillAngle = 0;
    qint64 m_stillSinceMs = -1;
    bool m_seen = false;     // marqueur visible à la dernière image
    double m_headingError = 0;
    bool m_inside = false;
    QString m_message;       // résultat du dernier jugement
    bool m_messageGood = false;
    qint64 m_messageMs = -10000;
};
