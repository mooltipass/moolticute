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

#include "MPDevice.h"
#include "MPNodeBLE.h"

QByteArray MPNode::EmptyAddress = QByteArray(2, 0);

MPNode::MPNode(const QByteArray &d, QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr):
    QObject(parent),
    data(d),
    address(nodeAddress),
    virtualAddress(virt_addr),
    pMesProt(getMesProt(parent)),
    isBLE{pMesProt->isBLE()}
{
}

MPNode::MPNode(QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr):
    QObject(parent),
    address(nodeAddress),
    virtualAddress(virt_addr),
    pMesProt(getMesProt(parent)),
    isBLE{pMesProt->isBLE()}
{
}

MPNode::MPNode(QByteArray &&d, QObject *parent, QByteArray &&nodeAddress, const quint32 virt_addr):
    QObject(parent),
    data(qMove(d)),
    address(qMove(nodeAddress)),
    virtualAddress(virt_addr),
    pMesProt(getMesProt(parent)),
    isBLE{pMesProt->isBLE()}
{
}

MPNode::MPNode(QObject *parent, QByteArray &&nodeAddress, const quint32 virt_addr):
    QObject(parent),
    address(qMove(nodeAddress)),
    virtualAddress(virt_addr),
    pMesProt(getMesProt(parent)),
    isBLE{pMesProt->isBLE()}
{
}

void MPNode::appendData(const QByteArray &d)
{
    data.append(d);
}

int MPNode::getType() const
{
    if (data.size() > 1)
    {
        return (static_cast<quint8>(data[1]) >> 6) & 0x03;
    }
    return NodeUnknown;
}

void MPNode::setType(const quint8 type)
{
    if (data.size() > 1)
    {
        data[1] = type << 6;
    }
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

void MPNode::setNotDeletedTagged()
{
    notDeletedTagged = true;
}

bool MPNode::getNotDeletedTagged() const
{
    return notDeletedTagged;
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

qint8 MPNode::getFavoriteProperty() const
{
    return favorite;
}

void MPNode::setFavoriteProperty(const qint8 favId)
{
    favorite = favId;
}

quint32 MPNode::getEncDataSize() const
{
    return encDataSize;
}

void MPNode::setEncDataSize(const quint32 encSize)
{
    encDataSize = encSize;
}

QByteArray MPNode::getPreviousParentAddress() const
{
    if (!isValid()) return QByteArray();
    if (prevVirtualAddressSet) return QByteArray();
    return data.mid(PREVIOUS_PARENT_ADDR_START, ADDRESS_LENGTH);
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
        data[PREVIOUS_PARENT_ADDR_START] = d[0];
        data[PREVIOUS_PARENT_ADDR_START+1] = d[1];
    }
}

QByteArray MPNode::getNextParentAddress() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return data.mid(NEXT_PARENT_ADDR_START, ADDRESS_LENGTH);
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
        data[NEXT_PARENT_ADDR_START] = d[0];
        data[NEXT_PARENT_ADDR_START+1] = d[1];
    }
}

QByteArray MPNode::getStartChildAddress() const
{
    if (!isValid()) return QByteArray();
    if (firstChildVirtualAddressSet) return QByteArray();
    return data.mid(START_CHILD_ADDR_START, ADDRESS_LENGTH);
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
        data[START_CHILD_ADDR_START] = d[0];
        data[START_CHILD_ADDR_START+1] = d[1];
    }
}

QByteArray MPNode::getNextChildAddress() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return data.mid(NEXT_PARENT_ADDR_START, ADDRESS_LENGTH);
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
        data[NEXT_PARENT_ADDR_START] = d[0];
        data[NEXT_PARENT_ADDR_START+1] = d[1];
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
    return data.mid(PREVIOUS_PARENT_ADDR_START, ADDRESS_LENGTH);
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
        data[PREVIOUS_PARENT_ADDR_START] = d[0];
        data[PREVIOUS_PARENT_ADDR_START+1] = d[1];
    }
}

QByteArray MPNode::getNextChildDataAddress() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return data.mid(NEXT_DATA_ADDR_START, ADDRESS_LENGTH);
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
        data[NEXT_DATA_ADDR_START] = d[0];
        data[NEXT_DATA_ADDR_START+1] = d[1];
    }
}

QByteArray MPNode::getNextDataAddress() const
{
    if (!isValid()) return QByteArray();
    return data.mid(NEXT_DATA_ADDR_START, ADDRESS_LENGTH);
}

QByteArray MPNode::getNodeData() const
{
    if (!isValid()) return QByteArray();
    return data;
}

QByteArray MPNode::getNodeFlags() const
{
    return data.mid(NODE_FLAG_ADDR_START, ADDRESS_LENGTH);
}

QByteArray MPNode::getLoginNodeData() const
{
    // return core data, excluding linked lists and flags
    if (!isValid()) return QByteArray();
    return data.mid(DATA_ADDR_START);
}

void MPNode::setLoginNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        data.replace(DATA_ADDR_START, pMesProt->getParentNodeSize()-DATA_ADDR_START, d);
        data.replace(0, ADDRESS_LENGTH, flags);
    }
}

QByteArray MPNode::getLoginChildNodeData() const
{
    // return core data, excluding linked lists and flags
    if (!isValid()) return QByteArray();
    return data.mid(LOGIN_CHILD_NODE_DATA_ADDR_START);
}

void MPNode::setLoginChildNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        data.replace(LOGIN_CHILD_NODE_DATA_ADDR_START, pMesProt->getChildNodeSize()-LOGIN_CHILD_NODE_DATA_ADDR_START, d);
        data.replace(0, ADDRESS_LENGTH, flags);
    }
}

QByteArray MPNode::getDataNodeData() const
{
    if (!isValid()) return QByteArray();
    return data.mid(DATA_ADDR_START);
}

void MPNode::setDataNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        data.replace(DATA_ADDR_START, pMesProt->getParentNodeSize()-DATA_ADDR_START, d);
        data.replace(0, ADDRESS_LENGTH, flags);
    }
}

QByteArray MPNode::getDataChildNodeData() const
{
    if (!isValid()) return QByteArray();
    return data.mid(DATA_CHILD_DATA_ADDR_START);
}

void MPNode::setDataChildNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        data.replace(DATA_CHILD_DATA_ADDR_START, pMesProt->getChildNodeSize()-DATA_CHILD_DATA_ADDR_START, d);
        data.replace(0, ADDRESS_LENGTH, flags);
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
        if (isBLE)
        {
            auto* bleNode = static_cast<const MPNodeBLE*>(this);
            obj["category"] = QString::number(bleNode->getCategory());
            obj["key_after_login"] = QString::number(bleNode->getKeyAfterLogin());
            obj["key_after_pwd"] = QString::number(bleNode->getKeyAfterPwd());
            obj["pwd_blank_flag"] = QString::number(bleNode->getPwdBlankFlag());
        }
    }
    else if (getType() == NodeChildData)
    {
        obj["data"] = Common::bytesToJson(getDataChildNodeData());
    }

    return obj;
}

IMessageProtocol *MPNode::getMesProt(QObject *parent)
{
    if (MPDevice* test = dynamic_cast<MPDevice*>(parent))
    {
        return test->getMesProt();
    }
    qFatal("Parent is not an MPDevice");
    return nullptr;
}
