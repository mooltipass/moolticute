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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include "WSClient.h"
#include <QtAwesome.h>
#include "CredentialsModel.h"
#include "WindowLog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(WSClient *client, QWidget *parent = 0);
    ~MainWindow();

    void daemonLogAppend(const QByteArray &logdata);

    bool isHttpDebugChecked();

signals:
    void windowCloseRequested();

private slots:
    void updatePage();
    void checkSettingsChanged();
    void memMgmtMode();

//    void mpAdded(MPDevice *device);
//    void mpRemoved(MPDevice *);

    void askPasswordDone(bool success, const QString &pass);

    void on_pushButtonSettingsReset_clicked();
    void on_pushButtonSettingsSave_clicked();
    void on_pushButtonMemMode_clicked();
    void on_pushButtonExitMMM_clicked();
    void on_pushButtonShowPass_clicked();
    void on_pushButtonCredAdd_clicked();
    void on_pushButtonCredEdit_clicked();
    void on_pushButtonQuickAddCred_clicked();
    void on_pushButtonViewLogs_clicked();
    void on_pushButtonAutoStart_clicked();

private:

    virtual void closeEvent(QCloseEvent *event);

    void checkAutoStart();

    Ui::MainWindow *ui;
    QtAwesome* awesome;

    WSClient *wsClient;

    CredentialsModel *credModel;
    CredentialsFilterModel *credFilterModel;

    QStandardItem *passItem = nullptr;

    bool editCredAsked = false;

    WindowLog *dialogLog = nullptr;
    QByteArray logBuffer;

    enum
    {
        PAGE_NO_CONNECTION = 0,
        PAGE_SETTINGS = 1,
        PAGE_CREDENTIALS_ENABLE = 2,
        PAGE_SYNC = 3,
        PAGE_NO_CARD = 4,
        PAGE_LOCKED = 5,
        PAGE_CREDENTIALS = 6,
        PAGE_WAIT_CONFIRM = 7,
        PAGE_ABOUT = 8,
        PAGE_MC_SETTINGS = 9,
    };
};

#endif // MAINWINDOW_H
