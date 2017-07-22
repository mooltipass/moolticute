#ifndef ANIMATEDCOLORBUTTON_H
#define ANIMATEDCOLORBUTTON_H

// Qt
#include <QPushButton>
#include <QTime>
class QPropertyAnimation;

class AnimatedColorButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(QColor bkgColor READ bkgColor WRITE setBkgColor)

public:
    AnimatedColorButton(const QColor &startColor=QColor(0, 0, 0), const QColor &endColor=QColor(240, 240, 240), int iAnimationDuration=1000, QWidget *parent = 0);
    void setBkgColor(const QColor &color);
    QColor bkgColor();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    QPropertyAnimation *m_pAnimation;
    bool m_bAnimationStarted;
    int m_iAnimationDuration;
    QTime m_tTimer;
};

#endif // ANIMATEDCOLORBUTTON_H
