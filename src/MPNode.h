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
    MPNode(const QByteArray &d, QObject *parent = nullptr);
    MPNode(QObject *parent = nullptr);

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
    bool isValid();

    /* accessors for node properties */

    int getType();

    // NodeParent / NodeParentData properties
    QByteArray getPreviousParentAddress();
    QByteArray getNextParentAddress();
    QByteArray getStartChildAddress();
    QString getService();

    QByteArray getStartDataCtr();

    QList<MPNode *> &getChildNodes() { return childNodes; }
    void appendChild(MPNode *node) { node->setParent(this); childNodes.append(node); }

    QList<MPNode *> &getChildDataNodes() { return childDataNodes; }
    void appendChildData(MPNode *node) { node->setParent(this); childDataNodes.append(node); }

    // NodeChild properties
    QByteArray getNextChildAddress();
    QByteArray getPreviousChildAddress();
    QByteArray getCTR();
    QString getDescription();
    QString getLogin();
    QByteArray getPasswordEnc();
    QDate getDateCreated();
    QDate getDateLastUsed();

    //Data node address
    //Address in data node is not at the same position as cred nodes
    QByteArray getNextChildDataAddress();

    // NodeChildData properties
    QByteArray getNextDataAddress();
    QByteArray getChildData();

    static QByteArray EmptyAddress;

    QJsonObject toJson();

private:
    QByteArray data;

    QList<MPNode *> childNodes;
    QList<MPNode *> childDataNodes;
};

#endif // MPNODE_H
