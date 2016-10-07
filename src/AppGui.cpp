#include "AppGui.h"

#ifdef Q_OS_MAC
#include "MacUtils.h"
#endif

AppGui::AppGui(int & argc, char ** argv) :
    QApplication(argc, argv),
    sharedMem("moolticute")
{
}

bool AppGui::initialize()
{
    QCoreApplication::setOrganizationName("Raoulh");
    QCoreApplication::setOrganizationDomain("raoulh.org");
    QCoreApplication::setApplicationName("Moolticute");

    Common::installMessageOutputHandler();

    setAttribute(Qt::AA_UseHighDpiPixmaps);
	setQuitOnLastWindowClosed(false);

    if (!createSingleApplication())
        return false;

    systray = new QSystemTrayIcon(this);
    QIcon icon(":/systray_disconnected.png");
#ifdef Q_OS_MAC
    icon.setIsMask(true);
#endif
    systray->setIcon(icon);
    systray->show();

    showConfigApp = new QAction(tr("&Show Moolticute configurator"), this);
    connect(showConfigApp, &QAction::triggered, [=]()
    {
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

    connect(systray, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason)
    {
        // On Linux/Windows, hide/show the app when the tray icon is clicked
        // On OSX this just shows the menu
#ifndef Q_OS_MACX
        if (reason == QSystemTrayIcon::DoubleClick ||
            reason == QSystemTrayIcon::Trigger)
        {
            if (win->isHidden())
                mainWindowShow();
            else
                mainWindowHide();
        }
#else
        Q_UNUSED(reason)
#endif
    });

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

    connect(daemonProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus)
    {
        qDebug() << "Daemon exits with error code " << exitCode << " Exit Status : " << exitStatus;

        if (aboutToQuit)
            return;

        dRunning = false;

        if (needRestart)
        {
            QTimer::singleShot(1500, [=]()
            {
                QStringList args = arguments;
                args << "-s 8484";
                daemonProcess->start(program, args);
            });
            needRestart = false;
        }
    });

    connect(daemonProcess, &QProcess::started, [=]()
    {
        qDebug() << "Daemon started";
        if (aboutToQuit)
            return;

        dRunning = true;
    });

    //search for a potential daemon already running
    searchDaemonTick();

    //only start daemon if none are found
    if (!foundDaemon)
        daemonProcess->start(program, arguments);

    connect(daemonAction, &DaemonMenuAction::restartClicked, [=]()
    {
        //We don't have control over daemon process, cannot restart it
        if (daemonProcess->state() != QProcess::Running && foundDaemon)
        {
            QMessageBox::information(nullptr, "Moolticute",
                                     tr("Can't restart daemon, it was started by hand and not using this App."));
            return;
        }

        if (dRunning)
        {
            daemonProcess->kill();
            needRestart = true;
        }
        else
        {
            QTimer::singleShot(1500, [=]()
            {
                QStringList args = arguments;
                args << "-s 8484";
                daemonProcess->start(program, args);
            });
        }
    });

    timerDaemon = new QTimer(this);
    connect(timerDaemon, SIGNAL(timeout()), SLOT(searchDaemonTick()));
    timerDaemon->start(800);

    return true;
}

void AppGui::connectedChanged()
{
    if (!wsClient->get_connected())
    {
#ifdef Q_OS_WIN
        QIcon icon(":/systray_disconnected_white.png");
#else
        QIcon icon(":/systray_disconnected.png");
#endif

#ifdef Q_OS_MAC
        icon.setIsMask(true);
#endif
        systray->setIcon(icon);
    }
    else
    {
#ifdef Q_OS_WIN
        QIcon icon(":/systray_white.png");
#else
        QIcon icon(":/systray.png");
#endif

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
    showConfigApp->setText(tr("&Hide Moolticute App"));
#ifdef Q_OS_MAC
    utils::mac::hideDockIcon(false);
    utils::mac::orderFrontRegardless(win->winId(), true);
#endif
}

void AppGui::mainWindowHide()
{
    if (!win)
        return;

    win->hide();
    showConfigApp->setText(tr("&Show Moolticute App"));
#ifdef Q_OS_MAC
    utils::mac::hideDockIcon(true);
#endif
}

void AppGui::enableDaemon()
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

void AppGui::searchDaemonTick()
{
    //Search for the daemon from the shared mem segment
    foundDaemon = false;

    if (sharedMem.attach())
    {
        QJsonObject obj = Common::readSharedMemory(sharedMem);

        //PID is stored as string to prevent double conversion in json
        qint64 pid = obj["daemon_pid"].toString().toLongLong();

        if (Common::isProcessRunning(pid))
            foundDaemon = true;

        sharedMem.detach();
    }

    if (foundDaemon)
        daemonAction->updateStatus(DaemonMenuAction::StatusRunning);
    else if (needRestart)
        daemonAction->updateStatus(DaemonMenuAction::StatusRestarting);
    else
        daemonAction->updateStatus(DaemonMenuAction::StatusStopped);

    if (foundDaemon)
    {
        delete logSocket;
        logSocket = new QLocalSocket(this);
        logSocket->connectToServer(MOOLTICUTE_DAEMON_LOG_SOCK);
        connect(logSocket, SIGNAL(readyRead()), this, SLOT(daemonLogRead()));
    }
}

bool AppGui::createSingleApplication()
{
    QString serverName = QApplication::organizationName() + QApplication::applicationName();
    serverName.replace(QRegExp("[^\\w\\-. ]"), "");

    // Attempt to connect to the LocalServer
    QLocalSocket *localSocket = new QLocalSocket();
    localSocket->connectToServer(serverName);
    if(localSocket->waitForConnected(1000))
    {
        //App is already running. Send a "show" command to open the existing App

        qInfo() << "AppGui already running.";
        localSocket->write("show");
        localSocket->waitForBytesWritten();
        localSocket->close();
        delete localSocket;

        return false;
    }
    else
    {
        delete localSocket;

        // If the connection is unsuccessful, this is the main process
        // So we create a Local Server
        localServer = new QLocalServer();
        localServer->removeServer(serverName);
        localServer->listen(serverName);
        QObject::connect(localServer, SIGNAL(newConnection()), this, SLOT(slotConnectionEstablished()));
    }

    return true;
}

void AppGui::slotConnectionEstablished()
{
    //We have a new instance that wants to be opened
    QLocalSocket *currSocket = localServer->nextPendingConnection();
    connect(currSocket, &QLocalSocket::readyRead, [=]()
    {
        QString data(currSocket->readAll());

        if (data == "show")
        {
            mainWindowShow();

#ifdef Q_OS_WIN
            //On windows the window is not forced at front sometimes. So force it
            HWND hWnd = (HWND)win->winId();
            ::ShowWindow(hWnd, SW_SHOW);
            ::BringWindowToTop(hWnd);
            ::SetForegroundWindow(hWnd);

            //-- on Windows 7, this workaround brings window to top
            ::SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            ::SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
#endif
        }
    });
    connect(currSocket, &QLocalSocket::disconnected, [=]()
    {
        delete currSocket;
    });
}

void AppGui::daemonLogRead()
{
    while (logSocket->canReadLine())
    {
        QByteArray out = logSocket->readLine();
        win->daemonLogAppend(out);
        //qDebug() << QString::fromUtf8(out);
    }
}
