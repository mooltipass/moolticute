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
#include "MPNode.h"
#include "MooltipassCmds.h"

QByteArray MPNode::EmptyAddress = QByteArray(2, 0);

MPNode::MPNode(const QByteArray &d, QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr):
    QObject(parent),
    data(std::move(d)),
    address(std::move(nodeAddress)),
    virtualAddress(virt_addr)
{
}

MPNode::MPNode(QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr):
    QObject(parent),
    address(std::move(nodeAddress)),
    virtualAddress(virt_addr)
{
}

int MPNode::getType() const
{
    if (data.size() > 1)
        return ((quint8)data[1] >> 6) & 0x03;
    return -1;
}

void MPNode::setType(const quint8 type)
{
    if (data.size() > 1)
    {
        data[1] = type << 6;
    }
}

bool MPNode::isValid() const
{
    return getType() != NodeUnknown &&
           data.size() == MP_NODE_SIZE &&
           ((quint8)data[1] & 0x20) == 0;
}

bool MPNode::isDataLengthValid() const
{
    return data.size() == MP_NODE_SIZE;
}

void MPNode::appendData(const QByteArray &d)
{
    data.append(d);
}

QByteArray MPNode::getAddress() const
{
    return address;
}

void MPNode::setAddress(const QByteArray &d, const quint32 virt_addr)
{
    address = d;
    virtualAddress = virt_addr;
}

void MPNode::setVirtualAddress(quint32 addr)
{
    virtualAddress = addr;
}

quint32 MPNode::getVirtualAddress(void) const
{
    return virtualAddress;
}

void MPNode::setPointedToCheck()
{
    pointedToCheck = true;
}

void MPNode::removePointedToCheck()
{
    pointedToCheck = false;
}

void MPNode::setMergeTagged()
{
    mergeTagged = true;
}

bool MPNode::getMergeTagged() const
{
    return mergeTagged;
}

bool MPNode::getPointedToCheck() const
{
    return pointedToCheck;
}

void MPNode::setFavoriteProperty(const quint8 favId)
{
    favorite = favId;
}

QByteArray MPNode::getPreviousParentAddress() const
{
    if (!isValid()) return QByteArray();
    if (prevVirtualAddressSet) return QByteArray();
    return data.mid(2, 2);
}

quint32 MPNode::getPreviousParentVirtualAddress() const
{
    return prevVirtualAddress;
}

void MPNode::setPreviousParentAddress(const QByteArray &d, const quint32 virt_addr)
{
    if (d.isNull())
    {
        prevVirtualAddress = virt_addr;
        prevVirtualAddressSet = true;
    }
    else
    {
        prevVirtualAddressSet = false;
        data[2] = d[0];
        data[3] = d[1];
    }
}

QByteArray MPNode::getNextParentAddress() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return data.mid(4, 2);
}

quint32 MPNode::getNextParentVirtualAddress() const
{
    return nextVirtualAddress;
}

void MPNode::setNextParentAddress(const QByteArray &d, const quint32 virt_addr)
{
    if (d.isNull())
    {
        nextVirtualAddress = virt_addr;
        nextVirtualAddressSet = true;
    }
    else
    {
        nextVirtualAddressSet = false;
        data[4] = d[0];
        data[5] = d[1];
    }
}

QByteArray MPNode::getStartChildAddress() const
{
    if (!isValid()) return QByteArray();
    if (firstChildVirtualAddressSet) return QByteArray();
    return data.mid(6, 2);
}

quint32 MPNode::getStartChildVirtualAddress() const
{
    return firstChildVirtualAddress;
}

void MPNode::setStartChildAddress(const QByteArray &d, const quint32 virt_addr)
{
    if (d.isNull())
    {
        firstChildVirtualAddress = virt_addr;
        firstChildVirtualAddressSet = true;
    }
    else
    {
        firstChildVirtualAddressSet = false;
        data[6] = d[0];
        data[7] = d[1];
    }
}

QString MPNode::getService() const
{
    if (!isValid()) return QString();
    return QString::fromUtf8(data.mid(8, MP_NODE_SIZE - 8 - 3));
}

void MPNode::setService(const QString &service)
{
    if (isValid())
    {
        QByteArray serviceArray = service.toUtf8();
        serviceArray.truncate(MP_MAX_SERVICE_LENGTH - 1);
        data.replace(8, serviceArray.size(), serviceArray);
    }
}

QByteArray MPNode::getStartDataCtr() const
{
    if (!isValid()) return QByteArray();
    return data.mid(129, 3);
}

QByteArray MPNode::getNextChildAddress() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return data.mid(4, 2);
}

quint32 MPNode::getNextChildVirtualAddress(void) const
{
    return nextVirtualAddress;
}

void MPNode::setNextChildAddress(const QByteArray &d, const quint32 virt_addr)
{
    if (d.isNull())
    {
        nextVirtualAddress = virt_addr;
        nextVirtualAddressSet = true;
    }
    else
    {
        nextVirtualAddressSet = false;
        data[4] = d[0];
        data[5] = d[1];
    }
}

quint32  MPNode::getPreviousChildVirtualAddress(void) const
{
    return prevVirtualAddress;
}

QByteArray MPNode::getPreviousChildAddress() const
{
    if (!isValid()) return QByteArray();
    if (prevVirtualAddressSet) return QByteArray();
    return data.mid(2, 2);
}

void MPNode::setPreviousChildAddress(const QByteArray &d, const quint32 virt_addr)
{
    if (d.isNull())
    {
        prevVirtualAddress = virt_addr;
        prevVirtualAddressSet = true;
    }
    else
    {
        prevVirtualAddressSet = false;
        data[2] = d[0];
        data[3] = d[1];
    }
}

QByteArray MPNode::getNextChildDataAddress() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return data.mid(2, 2);
}

void MPNode::setNextChildDataAddress(const QByteArray &d, const quint32 virt_addr)
{
    if (d.isNull())
    {
        nextVirtualAddress = virt_addr;
        nextVirtualAddressSet = true;
    }
    else
    {
        nextVirtualAddressSet = false;
        data[2] = d[0];
        data[3] = d[1];
    }
}

QByteArray MPNode::getCTR() const
{
    if (!isValid()) return QByteArray();
    return data.mid(34, 3);
}

QString MPNode::getDescription() const
{
    if (!isValid()) return QString();
    return QString::fromUtf8(data.mid(6, 24));
}

QString MPNode::getLogin() const
{
    if (!isValid()) return QString();
    return QString::fromUtf8(data.mid(37, 63));
}

QByteArray MPNode::getPasswordEnc() const
{
    if (!isValid()) return QByteArray();
    return data.mid(100, 32);
}

QDate MPNode::getDateCreated() const
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(data.mid(30, 2));
}

QDate MPNode::getDateLastUsed() const
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(data.mid(32, 2));
}

QByteArray MPNode::getNextDataAddress() const
{
    if (!isValid()) return QByteArray();
    return data.mid(2, 2);
}

void MPNode::setNextDataAddress(const QByteArray &d)
{
    data[2] = d[0];
    data[3] = d[1];
}

QByteArray MPNode::getNodeData() const
{
    if (!isValid()) return QByteArray();
    return data;
}

QByteArray MPNode::getNodeFlags() const
{
    return data.mid(0, 2);
}

QByteArray MPNode::getLoginNodeData() const
{
    // return core data, excluding linked lists and flags
    if (!isValid()) return QByteArray();
    return data.mid(8);
}

void MPNode::setLoginNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        data.replace(8, MP_NODE_SIZE-8, d);
        data.replace(0, 2, flags);
    }
}

QByteArray MPNode::getLoginChildNodeData() const
{
    // return core data, excluding linked lists and flags
    if (!isValid()) return QByteArray();
    return data.mid(6);
}

void MPNode::setLoginChildNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        data.replace(6, MP_NODE_SIZE-6, d);
        data.replace(0, 2, flags);
    }
}

QByteArray MPNode::getDataNodeData() const
{
    if (!isValid()) return QByteArray();
    return data.mid(8);
}

void MPNode::setDataNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        data.replace(8, MP_NODE_SIZE-8, d);
        data.replace(0, 2, flags);
    }
}

QByteArray MPNode::getDataChildNodeData() const
{
    if (!isValid()) return QByteArray();
    return data.mid(4);
}

void MPNode::setDataChildNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        data.replace(4, MP_NODE_SIZE-4, d);
        data.replace(0, 2, flags);
    }
}

QJsonObject MPNode::toJson() const
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
        obj["address"] = QJsonArray({{ address.at(0) },
                                     { address.at(1) }});
        obj["favorite"] = favorite;
    }
    else if (getType() == NodeChildData)
    {
        obj["data"] = Common::bytesToJson(getDataChildNodeData());
    }

    return obj;
}
