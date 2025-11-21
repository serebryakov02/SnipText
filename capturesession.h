#ifndef CAPTURESESSION_H
#define CAPTURESESSION_H

#include <QObject>
#include <QColor>
#include <QImage>
#include <QPointer>
#include <QRect>

class QScreen;
class SelectionOverlay;

// Handles the lifetime of a selection overlay and emits the cropped frame once
// the compositor has removed the overlay from the captured screen.
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

    // Screen chosen for the current capture session.
    QScreen *m_screen = nullptr;

    // Color used for the live overlay border/fill.
    QColor m_overlayColor = QColor("red");

    // Time to wait before grabbing so the overlay is no longer visible.
    int m_captureDelayMs = 0;

    // The top-level overlay widget, owned/lifetime-managed by the session.
    QPointer<SelectionOverlay> m_overlay;

    // Guards against running multiple captures at once.
    bool m_active = false;
};

#endif // CAPTURESESSION_H
