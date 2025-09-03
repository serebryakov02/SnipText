#include "selectionoverlay.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QKeyEvent>
#include <QColor>
#include <QRegion>

SelectionOverlay::SelectionOverlay(QWidget *parent)
    : QWidget(parent)
    , m_dragging(false)
    , m_color(QColor("red"))
{
    // Frameless overlay that floats above everything and lets each pixel be drawn
    // with its own transparency (so we can dim the screen but leave the selection area clear).
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setCursor(Qt::CrossCursor);
}

void SelectionOverlay::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;

    m_origin    = e->pos();
    m_selection = QRect(m_origin, QSize());
    m_dragging  = true;
    update();
}

void SelectionOverlay::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_dragging)
        return;

    m_selection = QRect(m_origin, e->pos());
    update();
}

void SelectionOverlay::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_dragging || e->button() != Qt::LeftButton)
        return;

    m_dragging = false;
    m_selection = QRect(m_origin, e->pos()).normalized();

    if (m_selection.width() > 0 && m_selection.height() > 0)
        emit selectionFinished(m_selection);
    else
        emit selectionCanceled();
    close();
}

void SelectionOverlay::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        emit selectionCanceled();
        close();
    } else {
        QWidget::keyPressEvent(e);
    }
}

void SelectionOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Show the user a live preview instead of drawing a stored screenshot.
    // We darken the whole screen, but keep the selected rectangle clear.
    // That way, the user still sees the real desktop inside the selection area.

    const QRect sel = m_selection.normalized();

    if (!sel.isNull()) {
        // Dim outside the selection.
        QRegion outside(rect());
        outside = outside.subtracted(QRegion(sel));
        p.save();
        p.setClipRegion(outside);
        p.fillRect(rect(), QColor(0, 0, 0, 120));
        p.restore();

        // Red border around the selection
        p.setPen(QPen(m_color, 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(sel.adjusted(0, 0, -1, -1));
    } else {
        // No selection yet: dim the whole screen.
        p.fillRect(rect(), QColor(0, 0, 0, 120));
    }
}

void SelectionOverlay::setColor(const QColor &newColor)
{
    m_color = newColor;
}
