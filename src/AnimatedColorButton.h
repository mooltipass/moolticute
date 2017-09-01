#ifndef ANIMATEDCOLORBUTTON_H
#define ANIMATEDCOLORBUTTON_H

// Qt
#include <QPushButton>
#include <QTimer>
class QPropertyAnimation;

class AnimatedColorButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(QColor bkgColor READ bkgColor WRITE setBkgColor)

public:
    AnimatedColorButton(QWidget *parent = nullptr,
                        const QString &sStartColor = "lightgray",
                        const QString &sEndColor = "red",
                        const QString &sDefaultText = "DEFAULT",
                        const QString &sPressAndHoldText = "PRESS AND HOLD",
                        int iAnimationDuration = 2000);

    void setAnimationDuration(int iAnimationDuration);
    int animationDuration() const;

    void setDefaultText(const QString &sDefaultText);
    const QString &defaultText() const;

    void setPressAndHoldText(const QString &sPressAndHoldText);
    const QString &pressAndHoldText() const;

    void setStartColor(const QColor &cColor);
    const QColor &startColor() const;

    void setEndColor(const QColor &cEndColor);
    const QColor &endColor() const;

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    void setBkgColor(const QColor &color);
    QColor bkgColor();
    void reset();

private:
    QPropertyAnimation *m_pAnimation;
    int m_iAnimationDuration;
    QString m_sDefaultText;
    QString m_sPressAndHoldText;
    QColor m_cStartColor;
    QColor m_cEndColor;
    QTimer m_tTimer;

public slots:
    void onTimeOut();

signals:
    void actionValidated();
};

#endif // ANIMATEDCOLORBUTTON_H
