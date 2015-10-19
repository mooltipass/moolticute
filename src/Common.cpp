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

static QFile debugLogFile;
static void _messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString fname = context.file;
    fname = fname.section('\\', -1, -1);

    switch (type) {
    default:
    case QtDebugMsg:
    {
        QString s = QString("DEBUG: %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
        printf("%s", qPrintable(s));
        if (debugLogFile.isOpen()) debugLogFile.write(s.toLocal8Bit());
        break;
    }
    case QtWarningMsg:
    {
        QString s = QString("WARNING: %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
        printf("%s", qPrintable(s));
        if (debugLogFile.isOpen()) debugLogFile.write(s.toLocal8Bit());
        break;
    }
    case QtCriticalMsg:
    {
        QString s = QString("CRITICAL: %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
        printf("%s", qPrintable(s));
        if (debugLogFile.isOpen()) debugLogFile.write(s.toLocal8Bit());
        break;
    }
    case QtFatalMsg:
    {
        QString s = QString("FATAL: %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
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
