#include "videowidget.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

namespace {

constexpr int kHandleSize = 8;  // côté des poignées, en pixels de l'écran

// Poignées : 0 haut-gauche, 1 haut, 2 haut-droite, 3 droite, 4 bas-droite, 5 bas, 6 bas-gauche, 7 gauche.
QPointF handlePoint(const QRectF &r, int i)
{
    const double xs[] = {r.left(), r.center().x(), r.right(), r.right(), r.right(), r.center().x(), r.left(), r.left()};
    const double ys[] = {r.top(), r.top(), r.top(), r.center().y(), r.bottom(), r.bottom(), r.bottom(), r.center().y()};
    return {xs[i], ys[i]};
}

} // namespace

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
}

void VideoWidget::setImage(const QImage &image)
{
    m_image = image;
    update();
}

void VideoWidget::setSelectionMode(bool enabled)
{
    m_selectionMode = enabled;
    m_drag = Drag::None;
    m_pending = QRect();
    setCursor(Qt::CrossCursor);
    emit pendingRectChanged();
    update();
}

void VideoWidget::setPendingRect(const QRect &imageRect)
{
    m_pending = imageRect.normalized() & m_image.rect();
    emit pendingRectChanged();
    update();
}

void VideoWidget::confirmSelection()
{
    if (m_pending.isNull())
        return;
    const QRect rect = m_pending;
    m_selectionMode = false;
    m_pending = QRect();
    m_drag = Drag::None;
    setCursor(Qt::CrossCursor);
    emit pendingRectChanged();
    update();
    emit rectangleSelected(rect);
}

QRect VideoWidget::targetRect() const
{
    if (m_image.isNull())
        return {};
    const QSize size = m_image.size().scaled(this->size(), Qt::KeepAspectRatio);
    return QRect(QPoint((width() - size.width()) / 2, (height() - size.height()) / 2), size);
}

QPoint VideoWidget::toImage(const QPointF &widgetPos) const
{
    const QRect target = targetRect();
    const QPointF rel = widgetPos - target.topLeft();
    return QPoint(std::clamp(int(rel.x() * m_image.width() / target.width()), 0, m_image.width() - 1),
                  std::clamp(int(rel.y() * m_image.height() / target.height()), 0, m_image.height() - 1));
}

QPointF VideoWidget::toWidget(const QPointF &imagePos) const
{
    const QRect target = targetRect();
    return target.topLeft() + QPointF(imagePos.x() * target.width() / m_image.width(),
                                      imagePos.y() * target.height() / m_image.height());
}

QRectF VideoWidget::pendingWidgetRect() const
{
    return QRectF(toWidget(m_pending.topLeft()), toWidget(m_pending.topLeft() + QPoint(m_pending.width(), m_pending.height())));
}

int VideoWidget::handleAt(const QPointF &widgetPos) const
{
    if (m_pending.isNull())
        return -1;
    const QRectF r = pendingWidgetRect();
    for (int i = 0; i < 8; ++i) {
        const QPointF d = widgetPos - handlePoint(r, i);
        if (std::abs(d.x()) <= kHandleSize && std::abs(d.y()) <= kHandleSize)
            return i;
    }
    return -1;
}

void VideoWidget::updateCursor(const QPointF &widgetPos)
{
    if (!m_selectionMode || m_pending.isNull()) {
        setCursor(Qt::CrossCursor);
        return;
    }
    static const Qt::CursorShape shapes[] = {Qt::SizeFDiagCursor, Qt::SizeVerCursor, Qt::SizeBDiagCursor,
                                             Qt::SizeHorCursor,  Qt::SizeFDiagCursor, Qt::SizeVerCursor,
                                             Qt::SizeBDiagCursor, Qt::SizeHorCursor};
    const int h = handleAt(widgetPos);
    if (h >= 0)
        setCursor(shapes[h]);
    else if (pendingWidgetRect().contains(widgetPos))
        setCursor(Qt::SizeAllCursor);
    else
        setCursor(Qt::CrossCursor);
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

    if (m_selectionMode && !m_pending.isNull()) {
        const QRectF r = pendingWidgetRect();
        painter.setPen(QPen(Qt::cyan, 2, Qt::DashLine));
        painter.setBrush(QColor(0, 255, 255, 40));
        painter.drawRect(r);
        if (m_drag != Drag::Draw) {
            painter.setPen(QPen(Qt::black, 1));
            painter.setBrush(Qt::cyan);
            for (int i = 0; i < 8; ++i) {
                const QPointF c = handlePoint(r, i);
                painter.drawRect(QRectF(c.x() - kHandleSize / 2.0, c.y() - kHandleSize / 2.0, kHandleSize, kHandleSize));
            }
        }
    }
}

void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    const QRect target = targetRect();
    const QPointF pos = event->position();
    if (!target.contains(pos.toPoint()) && handleAt(pos) < 0)
        return;
    if (!m_selectionMode) {
        emit imageClicked(toImage(pos));
        return;
    }
    // Sélection : poignée -> redimensionner, intérieur du cadre -> déplacer, ailleurs -> tracer.
    m_dragStart = toImage(pos);
    m_dragOrigin = m_pending;
    m_handle = handleAt(pos);
    if (m_handle >= 0) {
        m_drag = Drag::Resize;
    } else if (!m_pending.isNull() && pendingWidgetRect().contains(pos)) {
        m_drag = Drag::Move;
    } else {
        m_drag = Drag::Draw;
        m_drawStartWidget = pos.toPoint();
        m_pending = QRect(m_dragStart, QSize(1, 1));
    }
    update();
}

void VideoWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint p = toImage(event->position());
    switch (m_drag) {
    case Drag::None:
        updateCursor(event->position());
        return;
    case Drag::Draw:
        m_pending = QRect(m_dragStart, p).normalized();
        break;
    case Drag::Move: {
        QRect r = m_dragOrigin.translated(p - m_dragStart);
        r.moveLeft(std::clamp(r.left(), 0, m_image.width() - r.width()));
        r.moveTop(std::clamp(r.top(), 0, m_image.height() - r.height()));
        m_pending = r;
        break;
    }
    case Drag::Resize: {
        QRect r = m_dragOrigin;
        const QPoint d = p - m_dragStart;
        if (m_handle == 0 || m_handle == 6 || m_handle == 7)
            r.setLeft(std::min(r.left() + d.x(), r.right() - 8));
        if (m_handle == 2 || m_handle == 3 || m_handle == 4)
            r.setRight(std::max(r.right() + d.x(), r.left() + 8));
        if (m_handle == 0 || m_handle == 1 || m_handle == 2)
            r.setTop(std::min(r.top() + d.y(), r.bottom() - 8));
        if (m_handle == 4 || m_handle == 5 || m_handle == 6)
            r.setBottom(std::max(r.bottom() + d.y(), r.top() + 8));
        m_pending = r & m_image.rect();
        break;
    }
    }
    update();
}

void VideoWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_drag == Drag::None)
        return;
    const Drag drag = m_drag;
    m_drag = Drag::None;
    // Clic sans glisser : délimitation automatique de l'objet sous le curseur.
    if (drag == Drag::Draw && (event->position().toPoint() - m_drawStartWidget).manhattanLength() < 5) {
        m_pending = m_dragOrigin;
        update();
        emit selectionPointClicked(m_dragStart);
        return;
    }
    updateCursor(event->position());
    emit pendingRectChanged();
    update();
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Double-clic dans le cadre en attente : validation.
    if (m_selectionMode && !m_pending.isNull() && pendingWidgetRect().contains(event->position())) {
        confirmSelection();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}
