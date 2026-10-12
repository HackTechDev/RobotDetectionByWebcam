#include "videowidget.h"

#include <QMouseEvent>
#include <QPainter>

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::CrossCursor);
}

void VideoWidget::setImage(const QImage &image)
{
    m_image = image;
    update();
}

QRect VideoWidget::targetRect() const
{
    if (m_image.isNull())
        return {};
    const QSize size = m_image.size().scaled(this->size(), Qt::KeepAspectRatio);
    return QRect(QPoint((width() - size.width()) / 2, (height() - size.height()) / 2), size);
}

void VideoWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    if (m_image.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, tr("En attente de la webcam..."));
        return;
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(targetRect(), m_image);
}

void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    const QRect target = targetRect();
    if (!target.contains(event->position().toPoint()))
        return;
    const QPointF rel = event->position() - target.topLeft();
    emit imageClicked(QPoint(int(rel.x() * m_image.width() / target.width()),
                             int(rel.y() * m_image.height() / target.height())));
}
