#include "PromptWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>

#include "Common.h"

QString PromptWidget::MMM_ERROR = tr("Memory Management Error");

PromptWidget::PromptWidget(QWidget *parent) :
    QFrame(parent),
    m_hideAfterAccepted(true),
    m_messageLabel(new QLabel),
    m_buttonBox(new QDialogButtonBox),
    m_promptMessage(nullptr)
{
    QHBoxLayout *lay = new QHBoxLayout(this);
    setLayout(lay);

    lay->addStretch();
    lay->addWidget(m_messageLabel);
    lay->addWidget(m_buttonBox);
    lay->addStretch();

    initButtons();

    setStyleSheet("PromptWidget {border: 5px solid #60B1C7;}");

    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setTextFormat(Qt::RichText);
    m_messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_messageLabel->setOpenExternalLinks(true);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &PromptWidget::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &PromptWidget::onRejected);
}

PromptWidget::~PromptWidget()
{
    delete m_promptMessage;
}

void PromptWidget::setText(const QString &text)
{
    m_messageLabel->setText(text);
}

void PromptWidget::setPromptMessage(PromptMessage *promptMessage)
{
    cleanPromptMessage();

    m_promptMessage = promptMessage;

    if (m_promptMessage)
        m_messageLabel->setText(m_promptMessage->getText());

    if (!promptMessage->containsButtonCb())
    {
        m_buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        for (auto btn : m_buttonBox->buttons())
        {
            btn->setStyleSheet(CSS_BLUE_BUTTON);
        }
    }
}

void PromptWidget::cleanPromptMessage()
{
    initButtons();

    if (m_promptMessage)
    {
        delete m_promptMessage;
        m_promptMessage = nullptr;
    }
}

bool PromptWidget::isMMMErrorPrompt() const
{
    return m_promptMessage && m_messageLabel->text().contains(MMM_ERROR);
}

void PromptWidget::onAccepted()
{
    if (m_hideAfterAccepted)
        hide();

    if (m_promptMessage)
        m_promptMessage->runAcceptCallBack();

    emit accepted();
}

void PromptWidget::onRejected()
{
    if (m_promptMessage)
        m_promptMessage->runRejectCallBack();

    emit rejected();
}

void PromptWidget::initButtons()
{
    m_buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
    for (auto btn : m_buttonBox->buttons())
    {
        btn->setStyleSheet(CSS_BLUE_BUTTON);
    }
}
