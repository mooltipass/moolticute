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
#ifndef COMMON_H
#define COMMON_H

#include <QtCore>
#include <functional>
#include <memory>
#include <type_traits>

#define MOOLTIPASS_VENDORID     0x16D0
#define MOOLTIPASS_PRODUCTID    0x09A0
#define MOOLTIPASS_USAGE_PAGE   0xFF31
#define MOOLTIPASS_USAGE        0x74

#define MOOLTIPASS_FAV_MAX       14
#define MP_NODE_SIZE            132

#define MOOLTIPASS_BLOCK_SIZE   32

#define MOOLTICUTE_DAEMON_PORT  30035

//shared memory size
#define SHMEM_SIZE      128 * 1024

#define qToChar(s) s.toLocal8Bit().constData()
#define qToUtf8(s) s.toUtf8().constData()

#define MOOLTICUTE_DAEMON_LOG_SOCK    "moolticuted_local_log_sock"

class QLocalServer;

#if QT_VERSION < 0x050700
//This declares a qAsConst implementation if we build with Qt < 5.7

template< class T >
using mc_remove_reference_t = typename std::remove_reference<T>::type;
template<class T>
mc_remove_reference_t<T> const& qAsConst(T&&t){return t;}
template<class T>
T const qAsConst(T&&t){return std::forward<T>(t);}
template<class T>
T const& qAsConst(T&t){return t;}
#endif

class Common
{
public:
    static void installMessageOutputHandler(QLocalServer *logServer = nullptr);

    Q_ENUMS(MPStatus)
    typedef enum
    {
        UnknownStatus = -1,
        NoCardInserted = 0,
        Locked = 1,
        Error2 = 2,
        LockedScreen = 3,
        Error4 = 4,
        Unlocked = 5,
        Error6 = 6,
        Error7 = 7,
        Error8 = 8,
        UnkownSmartcad = 9,
    } MPStatus;
    static QHash<MPStatus, QString> MPStatusUserString, MPStatusString;

    static Common::MPStatus statusFromString(const QString &st);

    static QDate bytesToDate(const QByteArray &d);
    static QByteArray dateToBytes(const QDate &dt);

    static QJsonArray bytesToJson(const QByteArray &data);

    static bool isProcessRunning(qint64 pid);

    static QJsonObject readSharedMemory(QSharedMemory &sh);
    static bool writeSharedMemory(QSharedMemory &sh, const QJsonObject &o);

    //create a unique identifier and save it into a list
    static QString createUid(QString prefix = QString());
    //release the unique id and remove it from the list
    static void releaseUid(QString uid);

    typedef enum
    {
        MP_Classic = 0,
        MP_Mini = 1,
    } MPHwVersion;
};

Q_DECLARE_METATYPE(Common::MPStatus)
Q_DECLARE_METATYPE(Common::MPHwVersion)

#endif // COMMON_H

