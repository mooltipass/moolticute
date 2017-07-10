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

class MPNode: public QObject
{
public:
    MPNode(const QByteArray &d, QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNode(QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);

    enum
    {
        NodeUnknown = -1,
        NodeParent = 0,
        NodeChild = 1,
        NodeParentData = 2,
        NodeChildData = 3,
    };

    //Fill node container with data
    void appendData(const QByteArray &d);
    bool isDataLengthValid() const;
    bool isValid() const;

    /* accessors for node properties */
    void setAddress(const QByteArray &d, const quint32 virt_addr = 0);
    quint32 getVirtualAddress(void) const;
    void setVirtualAddress(quint32 addr);
    void setType(const quint8 type);
    QByteArray getAddress() const;
    int getType() const;

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
    void setService(const QString &service);
    QString getService() const;

    QByteArray getStartDataCtr() const;

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
    QByteArray getCTR() const;
    QString getDescription() const;
    QString getLogin() const;
    QByteArray getPasswordEnc() const;
    QDate getDateCreated() const;
    QDate getDateLastUsed() const;
    void setFavoriteProperty(const quint8 favId);

    //Data node address
    //Address in data node is not at the same position as cred nodes
    void setNextChildDataAddress(const QByteArray &d, const quint32 virt_addr = 0);
    QByteArray getNextChildDataAddress() const;

    // NodeChildData properties
    void setNextDataAddress(const QByteArray &d);
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

    static QByteArray EmptyAddress;

    QJsonObject toJson() const;

private:
    QByteArray data;
    QByteArray address;
    bool mergeTagged = false;
    bool pointedToCheck = false;
    bool firstChildVirtualAddressSet = false;
    quint32 firstChildVirtualAddress = 0;
    bool nextVirtualAddressSet = false;
    quint32 nextVirtualAddress = 0;
    bool prevVirtualAddressSet = false;
    quint32 prevVirtualAddress = 0;
    quint32 virtualAddress = 0;
    qint8 favorite = Common::FAV_NOT_SET;

    QList<MPNode *> childNodes;
    QList<MPNode *> childDataNodes;
};

#endif // MPNODE_H
