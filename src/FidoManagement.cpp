#include "FidoManagement.h"
#include "ui_FidoManagement.h"
#include "Common.h"
#include "AppGui.h"

FidoManagement::FidoManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FidoManagement)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->pageLocked);

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

    ui->pushButtonEnterFido->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterFido->setIcon(AppGui::qtAwesome()->icon(fa::unlock, whiteButtons));

    ui->pushButtonSaveExitFidoMMM->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonDelete->setStyleSheet(CSS_GREY_BUTTON);
    ui->pushButtonDelete->setIcon(AppGui::qtAwesome()->icon(fa::trash, whiteButtons));

    filesModel = new QStandardItemModel(this);
    ui->listView->setModel(filesModel);

    connect(ui->pushButtonDiscard, &AnimatedColorButton::actionValidated, this, &FidoManagement::on_pushButtonDiscard_clicked);
}

void FidoManagement::setWsClient(WSClient *c)
{
    wsClient = c;
    connect(wsClient, &WSClient::memoryDataChanged, this, &FidoManagement::loadModel);
    connect(wsClient, &WSClient::memMgmtModeChanged, this, &FidoManagement::enableMemManagement);
}

FidoManagement::~FidoManagement()
{
    delete ui;
}

void FidoManagement::on_pushButtonEnterFido_clicked()
{
    wsClient->sendEnterMMRequest(false, true);
    emit wantEnterMemMode();
}

void FidoManagement::loadModel()
{
    qCritical() << "Loading fido models";
    filesModel->clear();
    QJsonArray fidoArr = wsClient->getMemoryData()["fido_nodes"].toArray();
    if (fidoArr.isEmpty())
    {
        return;
    }
    for (int i = 0; i < fidoArr.count(); i++)
    {
        QJsonObject o = fidoArr.at(i).toObject();
        const auto service = o["service"].toString();
        auto childArray = o["childs"].toArray();
        for (int j = 0; j < childArray.count(); ++j)
        {
            QStandardItem *item = new QStandardItem();
            QString login = childArray.at(j)["login"].toString();
            item->setText(service + " - " + login);
            item->setIcon(AppGui::qtAwesome()->icon(fa::usb));
            filesModel->appendRow(item);
        }
    }

    // Select first item by default
    auto selectionModel = ui->listView->selectionModel();
    auto index = filesModel->index(0,0);
    selectionModel->select(index, QItemSelectionModel::ClearAndSelect);
    //currentSelectionChanged(index, QModelIndex());
}

void FidoManagement::enableMemManagement(bool enable)
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

void FidoManagement::on_pushButtonSaveExitFidoMMM_clicked()
{
    filesModel->clear();
//    if (!deletedList.isEmpty())
//    {
//        emit wantExitMemMode();
//        wsClient->deleteDataFilesAndLeave(deletedList);
//    }
//    else
     wsClient->sendLeaveMMRequest();
}

void FidoManagement::on_pushButtonDiscard_clicked()
{
    wsClient->sendLeaveMMRequest();
}

void FidoManagement::on_pushButtonDiscard_pressed()
{
    if (deletedList.isEmpty())
    {
        wsClient->sendLeaveMMRequest();
    }
}

void FidoManagement::on_pushButtonDelete_clicked()
{
    auto selectionModel = ui->listView->selectionModel();
    auto selectedIndexes = selectionModel->selectedIndexes();

    if (selectedIndexes.length() <= 0)
        return;

    auto selectedIndex = selectedIndexes.first();

    currentItem = filesModel->itemFromIndex(selectedIndex);

    if (!currentItem)
        return;

    if (QMessageBox::question(this, "Moolticute",
                              tr("\"%1\" fido credential is going to be removed from the device.\nContinue?")
                              .arg(currentItem->text())) != QMessageBox::Yes)
    {
        return;
    }

    deletedList.append(currentItem->text());
    filesModel->removeRow(currentItem->row());

    // Select another item
    auto index = filesModel->index(std::min(selectedIndex.row(), filesModel->rowCount()-1),0);
    selectionModel->select(index, QItemSelectionModel::ClearAndSelect);
    //currentSelectionChanged(index, QModelIndex());
}
