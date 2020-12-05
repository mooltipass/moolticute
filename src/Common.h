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
#include <random>

#define MOOLTIPASS_VENDORID     0x16D0
#define MOOLTIPASS_PRODUCTID    0x09A0
#define MOOLTIPASS_USAGE_PAGE   0xFF31
#define MOOLTIPASS_USAGE        0x74

#define MOOLTIPASS_BLE_VENDORID   0x1209
#define MOOLTIPASS_BLE_PRODUCTID  0x4321

#define MOOLTIPASS_USBHID_DESC_SIZE 28
#define MOOLTIPASS_BLEHID_DESC_SIZE 45

#define MOOLTIPASS_FAV_MAX      14
#define MOOLTIPASS_ADDRESS_SIZE 4
#define MP_NODE_SIZE            132
#define MP_NODE_DATA_ENC_SIZE   128

#define MAX_BLE_CAT_NUM 10

#define MOOLTIPASS_BLOCK_SIZE   32
#define MOOLTIPASS_DESC_SIZE    23 //max size allowed for description

#define CMD_MAX_RETRY            3 //retry sending command before dropping
#define CMD_DEFAULT_TIMEOUT     0xFFAAFFAA
#define CMD_DEFAULT_TIMEOUT_VAL 3000 //3seconds default timeout

//Data node header size. It contains the size of data in 4 bytes Big endian
#define MP_DATA_HEADER_SIZE      4

#define MOOLTICUTE_DAEMON_PORT  30035

//Max file size for standard data file
#define MP_MAX_FILE_SIZE        1024 * 10
#define MP_MAX_SSH_SIZE         1024 * 50

//SSH Service name should be lowercase
#define MC_SSH_SERVICE          "moolticute ssh keys"

//shared memory size
#define SHMEM_SIZE              128 * 1024

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
#define ID_KEYB_CA_FR_LUT       BITMAP_ID_OFFSET+45
#define ID_KEYB_CA_MAC_LUT      BITMAP_ID_OFFSET+46
#define ID_KEYB_DK_MAC_LUT      BITMAP_ID_OFFSET+47
#define ID_KEYB_UK_MAC_LUT      BITMAP_ID_OFFSET+48
#define ID_KEYB_POR_LUT         BITMAP_ID_OFFSET+49
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

//This declares a QOverload implementation if we build with Qt < 5.7
template <typename... Args>
struct QNonConstOverload
{
    template <typename R, typename T>
    Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...)) const noexcept -> decltype(ptr)
    { return ptr; }
    template <typename R, typename T>
    static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...)) noexcept -> decltype(ptr)
    { return ptr; }
};
template <typename... Args>
struct QConstOverload
{
    template <typename R, typename T>
    Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...) const) const noexcept -> decltype(ptr)
    { return ptr; }
    template <typename R, typename T>
    static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...) const) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args>
struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...>
{
    using QConstOverload<Args...>::of;
    using QConstOverload<Args...>::operator();
    using QNonConstOverload<Args...>::of;
    using QNonConstOverload<Args...>::operator();
    template <typename R>
    Q_DECL_CONSTEXPR auto operator()(R (*ptr)(Args...)) const noexcept -> decltype(ptr)
    { return ptr; }
    template <typename R>
    static Q_DECL_CONSTEXPR auto of(R (*ptr)(Args...)) noexcept -> decltype(ptr)
    { return ptr; }
};

/*
 * ENDIAN FUNCTIONS
 */

// Used to implement a type-safe and alignment-safe copy operation
// If you want to avoid the memcpy, you must write specializations for these functions
template <typename T> inline void qToUnaligned(const T src, void *dest)
{
    // Using sizeof(T) inside memcpy function produces internal compiler error with
    // MSVC2008/ARM in tst_endian -> use extra indirection to resolve size of T.
    const size_t size = sizeof(T);
    memcpy(dest, &src, size);
}

template <typename T> inline void qToLittleEndian(T src, void *dest)
{ qToUnaligned<T>(src, dest); }

#endif

//Q_DISABLE_COPY_MOVE from Qt 5.15
#define DISABLE_COPY_MOVE(Class) \
    Q_DISABLE_COPY(Class) \
    Class(const Class &&) Q_DECL_EQ_DELETE;\
    Class &operator=(const Class &&) Q_DECL_EQ_DELETE;

template<typename Container>
void clearAndDelete(Container& cont)
{
    qDeleteAll(cont);
    cont.clear();
}

class Common
{
public:
    typedef std::function<void(const QByteArray &)> GuiLogCallback;

    static void setIsDaemon(bool en);
    static bool isDaemon();
    static void installMessageOutputHandler(QLocalServer *logServer = nullptr, GuiLogCallback guicb = [](const QByteArray &){});

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
        UnknownSmartcard = 9,
        MMMMode = 21
    } MPStatus;
    static QHash<MPStatus, QString> MPStatusUserString, MPStatusString;
    static QMap<int, QString> BLE_CATEGORY_COLOR;

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

    //mask log by removing passwords and data from log
    static QString maskLog(const QString &rawJson);

    static std::vector<qint64> getRngSeed();
    static void updateSeed(std::vector<qint64> &newInts);

    static void fill(QByteArray &ba, int count, char c);

    static QByteArray toHexArray(const QString str);
    static QString toHexString(const QByteArray& array);

    typedef enum
    {
        MP_Classic = 0,
        MP_Mini = 1,
        MP_BLE = 2,
        MP_Unknown = -1
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

    enum class FetchState
    {
        STOPPED,
        STARTED
    };

    enum class FetchType
    {
        ACCELEROMETER,
        RANDOM_BYTES
    };

    enum ChangeNumberFlag : quint8
    {
        CredentialNumberChanged = 0x01,
        DataNumberChanged = 0x02
    };

    enum AddressType
    {
        CRED_ADDR_IDX = 0,
        WEBAUTHN_ADDR_IDX = 1
    };

    static const QString ISODateWithMsFormat;
    static const QString SIMPLE_CRYPT;
    static const QString SIMPLE_CRYPT_V2;
    static const int DEFAULT_PASSWORD_LENGTH = 16;
};

enum class DeviceType
{
    MOOLTIPASS = 0,
    MINI = 1,
    BLE = 2
};

const QRegularExpression regVersion("v([0-9]+)\\.([0-9]+)(.*)");

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
                        "}" \
                        "QPushButton:!enabled {" \
                            "background-color: #cee8ef;" \
                        "}"

#define CSS_GREY_BUTTON "QPushButton {" \
                            "color: #000000;" \
                            "background-color: #D3D3D3;" \
                            "padding: 12px;" \
                            "border: none;" \
                        "}" \
                        "QPushButton:hover {" \
                            "background-color: #DCDCDC;" \
                        "}" \
                        "QPushButton:pressed {" \
                            "color: #FFFFFF;" \
                            "background-color: #237C95;" \
                        "}"

#define CSS_CREDVIEW_HEADER "QHeaderView::section {" \
                                "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,"\
                                        "stop:0 #F1F1F1, stop: 0.5 #E0E0E0,"\
                                        "stop: 0.6 #D3D3D3, stop:1 #F5F5F5);"\
                                "color: black;"\
                                "padding-left: 4px;"\
                                "border: 1px solid #FcFcFc;"\
                            "}"

#define CSS_CREDVIEW_SELECTION "QTreeView::item:selected {" \
                                    "background-color: rgba( 128, 128, 128, 100);" \
                               "}"

enum class CloseBehavior
{
    CloseApp,
    HideWindow
};

#endif // COMMON_H

