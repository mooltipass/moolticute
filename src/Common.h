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

#define MOOLTIPASS_VENDORID     0x16D0
#define MOOLTIPASS_PRODUCTID    0x09A0
#define MOOLTIPASS_USAGE_PAGE   0xFF31
#define MOOLTIPASS_USAGE        0x74

#define MOOLTICUTE_DAEMON_PORT  30035

#define qToChar(s) s.toLocal8Bit().constData()
#define qToUtf8(s) s.toUtf8().constData()

class Common
{
public:
    static void installMessageOutputHandler();

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
    static QHash<MPStatus, QString> MPStatusString;
};

Q_DECLARE_METATYPE(Common::MPStatus)

#endif // COMMON_H

