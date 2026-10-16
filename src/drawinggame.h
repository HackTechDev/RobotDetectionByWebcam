#pragma once

#include "gameframework.h"

#include <QMap>
#include <QPolygonF>

// Jeu « Dessin au sol » : reproduire une forme (carré, cercle, triangle, zigzag) avec la trajectoire
// du robot. Le modèle est dessiné dans le cadre choisi (une zone) ; le tracé commence au premier
// mouvement du robot et se termine après 2 s d'immobilité (ou avec le bouton « Terminer le dessin »).
// La ressemblance (0 à 100 %) vient de l'écart moyen entre le tracé et le modèle, dans les deux sens
// (le tracé suit-il le modèle, et couvre-t-il toute la forme ?). Le meilleur dessin de chaque forme est
// conservé.
class DrawingGame : public Game
{
public:
    QString title() const override { return QObject::tr("Dessin au sol"); }
    QString key() const override { return QStringLiteral("drawing"); }
    QString rules() const override;
    std::vector<GameParam> params() const override;
    void draw(const GameDrawContext &draw, const GameContext &context) const override;
    QString bannerText(const GameContext &context) const override;
    QColor bannerColor(const GameContext &context) const override;
    QString countersText(const GameContext &context) const override;
    QJsonObject toJson(const GameContext &context) const override;
    QString actionLabel() const override { return QObject::tr("Terminer le dessin"); }
    void action(const GameContext &context) override;

    // Ressemblance (0 à 100) d'un tracé avec un modèle, rapportée à la diagonale du cadre.
    static double similarity(const QPolygonF &trace, const QPolygonF &model, double diagonal);
    // Polyligne régulièrement échantillonnée (n points le long du tracé).
    static QPolygonF resample(const QPolygonF &line, int n);

protected:
    void onStart(const GameContext &context) override;
    std::vector<MotionEvent> onUpdate(const GameContext &context) override;

private:
    enum Phase { Waiting, Drawing, Done };
    QPolygonF model(const GameContext &context) const;  // forme dans le cadre, en pixels de l'image
    std::vector<MotionEvent> evaluate(const GameContext &context);

    int m_shape = 0;
    Phase m_phase = Waiting;
    QPolygonF m_trace;
    bool m_hasAnchor = false;
    QPointF m_anchor;
    QPointF m_stillPos;
    qint64 m_stillSinceMs = -1;
    double m_score = 0;
    QMap<int, double> m_best;       // meilleure ressemblance par forme
    QMap<int, QPolygonF> m_bestTrace;
    bool m_newBest = false;
    bool m_finishRequested = false;
};
