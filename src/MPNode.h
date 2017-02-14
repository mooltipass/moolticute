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
    quint32 getVirtualAddress(void) const;
    void setVirtualAddress(quint32 addr);
    QByteArray getAddress() const;
    int getType() const;

    // NodeParent / NodeParentData properties
    void setPreviousParentAddress(const QByteArray &d, const quint32 virt_addr = 0);
    QByteArray getPreviousParentAddress() const;
    quint32 getPrevParentVirtualAddress() const;
    void setNextParentAddress(const QByteArray &d, const quint32 virt_addr = 0);
    QByteArray getNextParentAddress() const;
    quint32 getNextParentVirtualAddress() const;
    void setStartChildAddress(const QByteArray &d, const quint32 virt_addr = 0);
    quint32 getFirstChildVirtualAddress() const;
    QByteArray getStartChildAddress() const;
    void setService(const QString &service);
    QString getService() const;

    QByteArray getStartDataCtr() const;

    QList<MPNode *> &getChildNodes() { return childNodes; }
    void appendChild(MPNode *node) { node->setParent(this); childNodes.append(node); }

    QList<MPNode *> &getChildDataNodes() { return childDataNodes; }
    void appendChildData(MPNode *node) { node->setParent(this); childDataNodes.append(node); }

    // NodeChild properties
    void setNextChildAddress(const QByteArray &d);
    QByteArray getNextChildAddress() const;
    void setPreviousChildAddress(const QByteArray &d);
    QByteArray getPreviousChildAddress() const;
    QByteArray getCTR() const;
    QString getDescription() const;
    QString getLogin() const;
    QByteArray getPasswordEnc() const;
    QDate getDateCreated() const;
    QDate getDateLastUsed() const;

    //Data node address
    //Address in data node is not at the same position as cred nodes
    void setNextChildDataAddress(const QByteArray &d);
    QByteArray getNextChildDataAddress() const;

    // NodeChildData properties
    void setNextDataAddress(const QByteArray &d);
    QByteArray getNextDataAddress() const;
    QByteArray getChildData() const;

    // Node properties
    QByteArray getNodeData() const;

    // Pointedto access/write
    void setPointedToCheck();
    void removePointedToCheck();
    bool getPointedToCheck() const;

    static QByteArray EmptyAddress;

    QJsonObject toJson() const;

private:
    QByteArray data;
    QByteArray address;
    bool pointedToCheck = false;
    quint32 firstChildVirtualAddress = 0;
    quint32 nextVirtualAddress = 0;
    quint32 prevVirtualAddress = 0;
    quint32 virtualAddress = 0;

    QList<MPNode *> childNodes;
    QList<MPNode *> childDataNodes;
};

#endif // MPNODE_H
