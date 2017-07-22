// Qt
#include <QMouseEvent>
#include <QPropertyAnimation>

// Application
#include "AnimatedColorButton.h"
#include "Common.h"

AnimatedColorButton::AnimatedColorButton(QWidget *parent, const QString &sStartColor, const QString &sEndColor,
    const QString &sDefaultText,
    const QString &sPressAndHoldText,
    int iAnimationDuration) : QPushButton(parent),
    m_pAnimation(nullptr),
    m_bAnimationStarted(false),
    m_iAnimationDuration(iAnimationDuration),
    m_sDefaultText(sDefaultText),
    m_sPressAndHoldText(sPressAndHoldText)
{
    m_cStartColor.setNamedColor(sStartColor);
    m_cEndColor.setNamedColor(sEndColor);
    m_pAnimation = new QPropertyAnimation(this, "bkgColor");
    m_pAnimation->setDuration(iAnimationDuration);
    m_pAnimation->setStartValue(m_cStartColor);
    m_pAnimation->setEndValue(m_cEndColor);
    QString sButtonStyleSheet = QString("QPushButton {" \
                                        "color: #000;" \
                                        "background-color: %1;" \
                                        "padding: 12px;" \
                                        "border: none;" \
                                        "}").arg(sStartColor);
    setText(m_sDefaultText);
    setStyleSheet(sButtonStyleSheet);
}

void AnimatedColorButton::setBkgColor(const QColor &color)
{
    QString sButtonStyleSheet = QString("QPushButton {" \
                                        "color: #000;" \
                                        "background-color: rgb(%1, %2, %3);" \
                                        "padding: 12px;" \
                                        "border: none;" \
                                        "}").arg(color.red()).arg(color.green()).arg(color.blue());
    setStyleSheet(sButtonStyleSheet);
}

QColor AnimatedColorButton::bkgColor()
{
    return Qt::black;
}

void AnimatedColorButton::setAnimationDuration(int iAnimationDuration)
{
    m_iAnimationDuration = iAnimationDuration;
}

int AnimatedColorButton::animationDuration() const
{
    return m_iAnimationDuration;
}

void AnimatedColorButton::setDefaultText(const QString &sDefaultText)
{
    m_sDefaultText = sDefaultText;
    setText(m_sDefaultText);
}

const QString &AnimatedColorButton::defaultText() const
{
    return m_sDefaultText;
}

void AnimatedColorButton::setPressAndHoldText(const QString &sPressAndHoldText)
{
    m_sPressAndHoldText = sPressAndHoldText;
}

const QString &AnimatedColorButton::pressAndHoldText() const
{
    return m_sPressAndHoldText;
}

void AnimatedColorButton::setStartColor(const QColor &cColor)
{
    m_cStartColor = cColor;
}

const QColor &AnimatedColorButton::startColor() const
{
    return m_cStartColor;
}

void AnimatedColorButton::setEndColor(const QColor &cEndColor)
{
    m_cEndColor = cEndColor;
}

const QColor &AnimatedColorButton::endColor() const
{
    return m_cEndColor;
}

void AnimatedColorButton::mousePressEvent(QMouseEvent *event)
{
    setText(m_sPressAndHoldText);
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
    setText(m_sDefaultText);
    m_pAnimation->stop();
    m_bAnimationStarted = false;
    QString sButtonStyleSheet = QString("QPushButton {" \
                                        "color: #000;" \
                                        "background-color: %1;" \
                                        "padding: 12px;" \
                                        "border: none;" \
                                        "}").arg(m_cStartColor.name());
    setStyleSheet(sButtonStyleSheet);
    if (m_tTimer.elapsed() > m_iAnimationDuration) {
        event->accept();
        emit actionValidated();
        hide();
    }
    else event->ignore();
}
