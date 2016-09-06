#include "AppGui.h"

#ifdef Q_OS_MAC
#include "MacUtils.h"
#endif

AppGui::AppGui(int & argc, char ** argv) :
    QApplication(argc, argv)
{
}

bool AppGui::initialize()
{
    QCoreApplication::setOrganizationName("Raoulh");
    QCoreApplication::setOrganizationDomain("raoulh.org");
    QCoreApplication::setApplicationName("Moolticute");

    Common::installMessageOutputHandler();

    setAttribute(Qt::AA_UseHighDpiPixmaps);

    systray = new QSystemTrayIcon(this);
    QIcon icon(":/systray_disconnected.png");
#ifdef Q_OS_MAC
    icon.setIsMask(true);
#endif
    systray->setIcon(icon);
    systray->show();

    showConfigApp = new QAction(tr("&Show Moolticute configurator"), this);
    connect(showConfigApp, &QAction::triggered, [=](){
        if (win->isHidden())
            mainWindowShow();
        else
            mainWindowHide();
    });

    QAction* quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    QMenu *systrayMenu = new QMenu();

    daemonAction = new DaemonMenuAction(systrayMenu);
    systrayMenu->addAction(daemonAction);
    systrayMenu->addSeparator();
    systrayMenu->addAction(showConfigApp);
    systrayMenu->addSeparator();
    systrayMenu->addAction(quitAction);
    connect(systrayMenu, &QMenu::aboutToShow, [=]()
    {
        //Hack: On OSX the second time the menu is shown the widget is not drawn
        //Force a redraw make it visible again
        daemonAction->forceRepaint();
    });

    systray->setContextMenu(systrayMenu);

    wsClient = new WSClient(this);
    connect(wsClient, &WSClient::connectedChanged, [=]() { connectedChanged(); });
    connectedChanged();

    win = new MainWindow(wsClient);
    connect(win, &MainWindow::windowCloseRequested, [=]()
    {
        mainWindowHide();
    });
    //start hidden
    mainWindowHide();

    daemonProcess = new QProcess(this);
    QString program = QCoreApplication::applicationDirPath () + "/moolticuted";
    QStringList arguments;
    // TODO handle Debug arguments
    //arguments << "-e" <<  "-s 8080";
    qDebug() << "Running " << program << " " << arguments;
    daemonProcess->start(program, arguments);

    connect(daemonProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus)
    {
        qDebug() << "Daemon exits with error code " << exitCode << " Exit Status : " << exitStatus;

        if (aboutToQuit)
            return;

        daemonAction->updateStatus(DaemonMenuAction::StatusStopped);
        dRunning = false;

        if (needRestart)
        {
            daemonProcess->start(program, arguments);
            needRestart = false;
        }
    });

    connect(daemonProcess, &QProcess::started, [=]()
    {
        if (aboutToQuit)
            return;

        daemonAction->updateStatus(DaemonMenuAction::StatusRunning);
        dRunning = true;
    });

    connect(daemonAction, &DaemonMenuAction::restartClicked, [=]()
    {
        if (dRunning)
        {
            daemonProcess->kill();
            needRestart = true;
        }
        else
            daemonProcess->start(program, arguments);
    });

    return true;
}

void AppGui::connectedChanged()
{
    if (!wsClient->get_connected())
    {
        QIcon icon(":/systray_disconnected.png");
#ifdef Q_OS_MAC
        icon.setIsMask(true);
#endif
        systray->setIcon(icon);
    }
    else
    {
        QIcon icon(":/systray.png");
#ifdef Q_OS_MAC
        icon.setIsMask(true);
#endif
        systray->setIcon(icon);
    }
}

AppGui::~AppGui()
{
    aboutToQuit = true;
    delete wsClient;
    delete daemonProcess;
    delete daemonAction;
    delete systray;
    delete win;
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
