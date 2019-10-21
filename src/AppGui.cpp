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
#include "DbMasterController.h"
#include "PromptWidget.h"
#include "SystemNotifications/SystemNotification.h"
#include "DeviceDetector.h"

#ifdef Q_OS_WIN
#include "SystemNotifications/SystemNotificationWindows.h"
#endif

#ifdef Q_OS_MAC
#include "MacUtils.h"
#endif

#ifndef APP_RELEASE_TESTING
    #define GITHUB_UPDATE_URL "https://api.github.com/repos/mooltipass/moolticute/releases"
#else
#if defined(Q_OS_MAC)
    #define GITHUB_UPDATE_URL   "http://mooltipass-tests.com/mc_betas/updater_osx.json"
#else
    #define GITHUB_UPDATE_URL   "http://mooltipass-tests.com/mc_betas/updater.json"
#endif
#endif

//5 min
#define GUI_STARTUP_DELAY   5 * 60 * 1000

AppGui::AppGui(int & argc, char ** argv) :
    QApplication(argc, argv),
    sharedMem("moolticute"),
    dbMasterController(nullptr)
{
}

bool AppGui::initialize()
{
    qsrand(time(NULL));

    QCoreApplication::setOrganizationName("mooltipass");
    QCoreApplication::setOrganizationDomain("themooltipass.com");
    QCoreApplication::setApplicationName("moolticute");
    QCoreApplication::setApplicationVersion(APP_VERSION);

    QSettings s;

    Common::installMessageOutputHandler(nullptr, [this](const QByteArray &d)
    {
        logBuffer.append(d);

        if (win)
        {
            win->daemonLogAppend(logBuffer);
            logBuffer.clear();
        }
    });

    qInfo() << "------------------------------------";
    qInfo() << "Moolticute Gui version: " << APP_VERSION;
    qInfo() << "(c) The Mooltipass Team";
    qInfo() << "https://github.com/mooltipass/moolticute";
    qInfo() << "------------------------------------";

    setupLanguage();

    QSimpleUpdater::getInstance();

#ifdef Q_OS_WIN
    SystemNotification::instance();
#endif
    DeviceDetector::instance();

    setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    QCommandLineOption autoLaunchedOption("autolaunched");
    parser.addOption(autoLaunchedOption);
    parser.process(*QCoreApplication::instance());

    bool autoLaunched = parser.isSet(autoLaunchedOption);

    if (!createSingleApplication())
        return false;

    systray = new QSystemTrayIcon(this);
    QIcon icon(":/systray_disconnected" + s.value("settings/systray_icon").toString() + ".png");
#ifdef Q_OS_MAC
    icon.setIsMask(true);
#endif
    systray->setIcon(icon);
    systray->show();

#ifdef Q_OS_WIN
    const auto *winNotification = dynamic_cast<SystemNotificationWindows*>(SystemNotification::instance().getNotification());
    if (winNotification != nullptr)
    {
        connect(winNotification, &SystemNotificationWindows::notifySystray,
                [this] (QString title, QString text)
                {
                    if (systray)
                    {
                        systray->showMessage(title, text, QIcon(":/AppIcon_128.png"));
                    }
                }
        );
    }
#endif

    showConfigApp = new QAction(tr("&Show Moolticute Application"), this);
    connect(showConfigApp, &QAction::triggered, [=]()
    {
        if (!win || win->isHidden())
            mainWindowShow();
        else
            mainWindowHide();
    });

    QAction* quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit, Qt::QueuedConnection);

    QMenu *systrayMenu = new QMenu();

#ifndef Q_OS_WIN
    // Custom systray widgets are not working on linux
    // And it fails too much on macos, see issue #132
    restartDaemonAction = new QAction(tr("&Restart daemon"), this);
    connect(restartDaemonAction, &QAction::triggered, this, &AppGui::restartDaemon);
    systrayMenu->addAction(restartDaemonAction);
#else
    daemonAction = new DaemonMenuAction(systrayMenu);
    systrayMenu->addAction(daemonAction);
#endif

    systrayMenu->addSeparator();
    systrayMenu->addAction(showConfigApp);
    systrayMenu->addSeparator();
    systrayMenu->addAction(quitAction);

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
            if (!win || win->isHidden())
                mainWindowShow();
            else
                mainWindowHide();
        }
#else
        Q_UNUSED(this)
        Q_UNUSED(reason)
#endif
    });

    wsClient = new WSClient(this);
    connect(wsClient, &WSClient::connectedChanged, this, &AppGui::connectedChanged);
    connect(wsClient, &WSClient::statusChanged, this, &AppGui::updateSystrayTooltip);
    connect(wsClient, &WSClient::statusChanged, [this]()
    {
        if (wsClient->get_status() == Common::UnkownSmartcad)
           mainWindowShow();
    });

    connect(wsClient, &WSClient::displayStatusWarning, this, &AppGui::displayStatusWarningNotification);

    connectedChanged();

    dbMasterController = new DbMasterController(this);
    dbMasterController->setWSClient(wsClient);

    mainWindowShow(autoLaunched);

    connect(wsClient, &WSClient::showAppRequested, [=]()
    {
        mainWindowShow();
    });

    daemonProcess = new QProcess(this);
    QString program = QCoreApplication::applicationDirPath () + "/moolticuted";
    QStringList arguments;

    if (s.value("settings/enable_dev_log").toBool())
        arguments << "-l";
    if (s.value("settings/http_dev_server").toBool())
        arguments << "-s 8484";

    qInfo() << "Running " << program << " " << arguments;

    connect(daemonProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus)
    {
        qWarning() << "Daemon exits with error code " << exitCode << " Exit Status : " << exitStatus;

        if (aboutToQuit)
            return;

        dRunning = false;

        if (needRestart)
        {
            QTimer::singleShot(1500, [=]()
            {
                daemonProcess->start(program, arguments);
            });
            needRestart = false;
        }
    });

    connect(daemonProcess, &QProcess::started, [=]()
    {
        qInfo() << "Daemon started";
        if (aboutToQuit)
            return;

        dRunning = true;
    });

    //search for a potential daemon already running
    searchDaemonTick();

    //only start daemon if none are found
    if (!foundDaemon)
        daemonProcess->start(program, arguments);

#ifdef Q_OS_WIN
    connect(daemonAction, &DaemonMenuAction::restartClicked, this, &AppGui::restartDaemon);
#endif

    timerDaemon = new QTimer(this);
    connect(timerDaemon, SIGNAL(timeout()), SLOT(searchDaemonTick()));
    timerDaemon->start(800);

    startSSHAgent();

    connect(QSimpleUpdater::getInstance(), &QSimpleUpdater::updateAvailable, this, &AppGui::updateAvailableReceived);

    QTimer::singleShot(15000, [this]() {
        checkUpdate(false);
    });

    QTimer::singleShot(GUI_STARTUP_DELAY, [this]()
    {
        //After some delays create the MainWindow if it's not done yet
        createMainWindow();
    });

#ifdef Q_OS_MAC
    //Workaround for Qt display issues for Dark Mode
    this->setStyleSheet("QComboBox, QRadioButton, QCheckBox:unchecked, QCheckBox:checked, QLabel, QPushButton"
                          "{ color: black }");
#endif

    return true;
}

void AppGui::startSSHAgent()
{
    //Start ssh agent if needed
    QSettings s;
    if (s.value("settings/auto_start_ssh").toBool())
    {
        sshAgentProcess = new QProcess(this);
        QString program = QCoreApplication::applicationDirPath () + "/mc-agent";
        QStringList arguments;
#ifndef Q_OS_WIN
        arguments << "--no-fork";
#endif
        QString uargs = s.value("settings/ssh_args").toString();
        if (uargs != "")
        {
            QStringList userArgs = uargs.split(' ');
            if (!userArgs.isEmpty())
                arguments.append(userArgs);
        }

        qInfo() << "Running " << program << " " << arguments;

        connect(sshAgentProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                [=](int exitCode, QProcess::ExitStatus exitStatus)
        {
            qWarning() << "SSH agent exits with error code " << exitCode << " Exit Status : " << exitStatus;

            //Restart agent
            QTimer::singleShot(500, [=]()
            {
                startSSHAgent();
            });
        });

        if (arguments.isEmpty())
            sshAgentProcess->start(program);
        else
            sshAgentProcess->start(program, arguments);
    }
}

void AppGui::connectedChanged()
{
    QSettings s;
    QIcon icon;
    if (!wsClient->get_connected())
    {
        if (s.contains("settings/systray_icon"))
        {
            icon = QIcon(":/systray_disconnected" + s.value("settings/systray_icon").toString() + ".png");
        }
        else
        {
#ifdef Q_OS_WIN
            icon = QIcon(":/systray_disconnected_white.png");
            s.setValue("settings/systray_icon", "_white");
#else
            icon = QIcon(":/systray_disconnected.png");
            s.setValue("settings/systray_icon", "");
#endif
        }

#ifdef Q_OS_MAC
        icon.setIsMask(true);
#endif
        systray->setIcon(icon);
        systray->setToolTip(tr("No mooltipass connected."));
    }
    else
    {
        if (s.contains("settings/systray_icon"))
        {
            icon = QIcon(":/systray" + s.value("settings/systray_icon").toString() + ".png");
        }
        else
        {
#ifdef Q_OS_WIN
            icon = QIcon(":/systray_white.png");
            s.setValue("settings/systray_icon", "_white");
#else
            icon = QIcon(":/systray.png");
            s.setValue("settings/systray_icon", "");
#endif
        }

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

    const QString device_name = wsClient->isMPMini() ? "Mooltipass Mini": "Mooltipass";

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
    if (timerDaemon)
        timerDaemon->stop();

#ifndef Q_OS_WIN
    if (sshAgentProcess)
    {
        //First let mc-agent clean gracefully
        sshAgentProcess->terminate();
    }
#else
    delete sshAgentProcess;
#endif

    aboutToQuit = true;
    delete wsClient;
    delete daemonProcess;
    delete daemonAction;
    delete systray;
    delete win;
}

void AppGui::mainWindowShow(bool autoLaunched)
{
    if (!win)
    {
        //Postpone qtawesome initialisation when the windows is showed
        //This fix a crash when starting the app with system in macOS
        createMainWindow();
        if (autoLaunched)
        {
            return;
        }
    }

    win->show();
#ifdef Q_OS_MAC
    win->raise();
    win->activateWindow();
#endif

    showConfigApp->setText(tr("&Hide Moolticute App"));
#ifdef Q_OS_MAC
    utils::mac::hideDockIcon(false);
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

#ifndef Q_OS_WIN
    restartDaemonAction->setEnabled(foundDaemon && !needRestart);
    if (!foundDaemon && needRestart)
        restartDaemonAction->setText(tr("Restarting daemon..."));
    else
        restartDaemonAction->setText(tr("&Restart daemon"));
#else
    if (foundDaemon)
        daemonAction->updateStatus(DaemonMenuAction::StatusRunning);
    else if (needRestart)
        daemonAction->updateStatus(DaemonMenuAction::StatusRestarting);
    else
        daemonAction->updateStatus(DaemonMenuAction::StatusStopped);
#endif
    if (foundDaemon)
    {
        //Force reopen connection, this prevents waiting for a timeout
        //before retrying the connection. On macOS the timeout being too long
        //the gui waits too much before reconnecting
        wsClient->closeWebsocket();
        wsClient->openWebsocket();

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

    connect(currSocket, &QLocalSocket::disconnected,
            currSocket, &QLocalSocket::deleteLater);

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
}

void AppGui::daemonLogRead()
{
    while (logSocket->canReadLine())
    {
        logBuffer.append(logSocket->readLine());

        if (win)
        {
            win->daemonLogAppend(logBuffer);
            logBuffer.clear();
        }
        //qDebug() << QString::fromUtf8(out);
    }
}

void AppGui::updateAvailableReceived(QString version, QString changesetURL)
{
    const QString NEWVERSION_STRING = tr("%1 version has been released!").arg(version);
    SystemNotification::instance().createNotification(NEWVERSION_STRING, tr("Open MC client to download it."));

    const auto onAccept = []()
    {
        QSimpleUpdater::getInstance()->downloadFile(GITHUB_UPDATE_URL);
    };

    const auto onReject = [this]()
    {
        win->hidePrompt();
    };

    PromptMessage *message = new PromptMessage("<b>" + NEWVERSION_STRING + "</b><br>" +
                                                  tr("Would you like to download the update now?") + "<br>" +
                                                  "<a href=\"" + changesetURL + "\">" + tr("Open changelog") + "</a>",
                                               onAccept, onReject);
    win->showPrompt(message);
}

void AppGui::displayStatusWarningNotification()
{
    const auto actStatus = wsClient->get_status();
    //Init with Error8, because it is not used
    static Common::MPStatus lastStatus = Common::Error8;

    if (actStatus != lastStatus)
    {
        lastStatus = actStatus;
        QString title, message;
        if (actStatus == Common::UnknownStatus)
        {
            title = tr("Mooltipass Not Connected");
            message = tr("Please Connect Your Mooltipass");
        }
        else if (actStatus == Common::NoCardInserted)
        {
            title = tr("No Card in Mooltipass!");
            message = tr("Please Insert Your Smartcard and Enter Your PIN");
        }
        else if (actStatus == Common::Unlocked && wsClient->get_memMgmtMode())
        {
            title = tr("Mooltipass in Management Mode!");
            message = tr("Please leave management mode in the App");
        }
        else if (actStatus != Common::Unlocked)
        {
            title = tr("Mooltipass Locked");
            message = tr("Please Unlock Your Mooltipass");
        }
        else
        {
            return;
        }
        SystemNotification::instance().createNotification(title, message);
    }
}

QtAwesome *AppGui::qtAwesome()
{
    static QtAwesome *a = new QtAwesome(qApp);
    return a;
}

void AppGui::restartDaemon()
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
            QStringList args = daemonProcess->arguments();
            daemonProcess->start(daemonProcess->program(), args);
        });
    }
}

void AppGui::checkUpdate(bool displayMessage)
{
    if (QStringLiteral(APP_VERSION) == "git" ||
        QStringLiteral(APP_TYPE) == "deb")
        return;

    auto u = QSimpleUpdater::getInstance();
    u->setModuleVersion(GITHUB_UPDATE_URL, APP_VERSION);
    u->setNotifyOnUpdate(GITHUB_UPDATE_URL, true);
    u->setDownloaderEnabled(GITHUB_UPDATE_URL, true);
    u->setNotifyOnFinish(GITHUB_UPDATE_URL, displayMessage);
    u->setDisplayDialog(GITHUB_UPDATE_URL, false);
    u->setPlatformKey(GITHUB_UPDATE_URL, APP_TYPE);

    u->checkForUpdates(GITHUB_UPDATE_URL);

    //Recheck in at least 30minutes plus some random time
    if (!displayMessage)
        QTimer::singleShot(1000 * 60 * 60 * 30 + qrand() % 240, [this]() { checkUpdate(false); });
}

QString AppGui::getDataDirPath()
{
    QDir dataDir(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());
    dataDir.mkpath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());

    return dataDir.absolutePath();
}

void AppGui::setupLanguage()
{
    QString locale;
    {
        QSettings settings;
        QString lang = settings.value("settings/lang").toString();
        if (lang != "")
        {
            //set language from config
            locale = lang;
        }
        else
        {
            //set default system language
            locale = QLocale::system().name().section('_', 0, 0);
            qDebug() << "System locale: " << QLocale::system();
        }
    }

    delete translator;
    translator = new QTranslator(this);

    //Set language
    QString langfile = QString(":/lang/mc_%1.qm").arg(locale);
    qInfo() << "Trying to set language: " << langfile;
    if (QFile::exists(langfile))
    {
        translator->load(langfile);
        if (!installTranslator(translator))
            qCritical() << "Failed to install " << langfile;
        qDebug() << "Translator installed";
    }
}

void AppGui::createMainWindow()
{
    if (win) return;

    qtAwesome()->initFontAwesome();

    Q_ASSERT(dbMasterController);

    win = new MainWindow(wsClient, dbMasterController);
    connect(win, &MainWindow::destroyed, [this](QObject *)
    {
        win = nullptr;
    });
    connect(win, &MainWindow::windowCloseRequested, [=]()
    {
        mainWindowHide();
    });
    connect(win, &MainWindow::iconChangeRequested, [this]()
    {
        QSettings s;
        QString iconConnection = wsClient->get_connected() ? "" : "_disconnected";
        QIcon icon(":/systray" + iconConnection + s.value("settings/systray_icon").toString() + ".png");
        systray->setIcon(icon);
    });

    dbMasterController->setMainWindow(win);
}
