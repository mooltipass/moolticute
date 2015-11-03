/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Calaos is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Calaos is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#include "Common.h"

#ifdef Q_OS_WIN_DISABLE_FOR_NOW
#define COLOR_LIGHTRED
#define COLOR_RED
#define COLOR_LIGHTBLUE
#define COLOR_BLUE
#define COLOR_GREEN
#define COLOR_YELLOW
#define COLOR_ORANGE
#define COLOR_WHITE
#define COLOR_LIGHTCYAN
#define COLOR_CYAN
#define COLOR_RESET
#define COLOR_HIGH
#else
#define COLOR_LIGHTRED  "\033[31;1m"
#define COLOR_RED       "\033[31m"
#define COLOR_LIGHTBLUE "\033[34;1m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_GREEN     "\033[32;1m"
#define COLOR_YELLOW    "\033[33;1m"
#define COLOR_ORANGE    "\033[0;33m"
#define COLOR_WHITE     "\033[37;1m"
#define COLOR_LIGHTCYAN "\033[36;1m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_RESET     "\033[0m"
#define COLOR_HIGH      "\033[1m"
#endif

QHash<Common::MPStatus, QString> Common::MPStatusUserString = {
    { Common::UnknownStatus, QObject::tr("Unknown status") },
    { Common::NoCardInserted, QObject::tr("No card inserted") },
    { Common::Locked, QObject::tr("Mooltipass locked") },
    { Common::Error2, QObject::tr("Error 2 (should not happen)") },
    { Common::LockedScreen, QObject::tr("Mooltipass locked, unlocking screen") },
    { Common::Error4, QObject::tr("Error 4 (should not happen)") },
    { Common::Unlocked, QObject::tr("Mooltipass unlocked") },
    { Common::Error6, QObject::tr("Error 6 (should not happen)") },
    { Common::Error7, QObject::tr("Error 7 (should not happen)") },
    { Common::Error8, QObject::tr("Error 8 (should not happen)") },
    { Common::UnkownSmartcad, QObject::tr("Unknown smartcard inserted") }
};

QHash<Common::MPStatus, QString> Common::MPStatusString = {
    { Common::UnknownStatus, "UnknownStatus" },
    { Common::NoCardInserted, "NoCardInserted" },
    { Common::Locked, "Locked" },
    { Common::Error2, "Error2" },
    { Common::LockedScreen, "LockedScreen" },
    { Common::Error4, "Error4" },
    { Common::Unlocked, "Unlocked" },
    { Common::Error6, "Error6" },
    { Common::Error7, "Error7" },
    { Common::Error8, "Error8" },
    { Common::UnkownSmartcad, "UnkownSmartcad" }
};

Common::MPStatus Common::statusFromString(const QString &st)
{
    for (auto it = MPStatusString.begin();it != MPStatusString.end();it++)
    {
        if (it.value() == st)
            return it.key();
    }

    return Common::UnknownStatus;
}

static QFile debugLogFile;
static void _messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString fname = context.file;
    fname = fname.section('\\', -1, -1);

    switch (type) {
    default:
    case QtDebugMsg:
    {
        QString s = QString(COLOR_CYAN "DEBUG" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
        printf("%s", qPrintable(s));
        if (debugLogFile.isOpen()) debugLogFile.write(s.toLocal8Bit());
        break;
    }
    case QtInfoMsg:
    {
        QString s = QString(COLOR_GREEN "INFO" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
        printf("%s", qPrintable(s));
        if (debugLogFile.isOpen()) debugLogFile.write(s.toLocal8Bit());
        break;
    }
    case QtWarningMsg:
    {
        QString s = QString(COLOR_YELLOW "WARNING" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
        printf("%s", qPrintable(s));
        if (debugLogFile.isOpen()) debugLogFile.write(s.toLocal8Bit());
        break;
    }
    case QtCriticalMsg:
    {
        QString s = QString(COLOR_ORANGE "CRITICAL" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
        printf("%s", qPrintable(s));
        if (debugLogFile.isOpen()) debugLogFile.write(s.toLocal8Bit());
        break;
    }
    case QtFatalMsg:
    {
        QString s = QString(COLOR_RED "FATAL" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
        printf("%s", qPrintable(s));
        if (debugLogFile.isOpen()) debugLogFile.write(s.toLocal8Bit());
        break;
    }
    }

    if (debugLogFile.isOpen())
        debugLogFile.flush();
    fflush(stdout);
}

void Common::installMessageOutputHandler()
{
    if (!debugLogFile.isOpen())
    {
        //simple logrotate
        QString f = QCoreApplication::applicationDirPath() + "/app.log";
        QFileInfo fi(f);
        if (fi.size() > 10 * 1024 * 1024) //10Mb max
        {
            QFile::remove(f + ".5"); //remove really old
            QFile::rename(f + ".4", f + ".5");
            QFile::rename(f + ".3", f + ".4");
            QFile::rename(f + ".2", f + ".3");
            QFile::rename(f, f + ".2");
        }

        debugLogFile.setFileName(f);
        //debugLogFile.open(QFile::ReadWrite | QFile::Append);
        //debugLogFile.write(QString(DEBUG_START_LINE).arg(QDateTime::currentDateTime().toString()).toLocal8Bit());
    }
    qInstallMessageHandler(_messageOutput);
}

QDate Common::bytesToDate(const QByteArray &data)
{
    int y = (((quint8)data[0] >> 1) & 0x7F) + 2010;
    int m = (((quint8)data[0] & 0x01) << 3) | (((quint8)data[0] >> 5) & 0x07);
    int d = ((quint8)data[0] & 0x1F);

    return QDate(y, m, d);
}

QByteArray Common::dateToBytes(const QDate &dt)
{
    QByteArray data;
    data.resize(2);

    data[0] = (quint8)(((dt.year() - 2010) << 1) & 0xFE);
    if(dt.month() >= 8)
        data[0] = (quint8)((quint8)data[0] | 0x01);
    data[1] = (quint8)(((dt.month() % 8) << 5) & 0xE0);
    data[1] = (quint8)((quint8)data[1] | dt.day());

    return data;
}

QJsonArray Common::bytesToJson(const QByteArray &data)
{
    QJsonArray arr;
    for (int i = 0;i < data.size();i++)
        arr.append((quint8)data.at(i));
    return arr;
}
