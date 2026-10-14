#include "trackermodels.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QProgressDialog>
#include <QSaveFile>
#include <QStandardPaths>

namespace TrackerModels {

std::vector<File> files(Model model)
{
    if (model == Nano) {
        return {
            {QStringLiteral("nanotrack_backbone_sim.onnx"),
             QUrl(QStringLiteral("https://raw.githubusercontent.com/HonglinChu/SiamTrackers/master/NanoTrack/"
                                 "models/nanotrackv2/nanotrack_backbone_sim.onnx")),
             "530bdd0cd00f19afab79a863e71ba71e3312395a5dc9151af675082bdaaa2fc4", 1056849},
            {QStringLiteral("nanotrack_head_sim.onnx"),
             QUrl(QStringLiteral("https://raw.githubusercontent.com/HonglinChu/SiamTrackers/master/NanoTrack/"
                                 "models/nanotrackv2/nanotrack_head_sim.onnx")),
             "0d8c0637be849f092cc7236cae02e55c8b9455ebe37ba50601d6115db4247cd9", 726198},
        };
    }
    return {
        {QStringLiteral("object_tracking_vittrack_2023sep.onnx"),
         QUrl(QStringLiteral("https://media.githubusercontent.com/media/opencv/opencv_zoo/main/models/"
                             "object_tracking_vittrack/object_tracking_vittrack_2023sep.onnx")),
         "2990f0b7cd44d92afa48cd97db6de7be113fc1d9594fddb74e2725c10478e91d", 714726},
    };
}

QString downloadDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/models");
}

QString find(const QString &name)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    for (const QString &dir : {downloadDirectory(), appDir + QStringLiteral("/models"),
                               appDir + QStringLiteral("/../models")}) {
        const QString path = QDir(dir).filePath(name);
        if (QFileInfo::exists(path))
            return QDir::cleanPath(path);
    }
    return QString();
}

std::vector<File> missing(Model model)
{
    std::vector<File> out;
    for (const File &f : files(model)) {
        if (find(f.name).isEmpty())
            out.push_back(f);
    }
    return out;
}

bool download(const std::vector<File> &list, QWidget *parent, QString *error)
{
    error->clear();
    if (!QDir().mkpath(downloadDirectory())) {
        *error = QObject::tr("Impossible de créer le dossier %1").arg(downloadDirectory());
        return false;
    }
    qint64 total = 0;
    for (const File &f : list)
        total += f.size;

    QProgressDialog progress(QObject::tr("Téléchargement des modèles..."), QObject::tr("Annuler"), 0,
                             int(total / 1024), parent);
    progress.setWindowTitle(QObject::tr("Modèles du suivi"));
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    QNetworkAccessManager network;
    qint64 done = 0;
    for (const File &f : list) {
        progress.setLabelText(QObject::tr("Téléchargement de %1...").arg(f.name));
        QNetworkRequest request(f.url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply *reply = network.get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::downloadProgress, &loop, [&](qint64 received, qint64) {
            progress.setValue(int((done + received) / 1024));
        });
        QObject::connect(&progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        const QByteArray data = reply->readAll();
        const bool canceled = progress.wasCanceled();
        const QString networkError = reply->errorString();
        const bool failed = reply->error() != QNetworkReply::NoError;
        reply->deleteLater();
        if (canceled)
            return false;
        if (failed) {
            *error = QObject::tr("%1 : %2").arg(f.name, networkError);
            return false;
        }
        // Empreinte vérifiée : on n'exécute pas un fichier tronqué ou modifié.
        if (QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex() != f.sha256) {
            *error = QObject::tr("%1 : fichier reçu inattendu (empreinte différente).").arg(f.name);
            return false;
        }
        QSaveFile out(QDir(downloadDirectory()).filePath(f.name));
        if (!out.open(QIODevice::WriteOnly) || out.write(data) != data.size() || !out.commit()) {
            *error = QObject::tr("Impossible d'enregistrer %1").arg(out.fileName());
            return false;
        }
        done += f.size;
        progress.setValue(int(done / 1024));
    }
    return true;
}

} // namespace TrackerModels
