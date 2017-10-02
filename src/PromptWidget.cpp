#include "PromptWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>

#include "Common.h"

PromptWidget::PromptWidget(QWidget *parent) : QFrame(parent),
    m_messageLabel(new QLabel),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No))
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
    m_messageLabel->setText(tr("This is text message in PromptWidget.\n"
                               "And this is long long text"));

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &PromptWidget::accepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &PromptWidget::rejected);
}

void PromptWidget::setText(const QString &text)
{
    m_messageLabel->setText(text);
}
