#include "selectionoverlay.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QKeyEvent>
#include <QColor>
#include <QRegion>
#include <QPushButton>
#include <QResizeEvent>

SelectionOverlay::SelectionOverlay(QWidget *parent)
    : QWidget(parent)
    , m_dragging(false)
    , m_color(QColor("red"))
    , m_finishButton(new QPushButton(tr("Finish"), this))
    , m_multiSelection(false)
{
    // Frameless overlay that floats above everything and lets each pixel be drawn
    // with its own transparency (so we can dim the screen but leave the selection area clear).
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setCursor(Qt::CrossCursor);

    m_finishButton->setVisible(false);
    m_finishButton->setCursor(Qt::PointingHandCursor);
    connect(m_finishButton, &QPushButton::clicked, this, [this]() {
        if (m_multiSelection)
            emit finishRequested();
        close();
    });
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

    if (m_selection.width() > 0 && m_selection.height() > 0) {
        m_completedSelections.append(m_selection);
        emit selectionFinished(m_selection);
        if (!m_multiSelection) {
            close();
        } else {
            m_selection = QRect();
            update();
        }
    } else {
        if (!m_multiSelection) {
            emit selectionCanceled();
            close();
        } else {
            m_selection = QRect();
            update();
        }
    }
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

void SelectionOverlay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateFinishButtonPosition();
}

void SelectionOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Show the live desktop dimmed everywhere except the selected rectangles so
    // the user can keep track of areas they've already captured.
    QRegion clearRegion;
    const QRect sel = m_selection.normalized();
    if (!sel.isNull())
        clearRegion = clearRegion.united(QRegion(sel));

    for (const QRect &stored : m_completedSelections)
        clearRegion = clearRegion.united(QRegion(stored));

    if (clearRegion.isEmpty()) {
        p.fillRect(rect(), QColor(0, 0, 0, 120));
    } else {
        QRegion outside(rect());
        outside = outside.subtracted(clearRegion);
        p.save();
        p.setClipRegion(outside);
        p.fillRect(rect(), QColor(0, 0, 0, 120));
        p.restore();
    }

    auto drawRect = [&p, this](const QRect &r) {
        if (r.isNull())
            return;
        p.setPen(QPen(m_color, 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(0, 0, -1, -1));
    };

    for (const QRect &stored : m_completedSelections)
        drawRect(stored);

    drawRect(sel);
}

void SelectionOverlay::setColor(const QColor &newColor)
{
    m_color = newColor;
}

void SelectionOverlay::setMultiSelectionEnabled(bool enabled)
{
    m_multiSelection = enabled;
    m_finishButton->setVisible(enabled);
    updateFinishButtonPosition();
    m_completedSelections.clear();
    m_selection = QRect();
    update();
}

void SelectionOverlay::removeLastSelection()
{
    if (m_completedSelections.isEmpty())
        return;
    m_completedSelections.removeLast();
    update();
}

void SelectionOverlay::updateFinishButtonPosition()
{
    if (!m_finishButton)
        return;

    const int margin = 16;
    const QSize btnSize = m_finishButton->sizeHint();
    m_finishButton->setFixedSize(btnSize);
    m_finishButton->move(width() - btnSize.width() - margin,
                         height() - btnSize.height() - margin);
}
