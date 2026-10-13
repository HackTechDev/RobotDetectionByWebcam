#include "videowidget.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

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

void VideoWidget::setSelectionMode(bool enabled)
{
    m_selectionMode = enabled;
    m_dragging = false;
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

    if (m_dragging) {
        painter.setPen(QPen(Qt::cyan, 2, Qt::DashLine));
        painter.setBrush(QColor(0, 255, 255, 40));
        painter.drawRect(QRect(m_dragStart, m_dragEnd).normalized());
    }
}

QPoint VideoWidget::toImage(const QPointF &widgetPos) const
{
    const QRect target = targetRect();
    const QPointF rel = widgetPos - target.topLeft();
    return QPoint(int(rel.x() * m_image.width() / target.width()),
                  int(rel.y() * m_image.height() / target.height()));
}

void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    const QRect target = targetRect();
    if (!target.contains(event->position().toPoint()))
        return;
    if (m_selectionMode) {
        m_dragging = true;
        m_dragStart = m_dragEnd = event->position().toPoint();
        return;
    }
    emit imageClicked(toImage(event->position()));
}

void VideoWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging)
        return;
    // Le rectangle reste dans l'image affichée.
    const QRect target = targetRect();
    const QPoint p = event->position().toPoint();
    m_dragEnd = QPoint(std::clamp(p.x(), target.left(), target.right()),
                       std::clamp(p.y(), target.top(), target.bottom()));
    update();
}

void VideoWidget::mouseReleaseEvent(QMouseEvent *)
{
    if (!m_dragging)
        return;
    m_dragging = false;
    update();
    const QRect rect = QRect(toImage(m_dragStart), toImage(m_dragEnd)).normalized();
    emit rectangleSelected(rect);
}
