#include "mainwindow.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("RobotDetectionByWebcam"));

    // Masque les messages sans gravité de Qt Multimedia (version de FFmpeg) et du décodeur
    // JPEG (« Corrupt JPEG data », fréquent avec les webcams en MJPEG). L'option est écrite
    // dans le fichier de réglages au premier lancement pour pouvoir la désactiver.
    QSettings settings(MainWindow::settingsPath(), QSettings::IniFormat);
    const QString key = QStringLiteral("logging/hideLibraryMessages");
    if (!settings.contains(key))
        settings.setValue(key, true);
    if (settings.value(key).toBool()) {
        QLoggingCategory::setFilterRules(QStringLiteral("qt.multimedia.ffmpeg.info=false\n"
                                                        "qt.gui.imageio.jpeg.warning=false"));
    }

    MainWindow window;
    window.show();
    return app.exec();
}
