#include "AppGui.h"

AppGui::AppGui(int & argc, char ** argv) :
    QApplication(argc, argv)
{
    QCoreApplication::setOrganizationName("Raoulh");
    QCoreApplication::setOrganizationDomain("raoulh.org");
    QCoreApplication::setApplicationName("Moolticute");

    Common::installMessageOutputHandler();

    systray = new QSystemTrayIcon(this);
    //    sys->setIcon(QIcon("C:/Users/phyatt/Downloads/system-tray.png"));
    systray->setIcon(QIcon(":/systray.png"));
    systray->show();

    QAction* launchConfigApp = new QAction(tr("&Launch Mooltipass configurator"), this);
    connect(launchConfigApp, &QAction::triggered, [=](){
        QString dir = QCoreApplication::applicationDirPath();
    });

    QAction* maximizeAction = new QAction(tr("Ma&ximize"), this);
    //connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    QAction* restoreAction = new QAction(tr("&Restore"), this);
    //connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    QAction* quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);


    QMenu * systrayMenu = new QMenu();
    //systrayMenu->addAction(minimizeAction);
    systrayMenu->addAction(maximizeAction);
    systrayMenu->addAction(restoreAction);
    systrayMenu->addSeparator();
    systrayMenu->addAction(quitAction);

    systray->setContextMenu(systrayMenu);


}

AppGui::~AppGui()
{
    delete systray;
}

void AppGui::mainWindowShow()
{
    win.show();
}

void AppGui::mainWindowHide()
{
    win.hide();
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
