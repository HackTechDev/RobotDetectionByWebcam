#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>
#include <vector>

class QWidget;

// Modèles des trackers à réseaux de neurones (NanoTrack et VitTrack d'OpenCV) : fichiers ONNX
// à télécharger une fois. Ils sont cherchés, dans l'ordre :
//   ~/.local/share/RobotDetectionByWebcam/models/ (dossier de téléchargement),
//   models/ à côté de l'exécutable, puis models/ dans le dossier parent (lancement depuis build/).
namespace TrackerModels {

struct File
{
    QString name;
    QUrl url;
    QByteArray sha256;  // empreinte attendue, en hexadécimal
    qint64 size = 0;    // en octets
};

enum Model { Nano, Vit };

std::vector<File> files(Model model);
// Dossier où les modèles sont téléchargés.
QString downloadDirectory();
// Chemin du fichier s'il est trouvé, sinon chaîne vide.
QString find(const QString &name);
// Fichiers absents pour ce modèle.
std::vector<File> missing(Model model);

// Télécharge les fichiers (fenêtre de progression, empreintes vérifiées). Retourne false en cas
// d'échec ou d'annulation ; « error » reçoit alors l'explication (vide si annulé).
bool download(const std::vector<File> &files, QWidget *parent, QString *error);

} // namespace TrackerModels
