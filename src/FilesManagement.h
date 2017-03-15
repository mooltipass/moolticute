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
#ifndef FILESMANAGEMENT_H
#define FILESMANAGEMENT_H

#include <QtWidgets>
#include "WSClient.h"

namespace Ui {
class FilesManagement;
}

class FilesFilterModel: public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FilesFilterModel(QObject *parent = 0);

    void setFilter(const QString &filter_str);

    Q_INVOKABLE int indexToSource(int idx);
    Q_INVOKABLE int indexFromSource(int idx);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    QString filter;
};

class FilesManagement : public QWidget
{
    Q_OBJECT

public:
    explicit FilesManagement(QWidget *parent = 0);
    ~FilesManagement();

    void setWsClient(WSClient *c);

signals:
    void wantEnterMemMode();

private slots:
    void enableMemManagement(bool);
    void dataFileRequested(const QString &service, const QByteArray &data, bool success);
    void dataFileSent(const QString &service, bool success);
    void updateProgress(int total, int curr);
    void updateButtonsUI();

    void on_pushButtonEnterMMM_clicked();
    void on_buttonQuitMMM_clicked();

    void currentSelectionChanged(const QModelIndex &curr, const QModelIndex &);

    void on_pushButtonUpdateFile_clicked();
    void on_pushButtonSaveFile_clicked();
    void on_pushButtonDelFile_clicked();

    void on_addFileButton_clicked();

    void on_pushButtonFilename_clicked();

private:
    void loadModel();
    void addUpdateFile(QString service, QString filename, QProgressBar *pbar);

    Ui::FilesManagement *ui;

    WSClient *wsClient = nullptr;

    FilesFilterModel *filterModel;
    QStandardItemModel *filesModel;
    QStandardItem *currentItem = nullptr;
    QString fileName;
};

#endif // FILESMANAGEMENT_H
