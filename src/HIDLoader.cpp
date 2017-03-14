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
#include "HIDLoader.h"

#define DEFINE_FN(fnc) \
    static _##fnc func_##fnc = nullptr

DEFINE_FN(HidD_GetAttributes);
DEFINE_FN(HidP_GetCaps);
DEFINE_FN(HidD_GetPreparsedData);
DEFINE_FN(HidD_FreePreparsedData);

bool HIDLoader::isLoaded()
{
    if (lib.isNull())
        return load();
    return lib->isLoaded();
}

bool HIDLoader::load()
{
    if (lib.isNull())
        lib.reset(new QLibrary("hid"));
    if (lib->isLoaded())
        return true;

    if (!lib->load())
    {
        qWarning() << "Failed loading hid.dll: " << lib->errorString();
        return false;
    }

#define RESOLVE_FN(fnc) \
    func_##fnc = (_##fnc)lib->resolve(#fnc); \
    if (!func_##fnc) qWarning() << "Unable to resolve function "#fnc;

    RESOLVE_FN(HidD_GetAttributes);
    RESOLVE_FN(HidP_GetCaps);
    RESOLVE_FN(HidD_GetPreparsedData);
    RESOLVE_FN(HidD_FreePreparsedData);

    return true;
}

#define IMPLEMENT_FN_RET(fnc, ret, fmt, ...) \
    if (!func_##fnc) { qWarning() << "Function "#fnc" not resolved in library"; return ret; } \
    return func_##fnc(fmt, ##__VA_ARGS__);

BOOLEAN HIDLoader::HidD_GetAttributes(HANDLE device, PHIDD_ATTRIBUTES attrib)
{
    IMPLEMENT_FN_RET(HidD_GetAttributes, FALSE, device, attrib);
}

NTSTATUS HIDLoader::HidP_GetCaps(PHIDP_PREPARSED_DATA preparsed_data, HIDP_CAPS *caps)
{
    IMPLEMENT_FN_RET(HidP_GetCaps, 0, preparsed_data, caps);
}

BOOLEAN HIDLoader::HidD_GetPreparsedData(HANDLE handle, PHIDP_PREPARSED_DATA *preparsed_data)
{
    IMPLEMENT_FN_RET(HidD_GetPreparsedData, FALSE, handle, preparsed_data);
}

BOOLEAN HIDLoader::HidD_FreePreparsedData(PHIDP_PREPARSED_DATA preparsed_data)
{
    IMPLEMENT_FN_RET(HidD_FreePreparsedData, FALSE, preparsed_data);
}
