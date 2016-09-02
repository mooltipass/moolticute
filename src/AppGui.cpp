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

    showConfigApp = new QAction(tr("&Hide Moolticute configurator"), this);
    connect(showConfigApp, &QAction::triggered, [=](){
        if (win.isHidden())
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


}

AppGui::~AppGui()
{
    delete systray;
}

void AppGui::mainWindowShow()
{
    win.show();
    showConfigApp->setText(tr("&Hide Moolticute configurator"));
}

void AppGui::mainWindowHide()
{
    win.hide();
    showConfigApp->setText(tr("&Show Moolticute configurator"));
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
