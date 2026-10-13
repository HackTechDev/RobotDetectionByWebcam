#pragma once

#include <QImage>
#include <QWidget>

// Affiche une image en conservant ses proportions et signale les clics en coordonnées de
// l'image. En mode sélection, on trace un rectangle (ou on clique pour une délimitation
// automatique) ; le rectangle reste ajustable (poignées, déplacement) jusqu'à sa validation.
class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);

    void setImage(const QImage &image);
    void setSelectionMode(bool enabled);
    bool selectionMode() const { return m_selectionMode; }

    // Rectangle en attente de validation, en coordonnées de l'image affichée.
    void setPendingRect(const QRect &imageRect);
    bool hasPendingRect() const { return !m_pending.isNull(); }
    // Valide le rectangle en attente : émet rectangleSelected() et quitte le mode sélection.
    void confirmSelection();

signals:
    void imageClicked(const QPoint &imagePos);
    // Clic sans glisser en mode sélection : demande de délimitation automatique.
    void selectionPointClicked(const QPoint &imagePos);
    void pendingRectChanged();
    void rectangleSelected(const QRect &imageRect);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    enum class Drag { None, Draw, Move, Resize };

    QRect targetRect() const;
    QPoint toImage(const QPointF &widgetPos) const;
    QPointF toWidget(const QPointF &imagePos) const;
    QRectF pendingWidgetRect() const;
    int handleAt(const QPointF &widgetPos) const;  // poignée sous la souris (0 à 7) ou -1
    void updateCursor(const QPointF &widgetPos);

    QImage m_image;
    bool m_selectionMode = false;
    QRect m_pending;  // rectangle en attente, coordonnées de l'image affichée
    Drag m_drag = Drag::None;
    int m_handle = -1;
    QPoint m_dragStart;  // coordonnées de l'image
    QRect m_dragOrigin;
    QPoint m_drawStartWidget;
};
