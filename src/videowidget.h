#pragma once

#include <QImage>
#include <QWidget>

// Affiche une image en conservant ses proportions et signale les clics
// en coordonnées de l'image.
class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);

    void setImage(const QImage &image);

signals:
    void imageClicked(const QPoint &imagePos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QRect targetRect() const;

    QImage m_image;
};
