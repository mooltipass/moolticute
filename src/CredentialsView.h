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
#ifndef CREDENTIALSVIEW_H
#define CREDENTIALSVIEW_H

#include <QListView>
#include <QItemDelegate>
#include <functional>


class CredentialsView : public QListView
{
public:
    explicit CredentialsView(QWidget *parent = nullptr);
};

class ConditionalItemSelectionModel : public QItemSelectionModel {
    Q_OBJECT
public:
    using TestFunction = std::function<bool(const QModelIndex & idx)>;
    ConditionalItemSelectionModel(TestFunction f, QAbstractItemModel *model =  nullptr);

public Q_SLOTS:
    void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override;
    void select(const QItemSelection & selection, QItemSelectionModel::SelectionFlags command) override;
    void setCurrentIndex(const QModelIndex & index, QItemSelectionModel::SelectionFlags command) override;

private:
    bool canChangeIndex();

    TestFunction cb;
    quint64 lastRequestTime;
};

class CredentialViewItemDelegate : public QItemDelegate {

public:
    using QItemDelegate::QItemDelegate;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
};

#endif // CREDENTIALSVIEW_H
