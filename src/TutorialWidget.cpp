#include "TutorialWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "Common.h"

TutorialWidget::TutorialWidget(QWidget *parent) :
    QFrame(parent),
    m_messageLabel{new QLabel},
    m_nextButton{new QPushButton},
    m_exitButton{new QPushButton}
{
    QHBoxLayout *lay = new QHBoxLayout(this);
    setLayout(lay);

    lay->addStretch();
    lay->addWidget(m_messageLabel);
    lay->addWidget(m_nextButton);
    lay->addWidget(m_exitButton);
    lay->addStretch();

    setStyleSheet("TutorialWidget {border: 15px solid #60B1C7;}");

    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setTextFormat(Qt::RichText);
    m_messageLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_messageLabel->setOpenExternalLinks(true);
    //m_messageLabel->setWordWrap(true);

    m_nextButton->setText(tr("Next"));
    m_nextButton->setStyleSheet(CSS_BLUE_BUTTON);

    m_exitButton->setText(tr("Exit"));
    m_exitButton->setStyleSheet(CSS_BLUE_BUTTON);
    connect(m_exitButton, &QPushButton::clicked, this, &TutorialWidget::onExitClicked);
    QSettings s;
    m_tutorialFinished = s.value("settings/tutorial_finished", false).toBool();
    if (m_tutorialFinished)
    {
        hide();
    }
    else
    {
        m_messageLabel->setText("<b>Moolticute Tutorial</b><br>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum venenatis sollicitudin erat, eget gravida mi elementum vel.<br>Integer sit amet faucibus lorem. Pellentesque mattis congue massa, vel laoreet ligula vulputate nec.<br>Curabitur pellentesque vitae neque vel placerat. Suspendisse erat ipsum, sodales et pulvinar non, iaculis eget ligula.<br>Sed vitae velit ultricies, dapibus nisl eu, malesuada magna. Phasellus sit amet pellentesque dolor, quis suscipit eros.");
        show();
    }
}

TutorialWidget::~TutorialWidget()
{

}

void TutorialWidget::setText(const QString &text)
{
    m_messageLabel->setText(text);
}

void TutorialWidget::onExitClicked()
{
    QSettings s;
    s.setValue("settings/tutorial_finished", true);
    hide();
}
