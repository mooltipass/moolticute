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
    ui->pushButtonDiscard->setStyleSheet(CSS_BLUE_BUTTON);

    filesModel = new QStandardItemModel(this);
    ui->listView->setModel(filesModel);
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
        QStandardItem *item = new QStandardItem(o["service"].toString());
        item->setIcon(AppGui::qtAwesome()->icon(fa::clocko));
        filesModel->appendRow(item);
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
