#pragma once

#include <QImage>
#include <QWidget>

// Affiche une image en conservant ses proportions et signale les clics en coordonnées de
// l'image. En mode sélection, un glisser trace un rectangle au lieu d'un clic.
class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);

    void setImage(const QImage &image);
    void setSelectionMode(bool enabled);
    bool selectionMode() const { return m_selectionMode; }

signals:
    void imageClicked(const QPoint &imagePos);
    void rectangleSelected(const QRect &imageRect);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRect targetRect() const;
    QPoint toImage(const QPointF &widgetPos) const;

    QImage m_image;
    bool m_selectionMode = false;
    bool m_dragging = false;
    QPoint m_dragStart;  // en coordonnées du widget
    QPoint m_dragEnd;
};
