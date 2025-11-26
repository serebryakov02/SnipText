#ifndef SELECTIONOVERLAY_H
#define SELECTIONOVERLAY_H

#include <QWidget>
#include <QRect>
#include <QPoint>
#include <QColor>
#include <QList>

class QPushButton;
class SelectionOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit SelectionOverlay(QWidget *parent = nullptr);
    void setColor(const QColor &newColor);
    void setMultiSelectionEnabled(bool enabled);
    bool multiSelectionEnabled() const { return m_multiSelection; }
    void removeLastSelection();

signals:
    void selectionFinished(const QRect &rect);
    void selectionCanceled();
    void finishRequested();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint  m_origin;
    QRect   m_selection;
    bool    m_dragging;
    QColor  m_color;
    QPushButton *m_finishButton;
    bool m_multiSelection;
    QList<QRect> m_completedSelections;

    void updateFinishButtonPosition();
};

#endif // SELECTIONOVERLAY_H
