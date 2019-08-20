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
#ifndef MPNODE_H
#define MPNODE_H

#include "Common.h"
#include "IMessageProtocol.h"

class MPNode: public QObject
{
public:
    MPNode(const QByteArray &d, QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNode(QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNode(QByteArray &&d, QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNode(QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);

    enum NodeType
    {
        NodeUnknown = -1,
        NodeParent = 0,
        NodeChild = 1,
        NodeParentData = 2,
        NodeChildData = 3,
    };

    //Fill node container with data
    void appendData(const QByteArray &d);
    virtual bool isDataLengthValid() const = 0;
    virtual bool isValid() const = 0;
    int getType() const;
    void setType(const quint8 type);

    /* accessors for node properties */
    void setAddress(const QByteArray &d, const quint32 virt_addr = 0);
    quint32 getVirtualAddress(void) const;
    void setVirtualAddress(quint32 addr);

    QByteArray getAddress() const;

    // NodeParent / NodeParentData properties
    void setPreviousParentAddress(const QByteArray &d, const quint32 virt_addr = 0);
    QByteArray getPreviousParentAddress() const;
    quint32 getPreviousParentVirtualAddress() const;
    void setNextParentAddress(const QByteArray &d, const quint32 virt_addr = 0);
    QByteArray getNextParentAddress() const;
    quint32 getNextParentVirtualAddress() const;
    void setStartChildAddress(const QByteArray &d, const quint32 virt_addr = 0);
    quint32 getStartChildVirtualAddress() const;
    QByteArray getStartChildAddress() const;

    QList<MPNode *> &getChildNodes() { return childNodes; }
    void appendChild(MPNode *node) { node->setParent(this); childNodes.append(node); }
    void removeChild(MPNode *node) { node->setParent(nullptr); childNodes.removeAll(node); }

    QList<MPNode *> &getChildDataNodes() { return childDataNodes; }
    void appendChildData(MPNode *node) { node->setParent(this); childDataNodes.append(node); }
    void removeChildData(MPNode *node) { node->setParent(nullptr); childDataNodes.removeAll(node); }

    // NodeChild properties
    void setNextChildAddress(const QByteArray &d, const quint32 virt_addr = 0);
    quint32 getNextChildVirtualAddress(void) const;
    QByteArray getNextChildAddress() const;
    void setPreviousChildAddress(const QByteArray &d, const quint32 virt_addr = 0);
    quint32 getPreviousChildVirtualAddress(void) const;
    QByteArray getPreviousChildAddress() const;

    virtual QString getService() const = 0;
    virtual void setService(const QString &service) = 0;
    virtual QByteArray getStartDataCtr() const = 0;
    virtual QByteArray getCTR() const = 0;
    virtual QString getDescription() const = 0;
    virtual void setDescription(const QString &newDescription) = 0;
    virtual QString getLogin() const = 0;
    virtual void setLogin(const QString &newLogin) = 0;
    virtual QByteArray getPasswordEnc() const = 0;
    virtual QDate getDateCreated() const = 0;
    virtual QDate getDateLastUsed() const = 0;

    virtual void setFavoriteProperty(const qint8 favId);
    virtual qint8 getFavoriteProperty() const;
    virtual quint32 getEncDataSize() const;
    virtual void setEncDataSize(const quint32 encSize);

    //Data node address
    //Address in data node is not at the same position as cred nodes
    void setNextChildDataAddress(const QByteArray &d, const quint32 virt_addr = 0);
    QByteArray getNextChildDataAddress() const;

    // NodeChildData properties
    QByteArray getNextDataAddress() const;

    // Node properties
    QByteArray getNodeData() const;
    QByteArray getNodeFlags() const;

    // Access node core data
    void setLoginNodeData(const QByteArray &flags, const QByteArray &d);
    QByteArray getLoginNodeData() const;
    void setLoginChildNodeData(const QByteArray &flags, const QByteArray &d);
    QByteArray getLoginChildNodeData() const;
    void setDataNodeData(const QByteArray &flags, const QByteArray &d);
    QByteArray getDataNodeData() const;
    void setDataChildNodeData(const QByteArray &flags, const QByteArray &d);
    QByteArray getDataChildNodeData() const;

    // Pointedto access/write
    void setPointedToCheck();
    void removePointedToCheck();
    bool getPointedToCheck() const;

    // MergedTagged access/write
    void setMergeTagged();
    bool getMergeTagged() const;

    // NotDeletedTagged access/write
    void setNotDeletedTagged();
    bool getNotDeletedTagged() const;

    static QByteArray EmptyAddress;

    QJsonObject toJson() const;

protected:
    IMessageProtocol* getMesProt(QObject *parent);

    QByteArray data;
    QByteArray address;
    bool mergeTagged = false;
    bool pointedToCheck = false;
    bool notDeletedTagged = false;
    bool firstChildVirtualAddressSet = false;
    quint32 firstChildVirtualAddress = 0;
    bool nextVirtualAddressSet = false;
    quint32 nextVirtualAddress = 0;
    bool prevVirtualAddressSet = false;
    quint32 prevVirtualAddress = 0;
    quint32 virtualAddress = 0;
    quint32 encDataSize = 0;
    qint8 favorite = Common::FAV_NOT_SET;

    QList<MPNode *> childNodes;
    QList<MPNode *> childDataNodes;

    IMessageProtocol *pMesProt = nullptr;
    const bool isBLE = false;

    static constexpr int ADDRESS_LENGTH = 2;
    static constexpr int CTR_LENGTH = 3;
    static constexpr int NODE_FLAG_ADDR_START = 0;

    static constexpr int PREVIOUS_PARENT_ADDR_START = 2;
    static constexpr int NEXT_PARENT_ADDR_START = 4;
    static constexpr int START_CHILD_ADDR_START = 6;

    static constexpr int NEXT_DATA_ADDR_START = 2;
    static constexpr int DATA_CHILD_DATA_ADDR_START = 4;
    static constexpr int LOGIN_CHILD_NODE_DATA_ADDR_START = 6;
    static constexpr int DATA_ADDR_START = 8;

    static constexpr int SERVICE_ADDR_START = 8;
};

#endif // MPNODE_H
