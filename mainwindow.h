#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QMainWindow>
#include <QString>
#include <QStringList>

class QPushButton;
class QImage;
class QSettings;
class QShortcut;

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
    void processCapturedImage(const QImage &image, bool multiCapture);
    void handleCaptureError(const QString &errorMessage, bool fatal);
    CaptureSession* createCaptureSession();
    void finalizeMultiCapture();
    void saveScreenshot(const QImage &image);

private:
    QPushButton *m_newShotBtn;

    // Defer time to let compositor remove the overlay from the frame.
    const int m_captureDelayMs = 180;

    OcrService *m_ocrService;  // single API instance

    // Selection overlay color.
    QColor m_color;

    // When true, the screenshot is saved in addition to the extracted text.
    bool m_saveScreenshot;

    // When true, allow capturing multiple regions before finishing.
    bool m_captureMultipleAreas;

    // The directory where screenshots will be saved if the user has toggled that action on.
    QString m_dir;

    QStringList m_multiCaptureTexts;

    void initGUI();

    QSettings *m_settings;
    QString m_captureShortcut;
    QShortcut *m_shortcutHandler;
};

#endif // MAINWINDOW_H
