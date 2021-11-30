#include "TutorialWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "MainWindow.h"
#include "ui_MainWindow.h"

const QString TutorialWidget::TUTORIAL_HEADER_TEXT = tr("<b>Moolticute Tutorial - %1</b>");
const QString TutorialWidget::TUTORIAL_FINISHED_SETTING = "settings/tutorial_finished";

TutorialWidget::TutorialWidget(QWidget *parent) :
    QFrame(parent),
    m_messageLabel{new QLabel},
    m_titleLabel{new QLabel},
    m_nextButton{new QPushButton},
    m_exitButton{new QPushButton}
{
    setMaximumHeight(TUTORIAL_MAX_HEIGHT);
    QHBoxLayout *lay = new QHBoxLayout(this);
    setLayout(lay);

    setupLabel(m_titleLabel);
    setupLabel(m_messageLabel);

    QVBoxLayout *tutorialTextLayout = new QVBoxLayout(this);
    tutorialTextLayout->addStretch();
    tutorialTextLayout->addWidget(m_titleLabel);
    tutorialTextLayout->addWidget(m_messageLabel);
    tutorialTextLayout->setSpacing(TUTORIAL_TEXT_SPACING);
    tutorialTextLayout->addStretch();
    m_messageLabel->setWordWrap(true);

    lay->addLayout(tutorialTextLayout);
    lay->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    lay->addWidget(m_nextButton);
    lay->addWidget(m_exitButton);
    lay->addStretch();

    setStyleSheet("TutorialWidget {border: 15px solid #60B1C7; border-color: red;} ");

    m_nextButton->setText(tr("Next"));
    m_nextButton->setStyleSheet(CSS_BLUE_BUTTON);

    m_exitButton->setText(tr("Exit"));
    m_exitButton->setStyleSheet(CSS_BLUE_BUTTON);
    connect(m_exitButton, &QPushButton::clicked, this, &TutorialWidget::onExitClicked);
    connect(m_nextButton, &QPushButton::clicked, this, &TutorialWidget::onNextClicked);

    QSettings s;
    m_tutorialFinished = s.value(TUTORIAL_FINISHED_SETTING, false).toBool();
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
        connect(m_mw->wsClient, &WSClient::statusChanged, this, &TutorialWidget::onStatusChanged);
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

void TutorialWidget::changeTutorialFinished(bool enabled)
{
    QSettings s;
    s.setValue(TUTORIAL_FINISHED_SETTING, !enabled);
}

void TutorialWidget::onExitClicked()
{
    QSettings s;
    s.setValue(TUTORIAL_FINISHED_SETTING, true);
    m_tutorialFinished = true;
    hide();
    disconnect(m_mw->wsClient, &WSClient::deviceConnected, this, &TutorialWidget::onDeviceConnected);
    disconnect(m_mw->wsClient, &WSClient::deviceDisconnected, this, &TutorialWidget::onDeviceDisconnected);
    disconnect(m_mw->wsClient, &WSClient::statusChanged, this, &TutorialWidget::onStatusChanged);
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
        QMessageBox::information(this, tr("Tutorial finished"), tr("Congratulation, you have completed the tutorial."));
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
    m_titleLabel->setText(TUTORIAL_HEADER_TEXT.arg(button->text()));
    m_messageLabel->setText(m_tabs[m_current_index].text());
}

void TutorialWidget::onDeviceConnected()
{
    if (!m_tutorialFinished)
    {
        bool connected = m_mw->wsClient->isMPBLE();
        if (connected)
        {
            m_nextButton->setEnabled(Common::MPStatus::Unlocked == m_mw->wsClient->get_status());
        }
        else
        {
            m_nextButton->setEnabled(false);
            QMessageBox::warning(this, tr("Tutorial warning"), tr("Tutorial is only available for BLE device, please attach one or Exit tutorial."));
        }
    }
}

void TutorialWidget::onDeviceDisconnected()
{
    if (!m_tutorialFinished)
    {
        m_nextButton->setEnabled(false);
        // Display connect device again
        startTutorial();
    }
}

void TutorialWidget::onStatusChanged(Common::MPStatus status)
{
    if (!m_tutorialFinished)
    {
        m_nextButton->setEnabled(Common::MPStatus::Unlocked == status);
    }
}

void TutorialWidget::initTutorial()
{
    Ui::MainWindow* ui = m_mw->ui;
    m_tabs = {
        {ui->pushButtonDevSettings, tr("You can use the ‘Device Settings’ tab to change parameters on your Mooltipass device. Hovering the mouse pointer over an option will gives a short description of what that option does.")},
        {ui->pushButtonCred, tr("In case you aren't using our browser extensions, this tab allows you to manually add credentials to your database. By clicking the \"Enter Credentials Management Mode\" button, you will be able to visualize and modify the credentials stored on your database.")},
        {ui->pushButtonNotes, tr("This tab allows you to securely store text on your device. Simply add a new note, set a title and type its contents.")},
        {ui->pushButtonFiles, tr("The Mooltipass device can operate as a flash drive. The ‘Files’ tab allows you to add, store and access small files on your Mooltipass.")},
        {ui->pushButtonFido, tr("Similarly to the credentials tab, the FIDO2 tab allows you to visualize and delete FIDO2 credentials stored on your device.")},
        {ui->pushButtonSync, tr("The different buttons in the synchronization tab will allow you to backup and restore the credentials you have stored inside the memory of your Mooltipass. You can also select a backup monitoring file to make sure your Mooltipass database is always in sync with it.")},
        {ui->pushButtonAppSettings, tr("The ‘Settings’ tab lets you change parameters on the Moolticute app.")},
        {ui->pushButtonAbout, tr("Finally, the ‘About’ tab provides you with general information about your device and app. It allows you to check that you are running the latest version of Moolticute.")}
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
    m_titleLabel->setText(TUTORIAL_HEADER_TEXT.arg("Welcome"));
    m_messageLabel->setText(tr("Welcome to Mooltice tutorial! Please connect your device and unlock it to continue the tutorial."));
}

void TutorialWidget::setupLabel(QLabel *label)
{
    label->setMinimumWidth(TUTORIAL_LABEL_WIDTH);
    label->setAlignment(Qt::AlignCenter);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setOpenExternalLinks(true);
    QFont font = label->font();
    font.setPointSize(TUTORIAL_FONT_SIZE);
    label->setFont(font);
}
