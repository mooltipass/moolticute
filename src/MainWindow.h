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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include "WSClient.h"
#include <QtAwesome.h>
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
    void enableCredentialsManagement(bool enable);
    void updatePage();
    void checkSettingsChanged();
    void enableKnockSettings(bool visible);
    void updateSerialInfos();
    void wantEnterCredentialManagement();
    void wantSaveCredentialManagement();

//    void mpAdded(MPDevice *device);
//    void mpRemoved(MPDevice *);

    void integrityProgress(int total, int current);
    void integrityFinished(bool success);

    void loadingProgress(int total, int current);

    void on_pushButtonViewLogs_clicked();
    void on_pushButtonAutoStart_clicked();

    void on_checkBoxSSHAgent_stateChanged(int arg1);

    void on_pushButtonExportFile_clicked();
    void on_pushButtonImportFile_clicked();
    void on_pushButtonIntegrity_clicked();

    //Settings page
    void on_pushButtonSettingsReset_clicked();
    void on_pushButtonSettingsSave_clicked();

private:
    void setUIDRequestInstructionsWithId(const QString &id = "XXXX");

    virtual void closeEvent(QCloseEvent *event);

    void checkAutoStart();

    Ui::MainWindow *ui;
    QtAwesome* awesome;

    WSClient *wsClient;

    WindowLog *dialogLog = nullptr;
    QByteArray logBuffer;

    QMovie* gb_spinner;
};

#endif // MAINWINDOW_H
