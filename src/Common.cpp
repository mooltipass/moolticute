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
#include "Common.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <time.h>
#include "version.h"
#include <chrono>

#ifndef Q_OS_WIN
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#else
#include <qt_windows.h>
#endif

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
    const auto it = std::find_if(std::begin(MPStatusString), std::end(MPStatusString),
                                 [&st](const QString& val){ return val == st;});
    if (it != std::end(MPStatusString))
    {
        return it.key();
    }

    qWarning() << "Unable to find value from enum for status" << st;
    return Common::UnknownStatus;
}

static QLocalServer *debugLogServer = nullptr;
static QList<QLocalSocket *> debugLogClients;
static Common::GuiLogCallback guiLogCallback = [](const QByteArray &) {};
static void _messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString fname = context.file;
    fname = fname.section('\\', -1, -1);

    QString s;
    switch (type) {
        case QtDebugMsg:
        {
            s = QString(COLOR_CYAN "DEBUG" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
            break;
        }
        case QtInfoMsg:
        {
            s = QString(COLOR_GREEN "INFO" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
            break;
        }
        case QtWarningMsg:
        {
            s = QString(COLOR_YELLOW "WARNING" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
            break;
        }
        case QtCriticalMsg:
        {
            s = QString(COLOR_ORANGE "CRITICAL" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
            break;
        }
        case QtFatalMsg:
        {
            s = QString(COLOR_RED "FATAL" COLOR_RESET ": %1:%2 - %3\n").arg(fname).arg(context.line).arg(msg);
            break;
        }
    }

    if (!s.isEmpty())
    {
        printf("%s", qPrintable(s));
        fflush(stdout);

        //In release, do not display qDebug messages from GUI
        if (QStringLiteral(APP_VERSION) == "git" ||
            type == QtFatalMsg ||
            type == QtCriticalMsg ||
            type == QtWarningMsg ||
            type == QtInfoMsg)
            guiLogCallback(s.toUtf8());

        for (QLocalSocket *sock: debugLogClients)
        {
            sock->write(s.toUtf8());
            sock->flush();
        }
    }
}

void Common::installMessageOutputHandler(QLocalServer *logServer, GuiLogCallback guicb)
{
    debugLogServer = logServer;
    if (debugLogServer)
    {
        QObject::connect(debugLogServer, &QLocalServer::newConnection, []()
        {
            if (!debugLogServer->hasPendingConnections())
                return;

            QLocalSocket *s = debugLogServer->nextPendingConnection();

            //New clients gets added to the list
            //and logs will be forwarded to them
            debugLogClients.append(s);

            QObject::connect(s, &QLocalSocket::disconnected, [s]()
            {
                debugLogClients.removeAll(s);
                s->deleteLater();
            });
        });
    }
    guiLogCallback = guicb;
    qInstallMessageHandler(_messageOutput);
}

QDate Common::bytesToDate(const QByteArray &data)
{
    //reminder date is uint16_t : yyyy yyym mmmd dddd with year from 2010
    int y = ((static_cast<quint8>(data[0]) >> 1) & 0x7F) + 2010;
    int m = (static_cast<quint8>(data[0] & 0x01) << 3) | ((static_cast<quint8>(data[1]) >> 5) & 0x07);
    int d = (static_cast<quint8>(data[1]) & 0x1F);

    return QDate(y, m+1, d);
}

QByteArray Common::dateToBytes(const QDate &dt)
{
    // reminder Qt date use 1-12 month
    QByteArray data;
    data.resize(2);

    data[0] = static_cast<char>(((dt.year() - 2010) << 1) & 0xFE);
    if(dt.month() - 1  >= 8)
    {
        data[0] = static_cast<char>((static_cast<quint8>(data[0]) | 0x01));
    }
    data[1] = static_cast<char>((((dt.month() -1) % 8) << 5) & 0xE0);
    data[1] = static_cast<char>(static_cast<quint8>(data[1])|dt.day());

    return data;
}

QJsonArray Common::bytesToJson(const QByteArray &data)
{
    QJsonArray arr;
    for (int i = 0;i < data.size();i++)
    {
        arr.append(static_cast<quint8>(data.at(i)));
    }
    return arr;
}

QJsonObject Common::bytesToJsonObjectArray(const QByteArray &data)
{
    QJsonObject returnObject;
    for (qint32 i = 0; i < data.size(); i++)
    {
        returnObject[QString::number(i)] = QJsonValue(static_cast<quint8>(data[i]));
    }
    return returnObject;
}

//Check if the process with <pid> is running
bool Common::isProcessRunning(qint64 pid)
{
    if (pid == 0) return false;
#if defined(Q_OS_WIN)
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (!process) return false;
    DWORD ret = WaitForSingleObject(process, 0);
    CloseHandle(process);
    return (ret == WAIT_TIMEOUT);
#else
    /* This code is portable on linux and macos (not tested on *BSD) */

    pid_t p = (pid_t)pid;

    //Wait for defunct process to end
    while(waitpid(-1, 0, WNOHANG) > 0)
    { /* wait for defunct... */ }

    if (kill(p, 0) == 0)
        return true;

    //For some process we do not have permission to send signal
    //But this also means that the process exists
    if (errno == EPERM)
        return true;

    return false;
#endif
}

QJsonObject Common::readSharedMemory(QSharedMemory &sh)
{
    QJsonObject o;

    if (!sh.lock())
    {
        qCritical() << "Unable to lock access to shared mem segment: " << sh.errorString();
        return o;
    }

    QJsonParseError jerr;
    QJsonDocument jdoc = QJsonDocument::fromJson(static_cast<const char*>(sh.constData()), &jerr);

    if (jerr.error != QJsonParseError::NoError)
    {
        qCritical() << "Unable to parse shared mem JSON: " << jerr.errorString();
        sh.detach();
        sh.unlock();
        return o;
    }

    o = jdoc.object();

    sh.unlock();

    return o;
}

bool Common::writeSharedMemory(QSharedMemory &sh, const QJsonObject &o)
{
    if (!sh.lock())
    {
        qCritical() << "Unable to lock access to shared mem segment: " << sh.errorString();
        return false;
    }

    QJsonDocument jdoc(o);
    QByteArray ba = jdoc.toJson(QJsonDocument::Compact);
    memcpy(sh.data(), ba.constData(), static_cast<size_t>(ba.size()));

    sh.unlock();

    return true;
}

static bool commonUidInit = false;
static QHash<QString, bool> commonExistingUid;

QString Common::createUid(QString prefix)
{
    if (!commonUidInit)
    {
        commonUidInit = true;
        qsrand(static_cast<uint>(time(nullptr)));
    }

    //try to generate a unique id based on
    QString s;
    do
    {
        s = QStringLiteral("%1%2%3%4%5%6")
            .arg(prefix)
            .arg(qrand())
            .arg(qrand())
            .arg(qrand())
            .arg(qrand())
            .arg(qrand());
    } while (commonExistingUid.contains(s));

    return s;
}

void Common::releaseUid(QString uid)
{
    //delete this id from the list so it could be used again
    commonExistingUid.remove(uid);
}

QString Common::maskLog(const QString &rawJson)
{
    QString logMsg = rawJson;
    logMsg.replace(QRegularExpression("\"password\"\\s*:\\s*\"[^\"-\"]+\""), "\"password\":\"<masked>\"");
    logMsg.replace(QRegularExpression("\"file_data\"\\s*:\\s*\"[^\"-\"]+\""), "\"file_data\":\"<base64_data>\"");
    logMsg.replace(QRegularExpression("\"node_data\"\\s*:\\s*\"[^\"-\"]+\""), "\"node_data\":\"<base64_data>\"");
    return logMsg;
}

static std::vector<qint64> mpRngIntegers;
static std::random_device rngDevice;

std::vector<qint64> Common::getRngSeed()
{
    if (mpRngIntegers.empty())
    {
        std::vector<qint64> ints = {rngDevice(), rngDevice(), rngDevice(), rngDevice(),
                                 rngDevice(), rngDevice(), rngDevice(), rngDevice(),
                                 std::chrono::system_clock::now().time_since_epoch().count()};
        return ints;
    }
    else
    {
        std::vector<qint64> ints = mpRngIntegers;
        ints.push_back(rngDevice());
        ints.push_back(std::chrono::system_clock::now().time_since_epoch().count());
        return ints;
    }
}

void Common::updateSeed(std::vector<qint64> &newInts)
{
    mpRngIntegers = newInts;
}
