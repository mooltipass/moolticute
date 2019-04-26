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
#include "MPNodeMiniImpl.h"
#include "MPNodeBLEImpl.h"

#include "MPDevice.h"

QByteArray MPNode::EmptyAddress = QByteArray(2, 0);

MPNode::MPNode(const QByteArray &d, QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr):
    QObject(parent),
    address(std::move(nodeAddress)),
    virtualAddress(virt_addr),
    pMesProt(getMesProt(parent)),
    isBLE{"BLE" == pMesProt->getDeviceName()}
{
    createNodeImpl(d);
}

MPNode::MPNode(QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr):
    QObject(parent),
    address(std::move(nodeAddress)),
    virtualAddress(virt_addr),
    pMesProt(getMesProt(parent)),
    isBLE{"BLE" == pMesProt->getDeviceName()}
{
    createNodeImpl(QByteArray{});
}

void MPNode::createNodeImpl(const QByteArray &d)
{
    if (isBLE)
    {
        pNodeImpl = std::unique_ptr<MPNodeBLEImpl>(new MPNodeBLEImpl{d});
        qDebug() << "Node is BLE";
    }
    else
    {
        pNodeImpl = std::unique_ptr<MPNodeMiniImpl>(new MPNodeMiniImpl{d});
        qDebug() << "Node is Mini";
    }
}

int MPNode::getType() const
{
    return pNodeImpl->getType();
}

void MPNode::setType(const quint8 type)
{
    pNodeImpl->setType(type);
}

bool MPNode::isValid() const
{
    if (NodeUnknown == getType())
    {
        return false;
    }

    return pNodeImpl->isValid();
}

bool MPNode::isDataLengthValid() const
{
    return pNodeImpl->isDataLengthValid();
}

void MPNode::appendData(const QByteArray &d)
{
    pNodeImpl->appendData(d);
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
    return pNodeImpl->getPreviousAddress();
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
        pNodeImpl->setPreviousAddress(d);
    }
}

QByteArray MPNode::getNextParentAddress() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return pNodeImpl->getNextAddress();
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
        pNodeImpl->setNextAddress(d);
    }
}

QByteArray MPNode::getStartChildAddress() const
{
    if (!isValid()) return QByteArray();
    if (firstChildVirtualAddressSet) return QByteArray();
    return pNodeImpl->getStartAddress();
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
        pNodeImpl->setStartAddress(d);
    }
}

QString MPNode::getService() const
{
    if (!isValid()) return QString();
    return pMesProt->toQString(pNodeImpl->getService());
}

void MPNode::setService(const QString &service)
{
    if (isValid())
    {
        QByteArray serviceArray = pMesProt->toByteArray(service);
        serviceArray.append('\0');
        serviceArray.resize(MP_MAX_PAYLOAD_LENGTH);
        serviceArray[serviceArray.size()-1] = '\0';
        pNodeImpl->setService(serviceArray);
    }
}

QByteArray MPNode::getStartDataCtr() const
{
    if (!isValid()) return QByteArray();
    return pNodeImpl->getStartDataCtr();
}

QByteArray MPNode::getNextChildAddress() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return pNodeImpl->getNextAddress();
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
        pNodeImpl->setNextAddress(d);
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
    return pNodeImpl->getPreviousAddress();
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
        pNodeImpl->setPreviousAddress(d);
    }
}

QByteArray MPNode::getNextChildDataAddress() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return pNodeImpl->getNextChildDataAddress();
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
        pNodeImpl->setNextChildDataAddress(d);
    }
}

QByteArray MPNode::getCTR() const
{
    if (!isValid()) return QByteArray();
    return pNodeImpl->getCTR();
}

QString MPNode::getDescription() const
{
    if (!isValid()) return QString();
    return pMesProt->toQString(pNodeImpl->getDescription());
}

void MPNode::setDescription(const QString &newDescription)
{
    if (isValid())
    {
        QByteArray desc = pMesProt->toByteArray(newDescription);
        desc.append('\0');
        desc.resize(MP_MAX_DESC_LENGTH);
        desc[desc.size()-1] = '\0';
        pNodeImpl->setDescription(desc);
    }
}

QString MPNode::getLogin() const
{
    if (!isValid()) return QString();
    return pMesProt->toQString(pNodeImpl->getLogin());
}

void MPNode::setLogin(const QString &newLogin)
{
    if (isValid())
    {
        QByteArray login = pMesProt->toByteArray(newLogin);
        login.append('\0');
        login.resize(MP_MAX_PAYLOAD_LENGTH);
        login[login.size()-1] = '\0';
        pNodeImpl->setLogin(login);
    }
}

QByteArray MPNode::getPasswordEnc() const
{
    if (!isValid()) return QByteArray();
    return pNodeImpl->getPasswordEnc();
}

QDate MPNode::getDateCreated() const
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(pNodeImpl->getDateCreated());
}

QDate MPNode::getDateLastUsed() const
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(pNodeImpl->getDateLastUsed());
}

QByteArray MPNode::getNextDataAddress() const
{
    if (!isValid()) return QByteArray();
    return pNodeImpl->getNextDataAddress();
}

QByteArray MPNode::getNodeData() const
{
    if (!isValid()) return QByteArray();
    return pNodeImpl->getData();
}

QByteArray MPNode::getNodeFlags() const
{
    return pNodeImpl->getNodeFlags();
}

QByteArray MPNode::getLoginNodeData() const
{
    // return core data, excluding linked lists and flags
    if (!isValid()) return QByteArray();
    return pNodeImpl->getLoginNodeData();
}

void MPNode::setLoginNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        pNodeImpl->setLoginNodeData(flags, d);
    }
}

QByteArray MPNode::getLoginChildNodeData() const
{
    // return core data, excluding linked lists and flags
    if (!isValid()) return QByteArray();
    return pNodeImpl->getLoginChildNodeData();
}

void MPNode::setLoginChildNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        pNodeImpl->setLoginChildNodeData(flags, d);
    }
}

QByteArray MPNode::getDataNodeData() const
{
    if (!isValid()) return QByteArray();
    return pNodeImpl->getDataNodeData();
}

void MPNode::setDataNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        pNodeImpl->setDataNodeData(flags, d);
    }
}

QByteArray MPNode::getDataChildNodeData() const
{
    if (!isValid()) return QByteArray();
    return pNodeImpl->getDataChildNodeData();
}

void MPNode::setDataChildNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        pNodeImpl->setDataChildNodeData(flags, d);
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

IMessageProtocol *MPNode::getMesProt(QObject *parent)
{
    if (MPDevice* test = dynamic_cast<MPDevice*>(parent))
    {
        return test->getMesProt();
    }
    return nullptr;
}
