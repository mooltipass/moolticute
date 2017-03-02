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
#ifndef HID_DLL_H
#define HID_DLL_H

/* This header contains definitions from hid.h from microsoft
 * It's declared here for loading at runtime
 */

#include <qt_windows.h>
#include <hidsdi.h>
#include <hidpi.h>

typedef BOOLEAN (__stdcall *_HidD_GetAttributes)(HANDLE device, PHIDD_ATTRIBUTES attrib);
typedef NTSTATUS (__stdcall *_HidP_GetCaps)(PHIDP_PREPARSED_DATA preparsed_data, HIDP_CAPS *caps);
typedef BOOLEAN (__stdcall *_HidD_GetPreparsedData)(HANDLE handle, PHIDP_PREPARSED_DATA *preparsed_data);
typedef BOOLEAN (__stdcall *_HidD_FreePreparsedData)(PHIDP_PREPARSED_DATA preparsed_data);

#endif // HID_DLL_H
