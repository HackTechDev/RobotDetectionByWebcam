#pragma once

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>
#include <vector>

// Accès direct aux contrôles du pilote Linux des webcams (V4L2) : luminosité, contraste, gain,
// fréquence du secteur... Qt Multimedia n'expose que l'exposition et la balance des blancs.
// Hors Linux, aucune fonction n'a d'effet.
namespace V4l2 {

struct Control
{
    enum Type { Integer, Boolean, Menu };
    quint32 id = 0;
    QString name;       // nom donné par le pilote (en anglais)
    Type type = Integer;
    int minimum = 0;
    int maximum = 0;
    int step = 1;
    int defaultValue = 0;
    int value = 0;      // valeur actuelle dans la webcam
    bool inactive = false;  // sans effet dans l'état actuel (dépend d'un autre réglage)
    QList<QPair<int, QString>> menu;  // menus : valeur -> libellé du pilote
};

// Contrôles réglables de la webcam (entiers, cases à cocher et menus ; ni désactivés ni en
// lecture seule), dans l'ordre du pilote.
std::vector<Control> listControls(const QByteArray &device);

bool setControl(const QByteArray &device, quint32 id, int value);

// Limites réelles du temps de pose manuel, en millisecondes. Retourne false si la webcam ne
// les donne pas.
bool exposureRange(const QByteArray &device, double *minMs, double *maxMs);

// Remet l'exposition automatique (essaie le mode automatique complet, puis la priorité ouverture,
// seul mode accepté par certaines webcams comme la Logitech C270).
bool restoreAutoExposure(const QByteArray &device);

// Identifiants des contrôles déjà gérés par l'exposition et la balance des blancs.
bool isExposureOrWhiteBalance(quint32 id);

// Nom, explication et libellés de menu en français pour les contrôles courants ; à défaut, le
// nom donné par le pilote.
QString frenchName(const Control &control);
QString frenchDescription(const Control &control);
QString frenchMenuItem(const Control &control, int value, const QString &driverLabel);

} // namespace V4l2
