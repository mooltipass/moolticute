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
#ifndef MPDEVICE_MAC_H
#define MPDEVICE_MAC_H

#include "MPDevice.h"

class MPPlatformDef
{
public:
    QString id; //unique id for all platform

};

inline bool operator==(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return lhs.id == rhs.id; }
inline bool operator!=(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return !(lhs == rhs); }

class MPDevice_mac: public MPDevice
{
    Q_OBJECT
public:
    MPDevice_mac(QObject *parent, const MPPlatformDef &platformDef);
    virtual ~MPDevice_mac();

    //Static function for enumerating devices on platform
    static QList<MPPlatformDef> enumerateDevices();
};

#endif // MPDEVICE_MAC_H
