#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRect>
#include <QColor>

class QPushButton;
class QScreen;

namespace tesseract { class TessBaseAPI; } // Forward-declare.

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

    tesseract::TessBaseAPI* m_ocr = nullptr;  // single API instance

    // Selection overlay color.
    QColor m_color;

    // When true, the screenshot is saved in addition to the extracted text.
    bool m_saveScreenshot;

    // The directory where screenshots will be saved if the user has toggled that action on.
    QString m_dir;

    void initGUI();
};

#endif // MAINWINDOW_H
