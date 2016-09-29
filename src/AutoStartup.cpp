#include "AutoStartup.h"

#include <QtCore>

#ifdef Q_OS_MAC
#include "MacUtils.h"
#endif

void AutoStartup::enableAutoStartup(bool en)
{
#if defined(Q_OS_WIN)
    if (en)
    {
        //Install registry key
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                           QSettings::NativeFormat);
        settings.setValue("Moolticute", QDir::toNativeSeparators(qApp->applicationFilePath()));
        settings.sync();
    }
    else
    {
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                           QSettings::NativeFormat);
        settings.remove("Moolticute");
        settings.sync();
    }
#elif defined(Q_OS_LINUX)
    QString desktopFile = QString("[Desktop Entry]\n"
                                          "Name=Moolticute\n"
                                          "Comment=Mooltipass companion\n"
                                          "Icon=moolticute\n"
                                          "Type=Application\n"
                                          "Exec=%1\n"
                                          "Hidden=false\n"
                                          "NoDisplay=false\n"
                                          "X-GNOME-Autostart-enabled=true\n")
                                  .arg(qApp->applicationFilePath());

    QString autostartLocation;
    char *xgdConfigHome = getenv("XDG_CONFIG_HOME");
    if(xgdConfigHome != NULL)
        autostartLocation = QString(QString(xgdConfigHome) + "/.config/autostart");
    else
        autostartLocation = QString(QDir::homePath() + "/.config/autostart");

    QDir d;
    d.mkpath(autostartLocation);
    QFile f(autostartLocation + "/moolticute.desktop");

    if (en)
    {
        if (f.open(QFile::WriteOnly | QFile::Truncate))
        {
            f.write(desktopFile.toLocal8Bit());
            f.close();
        }
        f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther);
    }
    else
    {
        if (f.exists())
            f.remove();
    }
#elif defined(Q_OS_MAC)
    utils::mac::setAutoStartup(en);
#endif
}
