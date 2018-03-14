#ifndef ANIMATEDCOLORBUTTON_H
#define ANIMATEDCOLORBUTTON_H

// Qt
#include <QPushButton>
#include <QTimer>
class QPropertyAnimation;

class AnimatedColorButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(int progress READ getProgress WRITE setProgress)
    QString originalText;
public:
    AnimatedColorButton(QWidget *parent = nullptr, int iAnimationDuration = 1300);

    void setAnimationDuration(int iAnimationDuration);
    int animationDuration() const;

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    void setProgress(int p);
    int getProgress();
    void reset();

private slots:
    void mouseClick();

private:
    virtual void resizeEvent(QResizeEvent *event);

    QPropertyAnimation *m_pAnimation = nullptr;
    int m_iAnimationDuration;
    QTimer m_tTimer;
    QWidget *bar = nullptr;

public slots:
    void onTimeOut();

signals:
    void actionValidated();
};

#endif // ANIMATEDCOLORBUTTON_H
