#include "capturesession.h"

#include "selectionoverlay.h"

#include <QCursor>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QtMath>

CaptureSession::CaptureSession(QObject *parent)
    : QObject(parent)
{
}

void CaptureSession::setOverlayColor(const QColor &color)
{
    m_overlayColor = color;
}

void CaptureSession::setCaptureDelay(int delayMs)
{
    m_captureDelayMs = delayMs;
}

void CaptureSession::start()
{
    if (m_active)
        return;

    m_active = true;

    // Choose and store the screen under the cursor; fall back to primary.
    m_screen = QGuiApplication::screenAt(QCursor::pos());
    if (!m_screen)
        m_screen = QGuiApplication::primaryScreen();

    if (!m_screen) {
        emit captureFailed(tr("No screen available."), true);
        m_active = false;
        return;
    }

    beginOverlay();
}

void CaptureSession::beginOverlay()
{
    cleanupOverlay();

    // Create a transient overlay (top-level window).
    m_overlay = new SelectionOverlay;
    m_overlay->setColor(m_overlayColor);
    m_overlay->setAttribute(Qt::WA_DeleteOnClose, true);
    m_overlay->setGeometry(m_screen->geometry());
    m_overlay->show();

    // When selection finishes, hide overlay, defer grab, then save.
    connect(m_overlay, &SelectionOverlay::selectionFinished,
            this, [this](const QRect &logicalRect) {
                if (logicalRect.isNull()) {
                    cleanupOverlay();
                    m_active = false;
                    return;
                }

                // Ensure overlay is not captured.
                if (m_overlay)
                    m_overlay->hide();

                // Give the compositor time to remove it from the frame.
                QTimer::singleShot(m_captureDelayMs, this, [this, logicalRect]() {
                    performCapture(logicalRect);
                });
            });

    // If user cancels, just close/hide the overlay.
    connect(m_overlay, &SelectionOverlay::selectionCanceled,
            this, [this]() {
                cleanupOverlay();
                m_active = false;
            });

    connect(m_overlay, &QObject::destroyed,
            this, [this]() {
                m_overlay = nullptr;
            });
}

void CaptureSession::performCapture(const QRect &selectionLogical)
{
    cleanupOverlay();

    // Use the session's screen (selected once in start(), same idea as the old
    // onNewScreenshot() path) so we never capture from the wrong monitor even if
    // the cursor moves while the overlay is visible.
    if (!m_screen) {
        emit captureFailed(tr("No screen available."), true);
        m_active = false;
        return;
    }

    const QPixmap snap = m_screen->grabWindow(0);
    if (snap.isNull()) {
        emit captureFailed(tr("Failed to capture the screen."), true);
        m_active = false;
        return;
    }

    // The selection rectangle comes from the overlay in "logical" coordinates (DPI-independent).
    // The screenshot pixmap uses real screen pixels. On Retina/HiDPI displays, 1 logical unit
    // can equal 2 or more physical pixels. Multiply by devicePixelRatio (dpr) to match them.
    const qreal dpr = snap.devicePixelRatio();
    const QRect pixelRect(
        qRound(selectionLogical.x()      * dpr),
        qRound(selectionLogical.y()      * dpr),
        qRound(selectionLogical.width()  * dpr),
        qRound(selectionLogical.height() * dpr)
        );

    const QImage src = snap.toImage(); // Device pixels.
    if (!src.rect().contains(pixelRect)) {
        emit captureFailed(tr("Selection is out of bounds."), false);
        m_active = false;
        return;
    }

    emit captureReady(src.copy(pixelRect));
    m_active = false;
}

void CaptureSession::cleanupOverlay()
{
    if (m_overlay) {
        m_overlay->close();
        m_overlay = nullptr;
    }
}
