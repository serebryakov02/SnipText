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

    m_overlay = new SelectionOverlay;
    m_overlay->setColor(m_overlayColor);
    m_overlay->setAttribute(Qt::WA_DeleteOnClose, true);
    m_overlay->setGeometry(m_screen->geometry());
    m_overlay->show();

    connect(m_overlay, &SelectionOverlay::selectionFinished,
            this, [this](const QRect &logicalRect) {
                if (m_overlay)
                    m_overlay->hide();

                if (logicalRect.isNull()) {
                    cleanupOverlay();
                    m_active = false;
                    return;
                }

                QTimer::singleShot(m_captureDelayMs, this, [this, logicalRect]() {
                    performCapture(logicalRect);
                });
            });

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

    const qreal dpr = snap.devicePixelRatio();
    const QRect pixelRect(
        qRound(selectionLogical.x()      * dpr),
        qRound(selectionLogical.y()      * dpr),
        qRound(selectionLogical.width()  * dpr),
        qRound(selectionLogical.height() * dpr)
        );

    const QImage src = snap.toImage();
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
