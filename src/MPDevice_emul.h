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
#ifndef MPDEVICE_EMUL_H
#define MPDEVICE_EMUL_H
#include <QHash>
#include "MPDevice.h"

class MPDevice_emul : public MPDevice
{
public:
    MPDevice_emul(QObject *parent);
private:
    virtual void platformRead();
    virtual void platformWrite(const QByteArray &data);
    QHash<quint8, quint8> mooltipassParam;
    QHash<QString, QString> passwords;
    QHash<QString, QString> logins;
    QHash<QString, QString> descriptions;
    QString context;
};

#endif // MPDEVICE_EMUL_H
