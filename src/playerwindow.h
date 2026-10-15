#pragma once

#include <QImage>
#include <QWidget>

class QScreen;

// Fenêtre du joueur : uniquement la vidéo, sur fond noir, pour un second écran. Double-clic ou F11 :
// plein écran ; Échap : quitte le plein écran. Le curseur est masqué en plein écran.
class PlayerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerWindow(QWidget *parent = nullptr);

    void setImage(const QImage &image);
    // Retourne l'image horizontalement (joueur face à la caméra).
    void setFlipped(bool flipped);
    // Affiche la fenêtre sur cet écran, en plein écran ou non.
    void showOn(QScreen *screen, bool fullScreen);
    void setFullScreenMode(bool fullScreen);

signals:
    void closed();
    // Plein écran activé ou quitté par le joueur (double-clic, F11, Échap).
    void fullScreenChanged(bool fullScreen);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    QImage m_image;
    bool m_flipped = false;
};
