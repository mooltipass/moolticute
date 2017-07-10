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
#ifndef CREDENTIALSMANAGEMENT_H
#define CREDENTIALSMANAGEMENT_H

#include <QtWidgets>
#include "WSClient.h"
#include "CredentialsModel.h"

namespace Ui {
class CredentialsManagement;
}

class CredentialsManagement : public QWidget
{
    Q_OBJECT

public:
    explicit CredentialsManagement(QWidget *parent = 0);
    ~CredentialsManagement();

    void setWsClient(WSClient *c);

signals:
    void wantEnterMemMode();
    void wantSaveMemMode();

public slots:
    bool confirmDiscardUneditedCredentialChanges(QModelIndex idx = {});

private slots:
    void enableCredentialsManagement(bool);
    void updateQuickAddCredentialsButtonState();
    void updateSaveDiscardState(QModelIndex idx = {});

    void onPasswordUnlocked(const QString & service, const QString & login, const QString & password, bool success);
    void onCredentialUpdated(const QString & service, const QString & login, const QString & description, bool success);

    void saveSelectedCredential(QModelIndex idx = {});

    void on_pushButtonEnterMMM_clicked();
    void on_buttonDiscard_clicked();
    void on_buttonSaveChanges_clicked();

    void requestPasswordForSelectedItem();
    void on_addCredentialButton_clicked();

    void on_pushButtonConfirm_clicked();
    void on_pushButtonCancel_clicked();

    void on_pushButtonDelete_clicked();

private:
    void changeCurrentFavorite(int fav);

    Ui::CredentialsManagement *ui;

    CredentialsModel *credModel = nullptr;
    CredentialsFilterModel *credFilterModel = nullptr;
    WSClient *wsClient = nullptr;
};

#endif // CREDENTIALSMANAGEMENT_H
