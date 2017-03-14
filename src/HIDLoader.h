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
#ifndef HIDLOADER_H
#define HIDLOADER_H

#include <QtCore>
#include "hid_dll.h"

/*
 * Helper class to load hid.dll dynamically and provide the functions
 * It does all the checks and does not crash if accessing unloaded dll/func
 */

#define HID    HIDLoader::Instance()

class HIDLoader
{
private:
    QScopedPointer<QLibrary> lib;
    HIDLoader() {}

public:
    static HIDLoader &Instance()
    {
        static HIDLoader inst;
        return inst;
    }

    bool load();
    bool isLoaded();

    BOOLEAN HidD_GetAttributes(HANDLE device, PHIDD_ATTRIBUTES attrib);
    NTSTATUS HidP_GetCaps(PHIDP_PREPARSED_DATA preparsed_data, HIDP_CAPS *caps);
    BOOLEAN HidD_GetPreparsedData(HANDLE handle, PHIDP_PREPARSED_DATA *preparsed_data);
    BOOLEAN HidD_FreePreparsedData(PHIDP_PREPARSED_DATA preparsed_data);
};

#endif // HIDLOADER_H
