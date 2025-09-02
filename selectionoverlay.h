#ifndef SELECTIONOVERLAY_H
#define SELECTIONOVERLAY_H

#include <QWidget>
#include <QRect>
#include <QPoint>

class SelectionOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit SelectionOverlay(QWidget *parent = nullptr);
    QRect selectedRect() const;

signals:
    void selectionFinished(const QRect &rect);
    void selectionCanceled();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint  m_origin;
    QRect   m_selection;
    bool    m_dragging = false;
};

#endif // SELECTIONOVERLAY_H
