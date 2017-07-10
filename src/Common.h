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

#define MOOLTIPASS_FAV_MAX      14
#define MOOLTIPASS_ADDRESS_SIZE 4
#define MP_NODE_SIZE            132

#define MOOLTIPASS_BLOCK_SIZE   32
#define MOOLTIPASS_DESC_SIZE    23 //max size allowed for description

#define CMD_MAX_RETRY            3 //retry sending command before dropping
#define CMD_DEFAULT_TIMEOUT     0xFFAAFFAA
#define CMD_DEFAULT_TIMEOUT_VAL 3000 //3seconds default timeout

//Data node header size. It contains the size of data in 4 bytes Big endian
#define MP_DATA_HEADER_SIZE      4

#define MOOLTICUTE_DAEMON_PORT  30035

//shared memory size
#define SHMEM_SIZE      128 * 1024

#define qToChar(s) s.toLocal8Bit().constData()
#define qToUtf8(s) s.toUtf8().constData()

#define MOOLTICUTE_DAEMON_LOG_SOCK    "moolticuted_local_log_sock"

#define BITMAP_ID_OFFSET        128
#define ID_KEYB_EN_US_LUT       BITMAP_ID_OFFSET+18
#define ID_KEYB_FR_FR_LUT       BITMAP_ID_OFFSET+19
#define ID_KEYB_ES_ES_LUT       BITMAP_ID_OFFSET+20
#define ID_KEYB_DE_DE_LUT       BITMAP_ID_OFFSET+21
#define ID_KEYB_ES_AR_LUT       BITMAP_ID_OFFSET+22
#define ID_KEYB_EN_AU_LUT       BITMAP_ID_OFFSET+23
#define ID_KEYB_FR_BE_LUT       BITMAP_ID_OFFSET+24
#define ID_KEYB_PO_BR_LUT       BITMAP_ID_OFFSET+25
#define ID_KEYB_EN_CA_LUT       BITMAP_ID_OFFSET+26
#define ID_KEYB_CZ_CZ_LUT       BITMAP_ID_OFFSET+27
#define ID_KEYB_DA_DK_LUT       BITMAP_ID_OFFSET+28
#define ID_KEYB_FI_FI_LUT       BITMAP_ID_OFFSET+29
#define ID_KEYB_HU_HU_LUT       BITMAP_ID_OFFSET+30
#define ID_KEYB_IS_IS_LUT       BITMAP_ID_OFFSET+31
#define ID_KEYB_IT_IT_LUT       BITMAP_ID_OFFSET+32  // So it seems Italian keyboards don't have ~`
#define ID_KEYB_NL_NL_LUT       BITMAP_ID_OFFSET+33
#define ID_KEYB_NO_NO_LUT       BITMAP_ID_OFFSET+34
#define ID_KEYB_PO_PO_LUT       BITMAP_ID_OFFSET+35  // Polish programmer keyboard
#define ID_KEYB_RO_RO_LUT       BITMAP_ID_OFFSET+36
#define ID_KEYB_SL_SL_LUT       BITMAP_ID_OFFSET+37
#define ID_KEYB_FRDE_CH_LUT     BITMAP_ID_OFFSET+38
#define ID_KEYB_EN_UK_LUT       BITMAP_ID_OFFSET+39
#define ID_KEYB_CZ_QWERTY_LUT   BITMAP_ID_OFFSET+51
#define ID_KEYB_EN_DV_LUT       BITMAP_ID_OFFSET+52
#define ID_KEYB_FR_MAC_LUT      BITMAP_ID_OFFSET+53
#define ID_KEYB_FR_CH_MAC_LUT   BITMAP_ID_OFFSET+54
#define ID_KEYB_DE_CH_MAC_LUT   BITMAP_ID_OFFSET+55
#define ID_KEYB_DE_MAC_LUT      BITMAP_ID_OFFSET+56
#define ID_KEYB_US_MAC_LUT      BITMAP_ID_OFFSET+57

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
    static QJsonObject bytesToJsonObjectArray(const QByteArray &data);

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


    enum class LockUnlockModeFeatureFlags : uint {
        Disabled          = 0,
        Password          = 0x01,
        SendEnter         = 0x02,
        Login             = 0x04,
        SendWin_L         = 0x08,
        SendCtrl_Alt_Del  = 0x10
    };

    //Credentials favorites
    enum FavoriteFlags {
        FAV_NOT_SET = -1,
        FAV_1,
        FAV_2,
        FAV_3,
        FAV_4,
        FAV_5,
        FAV_6,
        FAV_7,
        FAV_8,
        FAV_9,
        FAV_10,
        FAV_11,
        FAV_12,
        FAV_13,
        FAV_14,
    };
};

Q_DECLARE_METATYPE(Common::MPStatus)
Q_DECLARE_METATYPE(Common::MPHwVersion)

#define CSS_BLUE_BUTTON "QPushButton {" \
                            "color: #fff;" \
                            "background-color: #60B1C7;" \
                            "padding: 12px;" \
                            "border: none;" \
                        "}" \
                        "QPushButton:hover {" \
                            "background-color: #3d96af;" \
                        "}" \
                        "QPushButton:pressed {" \
                            "background-color: #237C95;" \
                        "}"

#define CSS_GREY_BUTTON "QPushButton {" \
                            "color: #888888;" \
                            "background-color: #E7E7E7;" \
                            "padding: 12px;" \
                            "border: none;" \
                        "}" \
                        "QPushButton:hover {" \
                            "background-color: #DCDCDC;" \
                        "}" \
                        "QPushButton:pressed {" \
                            "color: #FFFFFF;" \
                            "background-color: #E57373;" \
                        "}"

#endif // COMMON_H

