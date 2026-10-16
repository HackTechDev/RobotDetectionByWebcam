#pragma once

#include "gameframework.h"

// Jeu « Le fil électrique » : suivre un couloir (zones cochées, qui se touchent) sans en sortir.
// La première zone est le départ, la dernière l'arrivée. Le chrono démarre quand le robot quitte le
// départ et s'arrête quand il est entièrement dans l'arrivée. Chaque sortie du couloir (cadre du
// robot qui dépasse) est une touche : +3 s. Variante « main tremblante » : vitesse maximale, chaque
// excès : +2 s.
class WireGame : public Game
{
public:
    QString title() const override { return QObject::tr("Le fil électrique"); }
    QString key() const override { return QStringLiteral("wire"); }
    QString rules() const override;
    std::vector<GameParam> params() const override;
    QString checkReady(const GameSettings &settings, const GameContext &context) const override;
    void draw(const GameDrawContext &draw, const GameContext &context) const override;
    QString bannerText(const GameContext &context) const override;
    QColor bannerColor(const GameContext &context) const override;
    QString countersText(const GameContext &context) const override;
    QJsonObject toJson(const GameContext &context) const override;

    static constexpr double kTouchPenalty = 3.0;
    static constexpr double kSpeedPenalty = 2.0;

protected:
    void onStart(const GameContext &context) override;
    std::vector<MotionEvent> onUpdate(const GameContext &context) override;

private:
    enum Phase { Waiting, Ready, Running, Finished };
    double speedLimit(const GameContext &context) const;  // en px/s (0 : pas de limite)
    double elapsed(qint64 timeMs) const;
    double score(qint64 timeMs) const { return elapsed(timeMs) + m_touches * kTouchPenalty + m_overspeeds * kSpeedPenalty; }

    QList<int> m_zones;
    Phase m_phase = Waiting;
    qint64 m_startMs = 0;
    qint64 m_finishMs = 0;
    int m_touches = 0;
    int m_overspeeds = 0;
    bool m_outside = false;        // robot hors du couloir (touche en cours)
    qint64 m_outsideSinceMs = -1;  // candidat à une touche
    bool m_tooFast = false;
    qint64 m_lastPenaltyMs = -10000;
    QString m_lastPenalty;
    qint64 m_finishSinceMs = -1;
    bool m_seen = false;
    double m_speed = 0;
};
