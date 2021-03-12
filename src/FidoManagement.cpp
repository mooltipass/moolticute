#include "FidoManagement.h"
#include "ui_FidoManagement.h"
#include "Common.h"
#include "AppGui.h"
#include "ServiceItem.h"
#include "LoginItem.h"

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

    ui->pushButtonSaveFidoMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExitFidoMMM->setStyleSheet(CSS_BLUE_BUTTON);

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
    connect(wsClient, &WSClient::deleteFidoNodesFailed, this, &FidoManagement::onDeleteFidoNodesFailed);
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
    m_pCredModel->load(wsClient->getMemoryData()["fido_nodes"].toArray(), true);
}

void FidoManagement::enableMemManagement(bool enable)
{
    if (enable)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageUnlocked);
        setFidoManagementClean(true);
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->pageLocked);
    }
}

void FidoManagement::on_pushButtonSaveFidoMMM_clicked()
{
    m_pCredModel->clear();
    if (!deletedList.isEmpty())
    {
        emit wantExitMemMode();
        wsClient->deleteFidoAndLeave(deletedList);
        deletedList.clear();
    }
    else
    {
        wsClient->sendLeaveMMRequest();
    }
}

void FidoManagement::on_pushButtonDiscard_clicked()
{
    deletedList.clear();
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
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->fidoTreeView->selectionModel();

    // Retrieve selected indexes
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() == 0)
        return;

    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(lIndexes.at(0));

    // Retrieve login item
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);

    // Retrieve parent item
    TreeItem *pParentItem = pLoginItem->parentItem();

    if ((pLoginItem != nullptr) && (pParentItem != nullptr))
    {
        // Retrieve parent item
        auto btn = QMessageBox::question(this,
                                         tr("Delete?"),
                                         tr("<i><b>%1</b></i>: Delete credential <i><b>%2</b></i>?").arg(pParentItem->name(), pLoginItem->name()),
                                         QMessageBox::Yes |
                                         QMessageBox::Cancel,
                                         QMessageBox::Yes);
        if (btn == QMessageBox::Yes)
        {
            if (deletedList.isEmpty())
            {
                setFidoManagementClean(false);
            }
            deletedList.append({pParentItem->name(), pLoginItem->name(), pLoginItem->address()});
            QModelIndexList nextRow = m_pCredModelFilter->getNextRow(lIndexes.at(0));
            auto selectionModel = ui->fidoTreeView->selectionModel();
            if (nextRow.size()>0 && nextRow.at(0).isValid())
                selectionModel->setCurrentIndex(nextRow.at(0)
                    , QItemSelectionModel::ClearAndSelect  | QItemSelectionModel::Rows);
            else
                selectionModel->setCurrentIndex(QModelIndex()
                    , QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

            m_pCredModel->removeCredential(srcIndex);
        }
    }
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

void FidoManagement::onDeleteFidoNodesFailed()
{
    QMessageBox::warning(this, tr("Fido Management Mode"),
                         tr("Deleting Fido Credential(s) Failed"));
}

QModelIndex FidoManagement::getSourceIndexFromProxyIndex(const QModelIndex &proxyIndex)
{
    if (proxyIndex.isValid())
    {
        return m_pCredModelFilter->mapToSource(proxyIndex);
    }
    return QModelIndex();
}

QModelIndex FidoManagement::getProxyIndexFromSourceIndex(const QModelIndex &srcIndex)
{
    return m_pCredModelFilter->mapFromSource(srcIndex);
}

void FidoManagement::setFidoManagementClean(bool isClean)
{
    ui->pushButtonExitFidoMMM->setVisible(isClean);
    ui->pushButtonSaveFidoMMM->setVisible(!isClean);
    ui->pushButtonDiscard->setVisible(!isClean);
}

void FidoManagement::on_pushButtonExitFidoMMM_clicked()
{
    wsClient->sendLeaveMMRequest();
}
