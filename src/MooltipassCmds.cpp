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
#include "MooltipassCmds.h"
#include <QMetaEnum>
#include <QDebug>

bool MPCmd::isUserRequired(Command c)
{
    return c == GET_LOGIN ||
           c == ADD_CONTEXT ||
           c == SET_LOGIN ||
           c == SET_PASSWORD ||
           c == START_MEMORYMGMT ||
           c == IMPORT_MEDIA_START ||
           c == RESET_CARD ||
           c == READ_CARD_LOGIN ||
           c == SET_CARD_LOGIN ||
           c == SET_CARD_PASS ||
           c == ADD_UNKNOWN_CARD ||
           c == ADD_DATA_SERVICE ||
           c == WRITE_32B_IN_DN ||
           c == GET_DESCRIPTION ||
           c == SET_DESCRIPTION;
}

QString MPCmd::toHexString(Command c)
{
    return QString("0x%1").arg((quint8)c, 2, 16, QChar('0'));
}

QString MPCmd::printCmd(const QByteArray &ba)
{
    if (ba.isEmpty())
        return QStringLiteral("<empty data>");
    int i = 0;
    if (ba.size() > 1)
        i = MP_CMD_FIELD_INDEX;

    QMetaEnum m = QMetaEnum::fromType<MPCmd::Command>();
    MPCmd::Command c = MPCmd::from(ba.at(i));
    return QString("%1 (%2)")
            .arg(m.valueToKey(c))
            .arg(MPCmd::toHexString(c));
}

MPCmd::Command MPCmd::from(char c)
{
    return static_cast<MPCmd::Command>((quint8)c);
}
