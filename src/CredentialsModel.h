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
#ifndef CREDENTIALSMODEL_H
#define CREDENTIALSMODEL_H

#include <QtCore>
#include <QStandardItemModel>
#include "Common.h"

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

class CredentialsModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    CredentialsModel(QObject *parent = 0);

    class Credential
    {
    public:
        QString service;
        QString login;
        QString password;
        QString description;
        QDate createdDate;
        QDate updatedDate;
        QByteArray address;
        qint8 favorite;

        bool operator==(const Credential &other) const
        {
            if (address.isEmpty() || other.address.isEmpty())
            {
                //if address is defined for both, check equality of address
                return address == other.address;
            }
            return service == other.service &&
                    login == other.login;
        }

        QJsonObject toJson() const;
    };

    void load(const QJsonArray &creds);
    void setClearTextPassword(const QString &service, const QString &login, const QString &password);
    void update(const QString &service, const QString &login, const QString &password, const QString &description);
    void update(const QModelIndex &idx, const Credential &cred);
    void removeCredential(const QModelIndex &idx);

    QJsonArray getJsonChanges();

    enum ColumnIdx
    {
        ServiceIdx,
        LoginIdx,
        PasswordIdx,
        DescriptionIdx,
        DateCreatedIdx,
        DateModifiedIdx,
        FavoriteIdx,
        ColumnCount,
    };
    enum CustomRole {
        LoginRole = Qt::UserRole + 1,
        PasswordUnlockedRole,
        FavRole,
    };

protected:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    //currently displayed list
    QVector<Credential> m_credentials;

    auto at(int idx) const -> const Credential&;

    void mergeWith(const QVector<Credential> &);

    friend class CredentialsFilterModel;
    friend class CredentialsManagement;
    friend class ServiceItemDelegate;
};

#endif // CREDENTIALSMODEL_H
