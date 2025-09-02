#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRect>

class QPushButton;
class QScreen;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNewScreenshot();

private:
    void saveSelectionFromScreen(const QRect &selectionLogical);

private:
    QPushButton *m_newShotBtn;

    // Screen chosen for the current capture session.
    QScreen *m_screen;

    // Defer time to let compositor remove the overlay from the frame.
    const int m_captureDelayMs = 180;
};

#endif // MAINWINDOW_H
