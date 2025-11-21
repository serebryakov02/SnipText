#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QMainWindow>
#include <QString>

class QPushButton;
class QImage;

class CaptureSession;
class OcrService;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNewScreenshot();

private:
    void processCapturedImage(const QImage &image);
    void handleCaptureError(const QString &errorMessage, bool fatal);
    CaptureSession* createCaptureSession();

private:
    QPushButton *m_newShotBtn;

    // Defer time to let compositor remove the overlay from the frame.
    const int m_captureDelayMs = 180;

    OcrService *m_ocrService;  // single API instance

    // Selection overlay color.
    QColor m_color;

    // When true, the screenshot is saved in addition to the extracted text.
    bool m_saveScreenshot;

    // The directory where screenshots will be saved if the user has toggled that action on.
    QString m_dir;

    void initGUI();
};

#endif // MAINWINDOW_H
