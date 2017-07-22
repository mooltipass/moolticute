// Qt
#include <QMouseEvent>
#include <QPropertyAnimation>

// Application
#include "AnimatedColorButton.h"

AnimatedColorButton::AnimatedColorButton(QWidget *parent, const QColor &startColor, const QColor &endColor, int iAnimationDuration) : QPushButton(parent),
    m_pAnimation(nullptr), m_bAnimationStarted(false), m_iAnimationDuration(iAnimationDuration)
{
    m_pAnimation = new QPropertyAnimation(this, "bkgColor");
    m_pAnimation->setDuration(iAnimationDuration);
    m_pAnimation->setStartValue(startColor);
    m_pAnimation->setEndValue(endColor);
}

void AnimatedColorButton::setBkgColor(const QColor &color)
{
    setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
}

QColor AnimatedColorButton::bkgColor()
{
    return Qt::black;
}

void AnimatedColorButton::mousePressEvent(QMouseEvent *event)
{
    if (!m_bAnimationStarted)
    {
        m_tTimer.restart();
        m_pAnimation->start();
        m_bAnimationStarted = true;
    }
    else event->ignore();
}

void AnimatedColorButton::mouseReleaseEvent(QMouseEvent *event)
{
    m_pAnimation->stop();
    m_bAnimationStarted = false;
    setBkgColor(m_pAnimation->startValue().value<QColor>());

    if (m_tTimer.elapsed() > m_iAnimationDuration)
        event->accept();
    else event->ignore();
}
