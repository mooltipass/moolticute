#include "TutorialWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Common.h"

const QString TutorialWidget::TUTORIAL_HEADER_TEXT = tr("<b>Moolticute Tutorial - %1</b><br><br>");

TutorialWidget::TutorialWidget(QWidget *parent) :
    QFrame(parent),
    m_messageLabel{new QLabel},
    m_nextButton{new QPushButton},
    m_exitButton{new QPushButton}
{
    QHBoxLayout *lay = new QHBoxLayout(this);
    setLayout(lay);

    m_messageLabel->setMinimumWidth(600);

    lay->addStretch();
    lay->addWidget(m_messageLabel);
    lay->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
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
    connect(m_nextButton, &QPushButton::clicked, this, &TutorialWidget::onNextClicked);

    QSettings s;
    m_tutorialFinished = s.value("settings/tutorial_finished", false).toBool();
    m_mw = static_cast<MainWindow*>(parent->parent());
    if (m_tutorialFinished)
    {
        hide();
    }
    else
    {
        initTutorial();
        startTutorial();
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

void TutorialWidget::displayCurrentTab()
{
    if (m_current_index != -1)
    {
        m_mw->ui->widgetHeader->setEnabled(true);
        m_tabs[m_current_index].button()->setEnabled(true);
    }
}

void TutorialWidget::onExitClicked()
{
    QSettings s;
    s.setValue("settings/tutorial_finished", true);
    m_tutorialFinished = true;
    hide();
    m_mw->updateTabButtons();
}

void TutorialWidget::onNextClicked()
{
    if (m_current_index != -1)
    {
        m_tabs[m_current_index].button()->setEnabled(false);
    }
    m_mw->ui->widgetHeader->setEnabled(true);
    m_current_index++;
    if (m_current_index >= m_tabs.size())
    {
        qCritical() << "No more tutorial page";
        QMessageBox::information(this, tr("Tutorial finished"), tr("Congratulation, you have completed the tutorial..."));
        onExitClicked();
        return;
    }
    QPushButton *button = m_tabs[m_current_index].button();
    button->setEnabled(true);
    QWidget* page = nullptr;
    foreach (QPushButton* v, m_mw->m_tabMap) {
            if (v == button)
               page = m_mw->m_tabMap.key(button);
        }
    if (page)
    {
        m_mw->ui->stackedWidget->setCurrentWidget(page);
    }
    else
    {
        qCritical() << "Cannot find page!";
    }
    m_messageLabel->setText(TUTORIAL_HEADER_TEXT.arg(button->text()) + m_tabs[m_current_index].text());
}

void TutorialWidget::initTutorial()
{
    Ui::MainWindow* ui = m_mw->ui;
    m_tabs = {
        {ui->pushButtonDevSettings, tr("test Device Settings") + "<br>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum venenatis sollicitudin erat, eget gravida mi elementum vel.<br>Integer sit amet faucibus lorem. Pellentesque mattis congue massa, vel laoreet ligula vulputate nec.<br>Curabitur pellentesque vitae neque vel placerat. Suspendisse erat ipsum, sodales et pulvinar non, iaculis eget ligula.<br>Sed vitae velit ultricies, dapibus nisl eu, malesuada magna. Phasellus sit amet pellentesque dolor, quis suscipit eros."},
        {ui->pushButtonCred, tr("test Credentials")},
        {ui->pushButtonNotes, tr("test Notes")},
        {ui->pushButtonFiles, tr("test Files")},
        {ui->pushButtonFido, tr("test Fido")},
        {ui->pushButtonSync, tr("test Sync")},
        {ui->pushButtonAppSettings, tr("test App Settigns")},
        {ui->pushButtonAbout, tr("test About")}
    };
}

void TutorialWidget::startTutorial()
{
    m_current_index = -1;
    m_messageLabel->setText(TUTORIAL_HEADER_TEXT.arg("WELCOME") + tr("Welcome to Mooltice tutorial! Please connect your device and unlock it to continue the tutorial."));
}
