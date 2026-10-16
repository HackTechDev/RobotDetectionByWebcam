#pragma once

#include "markerdetector.h"
#include "objecttracker.h"
#include "zonemanager.h"

#include <QColor>
#include <QDialog>
#include <QFont>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <functional>
#include <vector>

class QComboBox;
class QLabel;
class QListWidget;
class QPainter;
class QPushButton;
class QSpinBox;

// Cadre commun des jeux du menu Jeu : chaque jeu déclare ses réglages (robot, zones, choix...),
// reçoit à chaque image l'état des objets suivis, des marqueurs et des zones, et fournit son bandeau,
// son dessin sur l'image, ses événements et son état pour l'envoi UDP. La fenêtre du jeu est
// construite automatiquement à partir des réglages déclarés.

// Objet suivi, vu par un jeu.
struct GameObject
{
    int id = 0;
    QString name;
    const TrackedObject *state = nullptr;
};

// Ce que voit un jeu à chaque image.
struct GameContext
{
    std::vector<GameObject> tracked;
    const std::vector<DetectedMarker> *markers = nullptr;
    const std::vector<Zone> *zones = nullptr;
    QSize imageSize;
    qint64 timeMs = 0;
    double motionPx = 15.0;  // seuils de mouvement du suivi (onglet Suivi)
    double motionMm = 20.0;

    const GameObject *object(int id) const;
    const DetectedMarker *marker(int id) const;
    const Zone *zone(int id) const;
    QRectF zoneRect(int id) const;  // rectangle vide si la zone n'existe pas
};

// Pour dessiner sur l'image affichée (miroir éventuel déjà pris en compte par toDisplay).
struct GameDrawContext
{
    QPainter *painter = nullptr;
    std::function<QPointF(const QPointF &)> toDisplay;
    std::function<QRectF(const QRectF &)> rectToDisplay;
    int penWidth = 2;
    QFont font;
};

// Réglage déclaré par un jeu, affiché dans sa fenêtre.
struct GameParam
{
    enum Kind {
        Tracked,       // objet suivi (liste)
        Marker,        // numéro d'un marqueur ArUco
        Zone,          // zone obligatoire
        ZoneOptional,  // zone facultative (« noneLabel » = aucune, valeur 0)
        ZoneList,      // plusieurs zones, cochées dans la liste (ordre de l'onglet Zones)
        Choice         // liste de choix (valeur = index)
    };
    QString key;
    QString label;
    Kind kind = Choice;
    QStringList choices;  // Choice
    int defaultValue = 0; // Choice : index ; Tracked : rang dans la liste ; Marker : numéro
    QString noneLabel;    // ZoneOptional
    QString tooltip;
};

// Valeurs choisies dans la fenêtre du jeu.
struct GameSettings
{
    QMap<QString, int> values;
    QMap<QString, QList<int>> lists;
    int value(const QString &key, int fallback = 0) const { return values.value(key, fallback); }
    QList<int> list(const QString &key) const { return lists.value(key); }
};

class Game
{
public:
    virtual ~Game() = default;

    virtual QString title() const = 0;   // nom dans le menu Jeu
    virtual QString key() const = 0;     // « mode » de l'objet game en UDP
    virtual QString rules() const = 0;   // règle affichée dans la fenêtre du jeu
    virtual std::vector<GameParam> params() const = 0;
    // Message si la partie ne peut pas démarrer avec ces réglages (vide : prête).
    virtual QString checkReady(const GameSettings &settings, const GameContext &context) const;

    void start(const GameSettings &settings, const GameContext &context);
    void stop() { m_active = false; m_running = false; }
    bool isActive() const { return m_active; }    // en cours, ou terminée avec le résultat affiché
    bool isRunning() const { return m_running; }  // en cours

    // À chaque image : met la partie à jour si elle est en cours (sinon ne fait rien).
    std::vector<MotionEvent> update(const GameContext &context)
    {
        return m_running ? onUpdate(context) : std::vector<MotionEvent>();
    }
    virtual void draw(const GameDrawContext &, const GameContext &) const {}
    virtual QString bannerText(const GameContext &context) const = 0;
    virtual QColor bannerColor(const GameContext &context) const = 0;
    // Texte en grand dans la fenêtre du jeu (par défaut : celui du bandeau) et ligne de compteurs.
    virtual QString stateText(const GameContext &context) const { return bannerText(context); }
    virtual QString countersText(const GameContext &) const { return QString(); }
    virtual QJsonObject toJson(const GameContext &context) const;

    // Bouton facultatif de la fenêtre du jeu pendant la partie (« Terminer le dessin »...).
    virtual QString actionLabel() const { return QString(); }
    virtual void action(const GameContext &) {}

protected:
    virtual void onStart(const GameContext &context) = 0;
    // Appelée à chaque image tant que la partie est en cours.
    virtual std::vector<MotionEvent> onUpdate(const GameContext &context) = 0;
    void finish() { m_running = false; }  // partie terminée, résultat encore affiché
    // Événement du jeu, pour le journal et l'UDP.
    MotionEvent event(const GameContext &context, const QString &code, const QString &text, int objectId = 0,
                      int count = 0) const;

    GameSettings m_settings;

private:
    bool m_active = false;
    bool m_running = false;
};

// Fenêtre d'un jeu du cadre commun : réglages, état en grand, compteurs, boutons.
class GameDialog : public QDialog
{
    Q_OBJECT

public:
    GameDialog(Game *game, QWidget *parent = nullptr);

    // Objets suivis et zones disponibles : (numéro, nom). La sélection est conservée si possible.
    void setChoices(const std::vector<std::pair<int, QString>> &objects,
                    const std::vector<std::pair<int, QString>> &zones);
    GameSettings settings() const;
    void setRunning(bool running);
    void showStopped();
    void showState(const GameContext &context);

signals:
    void startRequested();
    void stopRequested();
    void actionRequested();

private:
    Game *m_game;
    struct ParamWidget
    {
        GameParam param;
        QComboBox *combo = nullptr;
        QSpinBox *spin = nullptr;
        QListWidget *list = nullptr;
    };
    std::vector<ParamWidget> m_widgets;
    QLabel *m_stateLabel = nullptr;
    QLabel *m_countersLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_actionButton = nullptr;
    bool m_running = false;
};

// Outils communs aux jeux.
namespace GameUtil {
// Couleurs habituelles des bandeaux.
QColor grey();
QColor green();
QColor red();
QColor orange();
QColor blue();
QColor gold();
QColor purple();
// Temps en secondes, avec une décimale (« 12,3 »).
QString seconds(double s);
// Minutes et secondes (« 1:05 »).
QString clock(double s);
double distance(const QPointF &a, const QPointF &b);
// Cadre entièrement contenu dans l'union des rectangles (ses 4 coins et son centre y sont).
bool insideUnion(const QRectF &box, const std::vector<QRectF> &rects);
// Écrit le nom d'une zone ou un texte au-dessus d'un rectangle.
void label(const GameDrawContext &draw, const QPointF &displayPos, const QString &text, const QColor &color);
} // namespace GameUtil
