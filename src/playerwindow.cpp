#include "playerwindow.h"

#include <QCloseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>

PlayerWindow::PlayerWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(tr("RobotDetectionByWebcam — Joueur"));
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMinimumSize(320, 240);
    resize(960, 720);
}

void PlayerWindow::setImage(const QImage &image)
{
    m_image = image;
    update();
}

void PlayerWindow::setFlipped(bool flipped)
{
    m_flipped = flipped;
    update();
}

void PlayerWindow::showOn(QScreen *screen, bool fullScreen)
{
    if (screen) {
        // Fenêtre normale centrée sur l'écran choisi, puis plein écran sur cet écran.
        if (isFullScreen())
            showNormal();
        setScreen(screen);
        const QRect area = screen->availableGeometry();
        const QSize size = this->size().boundedTo(area.size() * 0.9);
        setGeometry(QRect(area.center() - QPoint(size.width() / 2, size.height() / 2), size));
    }
    setFullScreenMode(fullScreen);
    raise();
}

void PlayerWindow::setFullScreenMode(bool fullScreen)
{
    if (fullScreen) {
        showFullScreen();
        setCursor(Qt::BlankCursor);
    } else {
        showNormal();
        unsetCursor();
    }
}

void PlayerWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    if (m_image.isNull())
        return;
    // Image entière, proportions conservées, centrée.
    const QSize size = m_image.size().scaled(this->size(), Qt::KeepAspectRatio);
    const QRect target(QPoint((width() - size.width()) / 2, (height() - size.height()) / 2), size);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (m_flipped) {
        painter.translate(target.left() + target.right() + 1, 0);  // symétrie autour du centre de l'image
        painter.scale(-1, 1);
    }
    painter.drawImage(target, m_image);
}

void PlayerWindow::mouseDoubleClickEvent(QMouseEvent *)
{
    setFullScreenMode(!isFullScreen());
    emit fullScreenChanged(isFullScreen());
}

void PlayerWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F11 || (event->key() == Qt::Key_Escape && isFullScreen())) {
        setFullScreenMode(event->key() == Qt::Key_F11 && !isFullScreen());
        emit fullScreenChanged(isFullScreen());
    } else {
        QWidget::keyPressEvent(event);
    }
}

void PlayerWindow::closeEvent(QCloseEvent *event)
{
    emit closed();
    QWidget::closeEvent(event);
}
