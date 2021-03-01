#include "FidoManagement.h"
#include "ui_FidoManagement.h"
#include "Common.h"
#include "AppGui.h"
#include "ServiceItem.h"

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

    m_pCredModel = new CredentialModel(this);

    m_pCredModelFilter = new CredentialModelFilter(this);
    m_pCredModelFilter->setSourceModel(m_pCredModel);
    ui->fidoTreeView->setModel(m_pCredModelFilter);

    connect(m_pCredModel, &CredentialModel::modelLoaded, ui->fidoTreeView, &CredentialView::onModelLoaded);
    connect(ui->fidoTreeView, &CredentialView::expanded, this, &FidoManagement::onItemExpanded);
    connect(ui->fidoTreeView, &CredentialView::collapsed, this, &FidoManagement::onItemCollapsed);

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
    m_pCredModel->load(wsClient->getMemoryData()["fido_nodes"].toArray(), true);
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
    m_pCredModel->clear();
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
// TODO implement delete with treeview
//    auto selectionModel = ui->fidoTreeView->selectionModel();
//    auto selectedIndexes = selectionModel->selectedIndexes();

//    if (selectedIndexes.length() <= 0)
//        return;

//    auto selectedIndex = selectedIndexes.first();

//    currentItem = filesModel->itemFromIndex(selectedIndex);

//    if (!currentItem)
//        return;

//    if (QMessageBox::question(this, "Moolticute",
//                              tr("\"%1\" fido credential is going to be removed from the device.\nContinue?")
//                              .arg(currentItem->text())) != QMessageBox::Yes)
//    {
//        return;
//    }

//    deletedList.append(currentItem->text());
//    filesModel->removeRow(currentItem->row());

//    // Select another item
//    auto index = filesModel->index(std::min(selectedIndex.row(), filesModel->rowCount()-1),0);
//    selectionModel->select(index, QItemSelectionModel::ClearAndSelect);
    //currentSelectionChanged(index, QModelIndex());
}

void FidoManagement::onItemExpanded(const QModelIndex &proxyIndex)
{
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    ServiceItem *pServiceItem = m_pCredModel->getServiceItemByIndex(srcIndex);
    if (pServiceItem != nullptr)
        pServiceItem->setExpanded(true);
}

void FidoManagement::onItemCollapsed(const QModelIndex &proxyIndex)
{
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    ServiceItem *pServiceItem = m_pCredModel->getServiceItemByIndex(srcIndex);
    if (pServiceItem != nullptr)
        pServiceItem->setExpanded(false);
}

QModelIndex FidoManagement::getSourceIndexFromProxyIndex(const QModelIndex &proxyIndex)
{
    if (proxyIndex.isValid())
        return m_pCredModelFilter->mapToSource(proxyIndex);
    return QModelIndex();
}

QModelIndex FidoManagement::getProxyIndexFromSourceIndex(const QModelIndex &srcIndex)
{
    return m_pCredModelFilter->mapFromSource(srcIndex);
}
