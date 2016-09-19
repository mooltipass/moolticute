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
#include "CredentialsModel.h"

CredentialsFilterModel::CredentialsFilterModel(QObject *parent):
    QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

void CredentialsFilterModel::setFilter(const QString &filter_str)
{
    filter = filter_str.toLower();
    invalidate();
}

int CredentialsFilterModel::indexToSource(int idx)
{
    return mapToSource(index(idx, 0)).row();
}

int CredentialsFilterModel::indexFromSource(int idx)
{
    return mapFromSource(index(idx, 0)).row();
}

bool CredentialsFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)

    //shortcut
    if (filter.isEmpty())
        return true;

    CredentialsModel *credModel = qobject_cast<CredentialsModel *>(sourceModel());

    for (int i = 0;i < credModel->columnCount();i++)
    {
        QStandardItem *item = credModel->item(source_row, i);
        if (item && item->text().toLower().contains(filter))
            return true;
    }

    return false;
}

bool CredentialsFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    CredentialsModel *credModel = qobject_cast<CredentialsModel *>(sourceModel());

    QStandardItem *itemLeft = credModel->itemFromIndex(left);
    QStandardItem *itemRight = credModel->itemFromIndex(right);

    QString l, r;

    l = itemLeft->text();
    r = itemRight->text();

    return l < r;
}

CredentialsModel::CredentialsModel(QObject *parent):
    QStandardItemModel(parent)
{
}

void CredentialsModel::load(const QJsonArray &creds)
{
    credentials = creds;
    reloadData();
}

void CredentialsModel::reloadData()
{
    clear();

    setHorizontalHeaderLabels(QStringList()
                              << tr("Service")
                              << tr("Login")
                              << tr("Password")
                              << tr("Description")
                              << tr("Date created")
                              << tr("Date last used"));

    for (int i = 0;i < credentials.size();i++)
    {
        QJsonObject pnode = credentials.at(i).toObject();

        QStandardItem *pitem = new QStandardItem(pnode["service"].toString());
        pitem->setData(TypeService, RoleType);

        QJsonArray jchilds = pnode["childs"].toArray();
        for (int j = 0;j < jchilds.size();j++)
        {
            QJsonObject cnode = jchilds.at(j).toObject();
            QList<QStandardItem *> rowData;

            rowData << new QStandardItem();
            rowData.last()->setData(TypeLogin, RoleType);

            rowData << new QStandardItem(cnode["login"].toString());
            rowData.last()->setData(TypeLogin, RoleType);

            rowData << new QStandardItem("******");
            rowData.last()->setData(TypeLogin, RoleType);
            rowData.last()->setData(false, RoleHasPassword);

            rowData << new QStandardItem(cnode["description"].toString());
            rowData.last()->setData(TypeLogin, RoleType);

            QDate dcrea = QDate::fromString(cnode["date_created"].toString(), Qt::ISODate);
            rowData << new QStandardItem(dcrea.toString(Qt::SystemLocaleShortDate));
            rowData.last()->setData(TypeLogin, RoleType);

            QDate dlast = QDate::fromString(cnode["date_last_used"].toString(), Qt::ISODate);
            rowData << new QStandardItem(dlast.toString(Qt::SystemLocaleShortDate));
            rowData.last()->setData(TypeLogin, RoleType);

            pitem->appendRow(rowData);
        }

        appendRow(pitem);
    }
}
