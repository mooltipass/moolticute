/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#include "AppGui.h"
#include "version.h"
#include "QSimpleUpdater.h"

#ifdef Q_OS_MAC
#include "MacUtils.h"
#endif

#if defined(Q_OS_WIN)
    #define MC_UPDATE_URL   "https://calaos.fr/mooltipass/windows/updater.json"
#elif defined(Q_OS_MAC)
    #define MC_UPDATE_URL   "https://calaos.fr/mooltipass/macos/updater.json"
#else
    #define MC_UPDATE_URL   ""
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
    QCoreApplication::setApplicationVersion(APP_VERSION);

    Common::installMessageOutputHandler();

    QSimpleUpdater::getInstance();

    setAttribute(Qt::AA_UseHighDpiPixmaps);
    setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    QCommandLineOption autoLaunchedOption("autolaunched");
    parser.addOption(autoLaunchedOption);
    parser.process(*QCoreApplication::instance());

    bool autoLaunched = parser.isSet(autoLaunchedOption);

    if (!createSingleApplication())
        return false;

    qtAwesome()->initFontAwesome();

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
        if (reason ==  QSystemTrayIcon::DoubleClick
#ifndef Q_OS_WIN
         //On linux, some Desktop environnements such as KDE won't let the user emit a double click
         || reason == QSystemTrayIcon::Trigger
#endif
       ) {
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
    connect(wsClient, &WSClient::connectedChanged, this, &AppGui::connectedChanged);
    connect(wsClient, &WSClient::statusChanged, this, &AppGui::updateSystrayTooltip);
    connectedChanged();

    win = new MainWindow(wsClient);
    connect(win, &MainWindow::windowCloseRequested, [=]()
    {
        mainWindowHide();
    });

    autoLaunched ?  mainWindowHide() : mainWindowShow();

    connect(wsClient, &WSClient::showAppRequested, [=]()
    {
        mainWindowShow();
    });

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

    startSSHAgent();

    QTimer::singleShot(15000, this, &AppGui::checkUpdate);

    return true;
}

void AppGui::startSSHAgent()
{
    //Start ssh agent if needed
    QSettings s;
    if (s.value("settings/auto_start_ssh").toBool())
    {
        sshAgentProcess = new QProcess(this);
        QString program = QCoreApplication::applicationDirPath () + "/moolticute_ssh-agent";
        QStringList arguments;
#ifndef Q_OS_WIN
        arguments << "--no-fork";
#endif
        qDebug() << "Running " << program << " " << arguments;

        connect(sshAgentProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                [=](int exitCode, QProcess::ExitStatus exitStatus)
        {
            qDebug() << "SSH agent exits with error code " << exitCode << " Exit Status : " << exitStatus;

            //Restart agent
            QTimer::singleShot(500, [=]()
            {
                startSSHAgent();
            });
        });

        sshAgentProcess->start(program, arguments);
    }
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
        systray->setToolTip(tr("No moolltipass connected."));
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

void AppGui::updateSystrayTooltip()
{
    if(!wsClient->get_connected())
        return;

    const auto status = wsClient->get_status();

    const QString device_name = wsClient->isMPMini() ? tr("Mooltipass Mini") : tr("Mooltipass");

    QString msg;
    switch(status) {
        case Common::Locked:
        case Common::LockedScreen:
           msg = tr("%1 locked").arg(device_name);
           break;
        case Common::Unlocked:
           msg = tr("%1 Unlocked").arg(device_name);
           break;
       case Common::NoCardInserted:
           msg = tr("No card inserted in your %1").arg(device_name);
           break;
       default:
           break;
    }

    systray->setToolTip(msg);
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
    bool search = false;

    if (sharedMem.attach())
    {
        QJsonObject obj = Common::readSharedMemory(sharedMem);

        //PID is stored as string to prevent double conversion in json
        qint64 pid = obj["daemon_pid"].toString().toLongLong();

        if (Common::isProcessRunning(pid))
            search = true;

        sharedMem.detach();
    }

    if (search == foundDaemon)
        return;
    foundDaemon = search;

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

QtAwesome *AppGui::qtAwesome()
{
    static QtAwesome *a = new QtAwesome(qApp);
    return a;
}

void AppGui::checkUpdate()
{
    auto u = QSimpleUpdater::getInstance();
    u->setModuleVersion(MC_UPDATE_URL, APP_VERSION);
    u->setNotifyOnUpdate(MC_UPDATE_URL, true);
    u->setDownloaderEnabled(MC_UPDATE_URL, true);

    u->checkForUpdates(MC_UPDATE_URL);
}
