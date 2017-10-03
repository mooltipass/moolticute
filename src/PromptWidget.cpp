#include "PromptWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>

#include "Common.h"

PromptWidget::PromptWidget(QWidget *parent) : QFrame(parent),
    m_hideAfterAccepted(true),
    m_messageLabel(new QLabel),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No)),
    m_promptMessage(nullptr)
{
    QHBoxLayout *lay = new QHBoxLayout(this);
    setLayout(lay);

    lay->addStretch();
    lay->addWidget(m_messageLabel);
    lay->addWidget(m_buttonBox);
    lay->addStretch();

    for(auto btn : m_buttonBox->buttons())
        btn->setStyleSheet(CSS_BLUE_BUTTON);
    setStyleSheet("PromptWidget {border: 5px solid #60B1C7;}");

    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &PromptWidget::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &PromptWidget::rejected);
}

void PromptWidget::setText(const QString &text)
{
    m_messageLabel->setText(text);
}

void PromptWidget::setPromptMessage(PromptMessage *promptMessage)
{
    if(m_promptMessage)
        delete m_promptMessage;

    m_promptMessage = promptMessage;

    if(m_promptMessage)
        m_messageLabel->setText(m_promptMessage->getText());
}

void PromptWidget::onAccepted()
{
    if(m_hideAfterAccepted)
        hide();

    if(m_promptMessage)
    {
        m_promptMessage->runCallBack();
        delete m_promptMessage;
        m_promptMessage = nullptr;
    }

    emit accepted();
}
