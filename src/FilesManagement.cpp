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

FilesFilterModel::FilesFilterModel(QObject *parent):
    QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

void FilesFilterModel::setFilter(const QString &filter_str)
{
    filter = filter_str.toLower();
    invalidate();
}

int FilesFilterModel::indexToSource(int idx)
{
    return mapToSource(index(idx, 0)).row();
}

int FilesFilterModel::indexFromSource(int idx)
{
    return mapFromSource(index(idx, 0)).row();
}

bool FilesFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)

    //shortcut
    if (filter.isEmpty())
        return true;

    QStandardItemModel *filesModel = qobject_cast<QStandardItemModel *>(sourceModel());
    QStandardItem *item = filesModel->item(source_row);

    if (item->text().toLower().contains(filter.toLower()))
        return true;

    return false;
}

bool FilesFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QStandardItemModel *filesModel = qobject_cast<QStandardItemModel *>(sourceModel());
    QStandardItem *itemLeft = filesModel->item(left.row());
    QStandardItem *itemRight = filesModel->item(right.row());

    return itemLeft->text() < itemRight->text();
}

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
    ui->buttonQuitMMM->setIcon(AppGui::qtAwesome()->icon(fa::signout, whiteButtons));
    ui->pushButtonSaveFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSaveFile->setIcon(AppGui::qtAwesome()->icon(fa::floppyo, whiteButtons));
    ui->pushButtonUpdateFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonUpdateFile->setIcon(AppGui::qtAwesome()->icon(fa::pencil, whiteButtons));
    ui->pushButtonDelFile->setStyleSheet(CSS_GREY_BUTTON);
    ui->pushButtonDelFile->setIcon(AppGui::qtAwesome()->icon(fa::trash, whiteButtons));

    filterModel = new FilesFilterModel(this);
    filesModel = new QStandardItemModel(this);
    filterModel->setSourceModel(filesModel);
    ui->filesListView->setModel(filterModel);

    ui->progressBar->hide();
    ui->progressBarTop->hide();

    connect(ui->lineEditFilterFiles, &QLineEdit::textChanged, [=](const QString &t)
    {
        filterModel->setFilter(t);
    });

    connect(ui->filesListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FilesManagement::currentSelectionChanged);
    currentSelectionChanged(QModelIndex(), QModelIndex());
    connect(ui->listFilesButton, &QPushButton::clicked, [=](){
        if (wsClient)
            wsClient->sendRefreshFilesCacheRequest();
    });

    ui->filesCacheListWidget->setVisible(false);
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
        loadModel();
        ui->lineEditFilterFiles->clear();
    });
    connect(wsClient, &WSClient::filesCacheChanged, [=]()
    {
        loadFilesCacheModel();
    });
    connect(wsClient, &WSClient::wsConnected, [=] () {
        wsClient->sendListFilesCacheRequest();
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
    wsClient->sendEnterMMRequest(true);
    emit wantEnterMemMode();
}

void FilesManagement::on_buttonQuitMMM_clicked()
{
    filesModel->clear();
    if (!deletedList.isEmpty())
    {
        emit wantExitMemMode();
        wsClient->deleteDataFilesAndLeave(deletedList);
    }
    else
        wsClient->sendLeaveMMRequest();
}

void FilesManagement::loadModel()
{
    filesModel->clear();
    QJsonArray jarr = wsClient->getMemoryData()["data_nodes"].toArray();
    for (int i = 0;i < jarr.count();i++)
    {
        QJsonObject o = jarr.at(i).toObject();
        QStandardItem *item = new QStandardItem(o["service"].toString());
        item->setIcon(AppGui::qtAwesome()->icon(fa::fileo));
        filesModel->appendRow(item);
    }
    deletedList.clear();
}

void FilesManagement::loadFilesCacheModel()
{
    QListWidget * listWidget = ui->filesCacheListWidget;
    listWidget->clear();
    for (auto entry : wsClient->getFilesCache())
    {

        QWidget* w = new QWidget();
        QHBoxLayout *rowLayout = new QHBoxLayout(listWidget);
        QLabel *icon = new QLabel(listWidget);
        icon->setPixmap(AppGui::qtAwesome()->icon(fa::fileo).pixmap(18, 18));
        rowLayout->addWidget(icon);
        rowLayout->addWidget(new QLabel(entry.toString()));

        QToolButton *button = new QToolButton;
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIcon(AppGui::qtAwesome()->icon(fa::save));

        connect(button, &QToolButton::clicked, [=]() {
            fileName = QFileDialog::getSaveFileName(this, tr("Save to file..."), entry.toString());

            if (fileName.isEmpty())
                return;

//            ui->progressBar->setMinimum(0);
//            ui->progressBar->setMaximum(0);
//            ui->progressBar->setValue(0);
//            ui->progressBar->show();
//            updateButtonsUI();

            connect(wsClient, &WSClient::dataFileRequested, this, &FilesManagement::dataFileRequested);
//            connect(wsClient, &WSClient::progressChanged, this, &FilesManagement::updateProgress);

            wsClient->requestDataFile(entry.toString());
        });

        rowLayout->addWidget(button, 1, Qt::AlignRight);
        rowLayout->setSizeConstraint( QLayout::SetMinAndMaxSize );
        rowLayout->setMargin(0);
        rowLayout->setContentsMargins(6,1,4,1);
        w->setLayout(rowLayout);


        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint( w->sizeHint() );
        listWidget->addItem(item);
        listWidget->setItemWidget(item,w);
    }

    listWidget->setVisible(listWidget->count() > 0);
    ui->listFilesButton->setVisible(listWidget->count() <= 0);
}

void FilesManagement::currentSelectionChanged(const QModelIndex &curr, const QModelIndex &)
{
    if (!curr.isValid())
    {
        ui->filesDisplayFrame->hide();
        currentItem = nullptr;
        return;
    }

    ui->filesDisplayFrame->show();

    currentItem = filesModel->itemFromIndex(filesModel->index(filterModel->indexToSource(curr.row()), 0));

    ui->fileDisplayServiceInput->setText(currentItem->text());
}

void FilesManagement::on_pushButtonUpdateFile_clicked()
{
    if (!currentItem)
        return;

    fileName = QFileDialog::getOpenFileName(this, tr("Load file to device..."));

    if (fileName.isEmpty())
        return;

    addUpdateFile(currentItem->text(), fileName, ui->progressBar);
}

void FilesManagement::on_pushButtonSaveFile_clicked()
{
    if (!currentItem)
        return;

    fileName = QFileDialog::getSaveFileName(this, tr("Save to file..."));

    if (fileName.isEmpty())
        return;

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->progressBar->setValue(0);
    ui->progressBar->show();
    updateButtonsUI();

    connect(wsClient, &WSClient::dataFileRequested, this, &FilesManagement::dataFileRequested);
    connect(wsClient, &WSClient::progressChanged, this, &FilesManagement::updateProgress);

    wsClient->requestDataFile(currentItem->text());
}

void FilesManagement::on_pushButtonDelFile_clicked()
{
    if (!currentItem)
        return;

    if (QMessageBox::question(this, "Moolticute",
                              tr("The file \"%1\" is going to be removed from the device.\nContinue?")
                              .arg(currentItem->text())) != QMessageBox::Yes)
    {
        return;
    }

    deletedList.append(currentItem->text());
    filesModel->removeRow(currentItem->row());
}

void FilesManagement::dataFileRequested(const QString &service, const QByteArray &data, bool success)
{
    Q_UNUSED(service)
    disconnect(wsClient, &WSClient::dataFileRequested, this, &FilesManagement::dataFileRequested);
    disconnect(wsClient, &WSClient::progressChanged, this, &FilesManagement::updateProgress);
    ui->progressBar->hide();
    updateButtonsUI();

    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Data sending was denied for '%1'!").arg(currentItem->text()));
        return;
    }

    QFile f(fileName);
    if (!f.open(QFile::ReadWrite))
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to open file '%1' for write!").arg(fileName));
        return;
    }

    f.write(data);
}

void FilesManagement::updateProgress(int total, int curr)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(curr);
    ui->progressBarTop->setMaximum(total);
    ui->progressBarTop->setValue(curr);
}

void FilesManagement::updateButtonsUI()
{
    bool vis = ui->progressBar->isVisible() ||
               ui->progressBarTop->isVisible();

    ui->buttonQuitMMM->setEnabled(!vis);
    ui->pushButtonDelFile->setEnabled(!vis);
    ui->pushButtonSaveFile->setEnabled(!vis);
    ui->pushButtonUpdateFile->setEnabled(!vis);
    ui->lineEditFilterFiles->setEnabled(!vis);
    ui->filesListView->setEnabled(!vis);
}

void FilesManagement::addUpdateFile(QString service, QString filename, QProgressBar *pbar)
{
    Q_UNUSED(service)

    qint64 maxSize = MP_MAX_FILE_SIZE;
    if (service == MC_SSH_SERVICE)
        maxSize = MP_MAX_SSH_SIZE;
    QFileInfo fi(filename);
    if (fi.size() > maxSize)
    {
        QMessageBox::warning(this, tr("Failure"), tr("File '%1' is too big to be stored in the Mooltipass!").arg(filename));
        return;
    }

    QFile f(filename);
    if (!f.open(QFile::ReadWrite))
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to open file '%1' for read!").arg(filename));
        return;
    }
    QByteArray b = f.readAll();

    pbar->setMinimum(0);
    pbar->setMaximum(0);
    pbar->setValue(0);
    pbar->show();
    updateButtonsUI();

    connect(wsClient, &WSClient::dataFileSent, this, &FilesManagement::dataFileSent);
    connect(wsClient, &WSClient::progressChanged, this, &FilesManagement::updateProgress);

    if (deletedList.contains(service))
        deletedList.removeOne(service);

    wsClient->sendDataFile(service, b);
}

void FilesManagement::dataFileSent(const QString &service, bool success)
{
    Q_UNUSED(service)
    disconnect(wsClient, &WSClient::dataFileSent, this, &FilesManagement::dataFileSent);
    disconnect(wsClient, &WSClient::progressChanged, this, &FilesManagement::updateProgress);
    ui->progressBar->hide();
    ui->progressBarTop->hide();
    updateButtonsUI();
    ui->lineEditFilename->clear();
    ui->addFileServiceInput->clear();

    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to send data!"));
        return;
    }

    //item already exists
    for (int i = 0;i < filesModel->rowCount();i++)
    {
        QStandardItem *it = filesModel->item(i);
        if (it->text() == service)
            return;
    }

    //if not, add the new item
    QStandardItem *item = new QStandardItem(service);
    item->setIcon(AppGui::qtAwesome()->icon(fa::fileo));
    filesModel->appendRow(item);
}

void FilesManagement::on_addFileButton_clicked()
{
    if (ui->addFileServiceInput->text().isEmpty() ||
        ui->lineEditFilename->text().isEmpty())
        return;

    addUpdateFile(ui->addFileServiceInput->text(),
                  ui->lineEditFilename->text(),
                  ui->progressBarTop);
}

void FilesManagement::on_pushButtonFilename_clicked()
{
    fileName = QFileDialog::getOpenFileName(this, tr("Load file to device..."));

    if (fileName.isEmpty())
        return;

    ui->lineEditFilename->setText(fileName);
}
