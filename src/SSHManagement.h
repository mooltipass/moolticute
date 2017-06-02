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
#ifndef SSHMANAGEMENT_H
#define SSHMANAGEMENT_H

#include <QtWidgets>
#include <QtCore>
#include "WSClient.h"

namespace Ui {
class SSHManagement;
}

class SSHManagement : public QWidget
{
    Q_OBJECT

public:
    explicit SSHManagement(QWidget *parent = 0);
    ~SSHManagement();

    void setWsClient(WSClient *c);

private slots:
    void readStdOutLoadKeys();
    void progressChanged(int total, int current);
    void onExportPublicKey();
    void onExportPrivateKey();
    void onServiceExists(const QString service, bool exists);

    void on_pushButtonUnlock_clicked();
    void on_buttonDiscard_clicked();
    void on_buttonSaveChanges_clicked();
    void on_pushButtonImport_clicked();

private:
    Ui::SSHManagement *ui;

    QProcess *sshProcess = nullptr;
    bool loaded = false;

    enum {
        RolePublicKey = Qt::UserRole + 1,
        RolePrivateKey,
    };

    QStandardItemModel *keysModel;

    WSClient *wsClient = nullptr;
};

#endif // SSHMANAGEMENT_H
