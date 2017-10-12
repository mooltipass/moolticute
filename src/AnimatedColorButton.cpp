// Qt
#include <QMouseEvent>
#include <QPropertyAnimation>

// Application
#include "AnimatedColorButton.h"
#include "Common.h"

AnimatedColorButton::AnimatedColorButton(QWidget *parent, int iAnimationDuration):
    QPushButton(parent),
    m_iAnimationDuration(iAnimationDuration)
{
    setStyleSheet(CSS_GREY_BUTTON);

    bar = new QWidget(this);
    bar->setMinimumHeight(3);
    bar->setMaximumHeight(3);
    bar->setStyleSheet("QWidget { background-color: #60B1C7; }");

    m_tTimer.setSingleShot(true);
    m_tTimer.setInterval(iAnimationDuration);
    connect(&m_tTimer, &QTimer::timeout, this, &AnimatedColorButton::onTimeOut);

    m_pAnimation = new QPropertyAnimation(this, "progress");
    m_pAnimation->setDuration(iAnimationDuration);
    m_pAnimation->setStartValue(0);
    m_pAnimation->setEndValue(width());

    connect(this, &AnimatedColorButton::clicked, this, &AnimatedColorButton::mouseClick);

    reset();
}

void AnimatedColorButton::setProgress(int p)
{
    qDebug() << "progress: " << p;
    bar->setMinimumWidth(p);
    bar->setMaximumWidth(p);
}

int AnimatedColorButton::getProgress()
{
    return bar->width();
}

void AnimatedColorButton::setAnimationDuration(int iAnimationDuration)
{
    m_iAnimationDuration = iAnimationDuration;
}

int AnimatedColorButton::animationDuration() const
{
    return m_iAnimationDuration;
}

void AnimatedColorButton::mousePressEvent(QMouseEvent *event)
{
    QSettings s;
    bool enabled = s.value("settings/long_press_cancel", true).toBool();

    if (enabled &&
        !m_tTimer.isActive())
    {
        m_tTimer.start();
        m_pAnimation->start();

        emit pressed();
    }
    else if (!enabled)
    {
        QPushButton::mousePressEvent(event);
    }
}

void AnimatedColorButton::mouseReleaseEvent(QMouseEvent *event)
{
    reset();

    QSettings s;
    bool enabled = s.value("settings/long_press_cancel", true).toBool();
    if (!enabled)
        QPushButton::mouseReleaseEvent(event);
}

void AnimatedColorButton::onTimeOut()
{
    reset();
    emit actionValidated();
}

void AnimatedColorButton::reset()
{
    m_tTimer.stop();
    m_pAnimation->stop();
    bar->setMinimumWidth(0);
    bar->setMaximumWidth(0);
}

void AnimatedColorButton::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_pAnimation->setEndValue(width());
}

void AnimatedColorButton::mouseClick()
{
    QSettings s;
    bool enabled = s.value("settings/long_press_cancel", true).toBool();
    if (!enabled)
        emit actionValidated();
}
