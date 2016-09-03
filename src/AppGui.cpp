#include "AppGui.h"

#ifdef Q_OS_MAC
#include "MacUtils.h"
#endif

AppGui::AppGui(int & argc, char ** argv) :
    QApplication(argc, argv)
{
    QCoreApplication::setOrganizationName("Raoulh");
    QCoreApplication::setOrganizationDomain("raoulh.org");
    QCoreApplication::setApplicationName("Moolticute");

    Common::installMessageOutputHandler();

    systray = new QSystemTrayIcon(this);
    //    sys->setIcon(QIcon("C:/Users/phyatt/Downloads/system-tray.png"));
     QIcon icon(":/systray_disconnected.png");
    icon.setIsMask(true);
    systray->setIcon(icon);
    systray->show();

    showConfigApp = new QAction(tr("&Hide Moolticute configurator"), this);
    connect(showConfigApp, &QAction::triggered, [=](){
        if (win->isHidden())
            mainWindowShow();
        else
            mainWindowHide();
    });

    QAction* quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    QMenu * systrayMenu = new QMenu();
    systrayMenu->addAction(showConfigApp);
    systrayMenu->addSeparator();
    systrayMenu->addAction(quitAction);

    systray->setContextMenu(systrayMenu);

    wsClient = new WSClient(this);
    connect(wsClient, &WSClient::connectedChanged, [=]() { connectedChanged(); });
    connectedChanged();
    win = new MainWindow(wsClient);

    QProcess *daemon = new QProcess(this);
    QString program = QCoreApplication::applicationDirPath () + "/../bin/moolticuted";
    QStringList arguments;
    // TODO handle Debug arguments
    //arguments << "-e" <<  "-s 8080";
    qDebug() << "Running " << program << " " << arguments;
    daemon->startDetached(program, arguments);

    connect(daemon, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus){
       qDebug() << "Daemon exits with error code " << exitCode << " Exit Status : " << exitStatus;
    });

}

void AppGui::connectedChanged()
{
    if (!wsClient->get_connected())
    {
        QIcon icon(":/systray_disconnected.png");
        icon.setIsMask(true);
        systray->setIcon(icon);
    }
    else
    {
        QIcon icon(":/systray.png");
        icon.setIsMask(true);
        systray->setIcon(icon);
    }
}

AppGui::~AppGui()
{
    delete systray;
    delete win;
    delete wsClient;
}

void AppGui::mainWindowShow()
{
    if (!win)
        return;

    win->show();
    showConfigApp->setText(tr("&Hide Moolticute configurator"));
#ifdef Q_OS_MAC
   utils::mac::hideDockIcon(false);
#endif
}

void AppGui::mainWindowHide()
{
    if (!win)
        return;

    win->hide();
    showConfigApp->setText(tr("&Show Moolticute configurator"));
#ifdef Q_OS_MAC
   utils::mac::hideDockIcon(true);
#endif
}


void AppGui::enableDameon()
{
#if defined(Q_OS_MAC)
    QFileInfo file("~/Library/LaunchAgents/org.mooltipass.moolticute.plist");
    if (!file.exists())
        QFile::copy(":/org.mooltipass.moolticute.plist", "~/Library/LaunchAgents/org.mooltipass.moolticute.plist");
#endif
}

void AppGui::disableDaemon()
{
#if defined(Q_OS_MAC)
    QFileInfo file("~/Library/LaunchAgents/org.mooltipass.moolticute.plist");
    if (file.exists())
    {
        QFile::remove("~/Library/LaunchAgents/org.mooltipass.moolticute.plist");
    }
#endif
}
