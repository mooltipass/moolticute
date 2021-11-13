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

    if (!m_tutorialFinished)
    {
        connect(m_mw->wsClient, &WSClient::deviceConnected, this, &TutorialWidget::onDeviceConnected);
        connect(m_mw->wsClient, &WSClient::deviceDisconnected, this, &TutorialWidget::onDeviceDisconnected);
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
    for (auto* tabButton : m_mw->m_tabMap)
    {
        if (tabButton == button)
        {
           page = m_mw->m_tabMap.key(button);
        }
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

void TutorialWidget::onDeviceConnected()
{
    if (!m_tutorialFinished)
    {
        bool connected = m_mw->wsClient->isConnected();
        m_nextButton->setEnabled(connected);
        if (!connected)
        {
            QMessageBox::warning(this, tr("Tutorial warning"), tr("Tutorial is only available for BLE device, please attach one or Exit tutorial."));
        }
    }
}

void TutorialWidget::onDeviceDisconnected()
{
    if (!m_tutorialFinished)
    {
        m_nextButton->setEnabled(true);

        //TODO: Restart tutorial?
    }
}

void TutorialWidget::initTutorial()
{
    Ui::MainWindow* ui = m_mw->ui;
    m_tabs = {
        {ui->pushButtonDevSettings, tr("You can use the ‘Device Settings’ tab to change parameters on your Mooltipass device.<br>Hovering the mouse pointer over an option will gives a short description of what that option does.")},
        {ui->pushButtonCred, tr("In case you aren't using our browser extensions, this tab allows you to manually add credentials to your database.<br>By clicking the \"Enter Credentials Management Mode\" button, you will be able to visualize and<br>modify the credentials stored on your database.")},
        {ui->pushButtonNotes, tr("This tab allows you to securely store text on your device. Simply add a new note, set a title and type its contents.")},
        {ui->pushButtonFiles, tr("The Mooltipass device can operate as a flash drive.<br>The ‘Files’ tab allows you to add, store and access small files on your Mooltipass.")},
        {ui->pushButtonFido, tr("Similarly to the credentials tab, the FIDO2 tab allows you to visualize and delete FIDO2 credentials stored on your device.")},
        {ui->pushButtonSync, tr("The different buttons in the synchronization tab will allow you to backup and<br>restore the credentials you have stored inside the memory of your Mooltipass.<br>You can also select a backup monitoring file to make sure your Mooltipass database is always in sync with it.")},
        {ui->pushButtonAppSettings, tr("The ‘Settings’ tab lets you change parameters on the Moolticute app.")},
        {ui->pushButtonAbout, tr("Finally, the ‘About’ tab provides you with general information about your device and app.<br>It allows you to check that you are running the latest version of Moolticute.")}
    };
}

void TutorialWidget::startTutorial()
{
    m_current_index = -1;
    auto connected = m_mw->wsClient->isConnected();
    if (!connected)
    {
        m_nextButton->setEnabled(false);
    }
    m_messageLabel->setText(TUTORIAL_HEADER_TEXT.arg("WELCOME") + tr("Welcome to Mooltice tutorial! Please connect your device and unlock it to continue the tutorial."));
}
