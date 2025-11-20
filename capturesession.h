#ifndef CAPTURESESSION_H
#define CAPTURESESSION_H

#include <QObject>
#include <QColor>
#include <QImage>
#include <QPointer>
#include <QRect>
#include <QString>

class QScreen;
class SelectionOverlay;

class CaptureSession : public QObject
{
    Q_OBJECT
public:
    explicit CaptureSession(QObject *parent = nullptr);

    void setOverlayColor(const QColor &color);
    void setCaptureDelay(int delayMs);

    void start();

signals:
    void captureReady(const QImage &image);
    void captureFailed(const QString &errorMessage, bool fatal);

private:
    void beginOverlay();
    void performCapture(const QRect &logicalRect);
    void cleanupOverlay();

    QScreen *m_screen = nullptr;
    QColor m_overlayColor = QColor("red");
    int m_captureDelayMs = 0;
    QPointer<SelectionOverlay> m_overlay;
    bool m_active = false;
};

#endif // CAPTURESESSION_H
