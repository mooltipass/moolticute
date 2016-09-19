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
#ifndef CREDENTIALSMODEL_H
#define CREDENTIALSMODEL_H

#include <QtCore>
#include <QStandardItemModel>

class CredentialsFilterModel: public QSortFilterProxyModel
{
    Q_OBJECT
public:
    CredentialsFilterModel(QObject *parent = 0);

    void setFilter(const QString &filter_str);

    Q_INVOKABLE int indexToSource(int idx);
    Q_INVOKABLE int indexFromSource(int idx);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    QString filter;
};

class CredentialsModel: public QStandardItemModel
{
    Q_OBJECT
public:
    CredentialsModel(QObject *parent = 0);

    void load(const QJsonArray &creds);
    void reloadData();

    enum
    {
        RoleType = Qt::UserRole + 1,
        RoleHasPassword,
    };

    enum
    {
        TypeService,
        TypeLogin,
        TypeData,
    };

private:
    QJsonArray credentials;
};

#endif // CREDENTIALSMODEL_H
