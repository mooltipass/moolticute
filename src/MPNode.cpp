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
#include "MPNode.h"

QByteArray MPNode::EmptyAddress = QByteArray(2, 0);

MPNode::MPNode(const QByteArray &d, QObject *parent):
    QObject(parent),
    data(std::move(d))
{
}

MPNode::MPNode(QObject *parent):
    QObject(parent)
{
}

int MPNode::getType()
{
    if (data.size() > 1)
        return ((quint8)data[1] >> 6) & 0x03;
    return -1;
}

bool MPNode::isValid()
{
    return getType() != NodeUnknown &&
           data.size() == MP_NODE_SIZE &&
           ((quint8)data[1] & 0x20) == 0;
}

void MPNode::appendData(const QByteArray &d)
{
    data.append(d);
}

QByteArray MPNode::getPreviousParentAddress()
{
    if (!isValid()) return QByteArray();
    return data.mid(2, 2);
}

QByteArray MPNode::getNextParentAddress()
{
    if (!isValid()) return QByteArray();
    return data.mid(4, 2);
}

QByteArray MPNode::getStartChildAddress()
{
    if (!isValid()) return QByteArray();
    return data.mid(6, 2);
}

QString MPNode::getService()
{
    if (!isValid()) return QString();
    return QString::fromUtf8(data.mid(8, MP_NODE_SIZE - 8 - 3));
}

QByteArray MPNode::getStartDataCtr()
{
    if (!isValid()) return QByteArray();
    return data.mid(129, 3);
}

QByteArray MPNode::getNextChildAddress()
{
    if (!isValid()) return QByteArray();
    return data.mid(4, 2);
}

QByteArray MPNode::getPreviousChildAddress()
{
    if (!isValid()) return QByteArray();
    return data.mid(2, 2);
}

QByteArray MPNode::getNextChildDataAddress()
{
    //in data nodes, there is no linked list
    //the only address is the next one
    //It is the same as previous for cred nodes
    return getPreviousChildAddress();
}

QByteArray MPNode::getCTR()
{
    if (!isValid()) return QByteArray();
    return data.mid(34, 3);
}

QString MPNode::getDescription()
{
    if (!isValid()) return QString();
    return QString::fromUtf8(data.mid(6, 24));
}

QString MPNode::getLogin()
{
    if (!isValid()) return QString();
    return QString::fromUtf8(data.mid(37, 63));
}

QByteArray MPNode::getPasswordEnc()
{
    if (!isValid()) return QByteArray();
    return data.mid(100, 32);
}

QDate MPNode::getDateCreated()
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(data.mid(30, 2));
}

QDate MPNode::getDateLastUsed()
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(data.mid(32, 2));
}

QByteArray MPNode::getNextDataAddress()
{
    if (!isValid()) return QByteArray();
    return data.mid(2, 2);
}

QByteArray MPNode::getChildData()
{
    if (!isValid()) return QByteArray();
    return data.mid(4);
}

QJsonObject MPNode::toJson()
{
    QJsonObject obj;

    if (getType() == NodeParent)
    {
        obj["service"] = getService();

        QJsonArray childs;
        foreach (MPNode *cnode, childNodes)
        {
            QJsonObject cobj = cnode->toJson();
            childs.append(cobj);
        }

        obj["childs"] = childs;
    }
    else if (getType() == NodeParentData)
    {
        obj["service"] = getService();

        QJsonArray childs;
        foreach (MPNode *cnode, childDataNodes)
        {
            QJsonObject cobj = cnode->toJson();
            childs.append(cobj);
        }

        obj["childs"] = childs;
    }
    else if (getType() == NodeChild)
    {
        obj["login"] = getLogin();
        obj["description"] = getDescription();
        obj["password_enc"] = Common::bytesToJson(getPasswordEnc());
        obj["date_created"] = getDateCreated().toString(Qt::ISODate);
        obj["date_last_used"] = getDateLastUsed().toString(Qt::ISODate);
    }
    else if (getType() == NodeChildData)
    {
        obj["data"] = Common::bytesToJson(getChildData());
    }

    return obj;
}
