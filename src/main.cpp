#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("RobotDetectionByWebcam"));
    MainWindow window;
    window.show();
    return app.exec();
}
