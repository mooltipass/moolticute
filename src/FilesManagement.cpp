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
#include "FilesManagement.h"
#include "ui_FilesManagement.h"
#include "Common.h"
#include "AppGui.h"

FilesManagement::FilesManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FilesManagement)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->pageLocked);

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

    ui->pushButtonEnterMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterMMM->setIcon(AppGui::qtAwesome()->icon(fa::unlock, whiteButtons));
    ui->addFileButton->setStyleSheet(CSS_BLUE_BUTTON);

    ui->buttonQuitMMM->setStyleSheet(CSS_GREY_BUTTON);
    ui->pushButtonSaveFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonUpdateFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDelFile->setStyleSheet(CSS_GREY_BUTTON);
}

FilesManagement::~FilesManagement()
{
    delete ui;
}

void FilesManagement::setWsClient(WSClient *c)
{
    wsClient = c;
    connect(wsClient, &WSClient::memMgmtModeChanged, this, &FilesManagement::enableMemManagement);
    connect(wsClient, &WSClient::memoryDataChanged, [=]()
    {
        //credModel->load(wsClient->getMemoryData()["login_nodes"].toArray());
        ui->lineEditFilterFiles->clear();
    });
}

void FilesManagement::enableMemManagement(bool enable)
{
    if (enable)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageUnlocked);
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->pageLocked);
    }
}

void FilesManagement::on_pushButtonEnterMMM_clicked()
{
    wsClient->sendEnterMMRequest();
    emit wantEnterMemMode();
}

void FilesManagement::on_buttonQuitMMM_clicked()
{
    wsClient->sendLeaveMMRequest();
}
