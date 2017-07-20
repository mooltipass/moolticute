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

// Application
#include "CredentialsManagement.h"
#include "credentialmodel.h"
#include "credentialmodelfilter.h"
#include "ui_CredentialsManagement.h"
#include "Common.h"
#include "AppGui.h"
#include "treeitem.h"
#include "loginitem.h"
#include "serviceitem.h"

//-------------------------------------------------------------------------------------------------

CredentialsManagement::CredentialsManagement(QWidget *parent) :
    QWidget(parent), ui(new Ui::CredentialsManagement)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->pageLocked);

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

    ui->pushButtonEnterMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->addCredentialButton->setStyleSheet(CSS_BLUE_BUTTON);
    ui->buttonDiscard->setStyleSheet(CSS_GREY_BUTTON);
    ui->buttonSaveChanges->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterMMM->setIcon(AppGui::qtAwesome()->icon(fa::unlock, whiteButtons));
    ui->pushButtonConfirm->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonCancel->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDelete->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDelete->setIcon(AppGui::qtAwesome()->icon(fa::trash, whiteButtons));
    ui->pushButtonFavorite->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonFavorite->setIcon(AppGui::qtAwesome()->icon(fa::star, whiteButtons));

    QMenu *favMenu = new QMenu();
    QAction *action = favMenu->addAction(tr("Not a favorite"));
    connect(action, &QAction::triggered, [this](){ changeCurrentFavorite(Common::FAV_NOT_SET); });
    for (int i = 1;i < 15;i++)
    {
        action = favMenu->addAction(tr("Set as favorite #%1").arg(i));
        connect(action, &QAction::triggered, [this, i](){ changeCurrentFavorite(i - 1); });
    }
    ui->pushButtonFavorite->setMenu(favMenu);

    m_pCredModel = new CredentialModel(this);
    m_pCredModelFilter = new CredentialModelFilter(this);
    m_pCredModelFilter->setSourceModel(m_pCredModel);
    ui->credentialTreeView->setModel(m_pCredModelFilter);

    /*
    delete ui->credentialTreeView->selectionModel();
    ui->credentialTreeView->setSelectionModel(
                new ConditionalItemSelectionModel(ConditionalItemSelectionModel::TestFunction(std::bind(&CredentialsManagement::confirmDiscardUneditedCredentialChanges, this, std::placeholders::_1)),
                                                  m_pCredModelFilter));
    */

    connect(m_pCredModel, &CredentialModel::modelLoaded, ui->credentialTreeView, &CredentialView::onModelLoaded);
    connect(ui->credentialTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &CredentialsManagement::onCredentialSelected);
    connect(this, &CredentialsManagement::loginSelected, this, &CredentialsManagement::onLoginSelected);
    connect(this, &CredentialsManagement::serviceSelected, this, &CredentialsManagement::onServiceSelected);
    connect(ui->credDisplayPasswordInput, &LockedPasswordLineEdit::unlockRequested, this, &CredentialsManagement::requestPasswordForSelectedItem);
    connect(ui->pushButtonConfirm, &QPushButton::clicked, [this](bool)
    {
        saveSelectedCredential();
    });

    connect(ui->addCredServiceInput, &QLineEdit::textChanged, this, &CredentialsManagement::updateQuickAddCredentialsButtonState);
    connect(ui->addCredLoginInput, &QLineEdit::textChanged, this, &CredentialsManagement::updateQuickAddCredentialsButtonState);
    connect(ui->addCredPasswordInput, &QLineEdit::textChanged, this, &CredentialsManagement::updateQuickAddCredentialsButtonState);
    updateQuickAddCredentialsButtonState();

    connect(ui->credDisplayLoginInput, &QLineEdit::textChanged, [this] { updateSaveDiscardState(); });
    connect(ui->credDisplayDescriptionInput, &QLineEdit::textChanged, [this] { updateSaveDiscardState(); });
    connect(ui->credDisplayPasswordInput, &QLineEdit::textChanged, [this] { updateSaveDiscardState(); });
    ui->credDisplayFrame->setEnabled(false);
    updateSaveDiscardState();

    connect(ui->lineEditFilterCred, &QLineEdit::textChanged, [=](const QString &t)
    {
        m_pCredModelFilter->setFilter(t);
    });
}

//-------------------------------------------------------------------------------------------------

CredentialsManagement::~CredentialsManagement()
{
    delete ui;
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::setWsClient(WSClient *c)
{
    wsClient = c;
    connect(wsClient, &WSClient::memMgmtModeChanged, this, &CredentialsManagement::enableCredentialsManagement);
    connect(wsClient, &WSClient::memoryDataChanged, [=]()
    {
        m_pCredModel->load(wsClient->getMemoryData()["login_nodes"].toArray());
        ui->credentialTreeView->expandAll();
        ui->lineEditFilterCred->clear();
    });
    connect(wsClient, &WSClient::passwordUnlocked, this, &CredentialsManagement::onPasswordUnlocked);
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::enableCredentialsManagement(bool enable)
{
    if (enable)
        ui->stackedWidget->setCurrentWidget(ui->pageUnlocked);
    else
        ui->stackedWidget->setCurrentWidget(ui->pageLocked);
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::updateQuickAddCredentialsButtonState()
{
    ui->addCredentialButton->setEnabled(ui->addCredLoginInput->hasAcceptableInput() && ui->addCredLoginInput->text().length() > 0 &&
                                        ui->addCredServiceInput->hasAcceptableInput() && ui->addCredServiceInput->text().length() > 0 &&
                                        ui->addCredPasswordInput->hasAcceptableInput() && ui->addCredPasswordInput->text().length() > 0);
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::on_pushButtonEnterMMM_clicked()
{
    wsClient->sendEnterMMRequest();
    emit wantEnterMemMode();
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::on_buttonDiscard_clicked()
{
    if (!confirmDiscardUneditedCredentialChanges())
        return;
    wsClient->sendLeaveMMRequest();
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::on_buttonSaveChanges_clicked()
{
    wsClient->sendCredentialsMM(m_pCredModel->getJsonChanges());
    emit wantSaveMemMode(); //waits for the daemon to process the data
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::requestPasswordForSelectedItem()
{
    if (!wsClient->get_memMgmtMode())
        return;

    // Get selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() != 1)
        return;

    // Retrieve src index
    QModelIndex srcIndex = m_pCredModelFilter->mapToSource(lIndexes.first());

    // Retrieve item
    TreeItem *pItem = m_pCredModel->getItemByIndex(srcIndex);

    // Do we have a login item?
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);

    // Request password
    if (pLoginItem != nullptr) {
        // Retrieve parent
        TreeItem *pItem = pLoginItem->parentItem();
        if (pItem != nullptr)
            wsClient->requestPassword(pItem->name(), pLoginItem->name());
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::on_addCredentialButton_clicked()
{
    if (wsClient->get_memMgmtMode())
    {
        m_pCredModel->addCredential(ui->addCredServiceInput->text(),
             ui->addCredLoginInput->text(),
             ui->addCredPasswordInput->text(),
             QString());

        ui->addCredServiceInput->clear();
        ui->addCredLoginInput->clear();
        ui->addCredPasswordInput->clear();
    }
    else
    {
        ui->gridLayoutAddCred->setEnabled(false);
        wsClient->addOrUpdateCredential(ui->addCredServiceInput->text(),
                                        ui->addCredLoginInput->text(),
                                        ui->addCredPasswordInput->text());

        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(wsClient, &WSClient::credentialsUpdated, [this, conn](const QString & service, const QString & login, const QString &, bool success)
        {
            disconnect(*conn);
            ui->gridLayoutAddCred->setEnabled(true);
            if (!success)
            {
                QMessageBox::warning(this, tr("Failure"), tr("Unable to set credential %1/%2!").arg(service, login));
                return;
            }

            QMessageBox::information(this, tr("Moolticute"), tr("New credential %1/%2 added successfully.").arg(service, login));
            ui->addCredServiceInput->clear();
            ui->addCredLoginInput->clear();
            ui->addCredPasswordInput->clear();
        });
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::onPasswordUnlocked(const QString & service, const QString & login,
                                               const QString & password, bool success)
{
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to query password!"));
        return;
    }

    m_pCredModel->setClearTextPassword(service, login, password);
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::onCredentialUpdated(const QString & service, const QString & login, const QString & description, bool success)
{
    Q_UNUSED(description)
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to modify %1/%2").arg(service, login));
        return;
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::saveSelectedCredential(const QModelIndex &proxyIndex)
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve src index
    QModelIndex srcIndex = m_pCredModelFilter->mapToSource(proxyIndex);
    if (!srcIndex.isValid())
    {
        QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
        if (lIndexes.size() != 1)
            return;
        srcIndex = m_pCredModelFilter->mapToSource(lIndexes.first());
    }

    // Retrieve item
    TreeItem *pItem = m_pCredModel->getItemByIndex(srcIndex);

    // Do we have a login item?
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
    if (pLoginItem != nullptr)
        m_pCredModel->updateLoginItem(srcIndex, ui->credDisplayPasswordInput->text(), ui->credDisplayDescriptionInput->text(), ui->credDisplayLoginInput->text());
}

//-------------------------------------------------------------------------------------------------

bool CredentialsManagement::confirmDiscardUneditedCredentialChanges(const QModelIndex &proxyIndex)
{   
    if (ui->stackedWidget->currentWidget() != ui->pageUnlocked || !wsClient->get_memMgmtMode() || deletingCred)
        return true;

    // Retrieve src index
    QModelIndex srcIndex = m_pCredModelFilter->mapToSource(proxyIndex);
    if (!srcIndex.isValid())
    {
        // Retrieve selection model
        ConditionalItemSelectionModel *pSelectionModel = dynamic_cast<ConditionalItemSelectionModel *>(ui->credentialTreeView->selectionModel());

        //QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
        QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
        if (lIndexes.size() != 1)
            return true;

        srcIndex = m_pCredModelFilter->mapToSource(lIndexes.first());
    }

    // Retrieve item
    TreeItem *pItem = m_pCredModel->getItemByIndex(srcIndex);

    // Retrieve login item
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);

    if (pLoginItem != nullptr) {
        // Retrieve parent item
        TreeItem *pServiceItem = pLoginItem->parentItem();
        if (pServiceItem != nullptr) {
            // Retrieve UI information
            QString sPassword = ui->credDisplayPasswordInput->text();
            QString sDescription = ui->credDisplayDescriptionInput->text();
            QString sLogin = ui->credDisplayLoginInput->text();

            if ((!sPassword.isEmpty() && (sPassword != pLoginItem->password())) ||
                    (!sDescription.isEmpty() && (sDescription != pLoginItem->description())) ||
                    (!sLogin.isEmpty() && (sLogin != pLoginItem->name())))
            {
                auto btn = QMessageBox::question(this,
                                                 tr("Discard Modifications ?"),
                                                 tr("You have modified %1/%2 - Do you want to discard the modifications ?").arg(pServiceItem->name(), pLoginItem->name()),
                                                 QMessageBox::Discard |
                                                 QMessageBox::Save |
                                                 QMessageBox::Cancel,
                                                 QMessageBox::Cancel);
                if (btn == QMessageBox::Cancel)
                    return false;
                if (btn == QMessageBox::Discard)
                    return true;
                if (btn == QMessageBox::Save)
                {
                    saveSelectedCredential(proxyIndex);
                    return true;
                }
            }
        }
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::on_pushButtonConfirm_clicked()
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() != 1)
        return;

    // Save selected credential
    saveSelectedCredential(lIndexes.first());

    // Update save discard state
    updateSaveDiscardState();
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::on_pushButtonCancel_clicked()
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve selected indexes
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() != 1)
        return;

    // Retrieve src index
    QModelIndex srcIndex = m_pCredModelFilter->mapToSource(lIndexes.first());

    // Retrieve login item
    TreeItem *pItem = m_pCredModel->getItemByIndex(srcIndex);
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
    if (pLoginItem != nullptr) {
        ui->credDisplayPasswordInput->setText(pLoginItem->password());
        ui->credDisplayDescriptionInput->setText(pLoginItem->description());
        ui->credDisplayLoginInput->setText(pLoginItem->name());
        updateSaveDiscardState();
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::updateSaveDiscardState(const QModelIndex &proxyIndex)
{
    // Retrieve src index
    QModelIndex srcIndex = m_pCredModelFilter->mapToSource(proxyIndex);
    if (!srcIndex.isValid())
    {
        QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
        QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
        if (lIndexes.size() != 1)
        {
            ui->pushButtonCancel->hide();
            ui->pushButtonConfirm->hide();
        }
        else srcIndex = m_pCredModelFilter->mapToSource(lIndexes.first());
    }

    if (!srcIndex.isValid())
    {
        ui->pushButtonCancel->hide();
        ui->pushButtonConfirm->hide();
    }
    else
    {
        // Retrieve item
        TreeItem *pItem = m_pCredModel->getItemByIndex(srcIndex);

        // Do we have a login item?
        LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);

        if (pLoginItem != nullptr) {
            // Retrieve item info
            QString sPassword = ui->credDisplayPasswordInput->text();
            QString sDescription = ui->credDisplayDescriptionInput->text();
            QString sLogin = ui->credDisplayLoginInput->text();

            if (sPassword != pLoginItem->password() ||
                    sDescription != pLoginItem->description() ||
                    sLogin != pLoginItem->name())
            {
                ui->pushButtonCancel->show();
                ui->pushButtonConfirm->show();
            }
            else
            {
                ui->pushButtonCancel->hide();
                ui->pushButtonConfirm->hide();
            }
        }
        else {
            ui->pushButtonCancel->hide();
            ui->pushButtonConfirm->hide();
        }
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::changeCurrentFavorite(int iFavorite)
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve selected indexes
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() != 1)
        return;

    // Retrieve src index
    QModelIndex srcIndex = m_pCredModelFilter->mapToSource(lIndexes.first());

    // Retrieve login item
    TreeItem *pItem = m_pCredModel->getItemByIndex(srcIndex);
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);

    if (pLoginItem != nullptr)
        m_pCredModel->updateLoginItem(srcIndex, CredentialModel::FavoriteIdx, iFavorite);
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::on_pushButtonDelete_clicked()
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve selected indexes
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() != 1)
        return;

    // Retrieve src index
    QModelIndex srcIndex = m_pCredModelFilter->mapToSource(lIndexes.at(0));

    // Retrieve login item
    TreeItem *pItem = m_pCredModel->getItemByIndex(srcIndex);
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);

    if (pLoginItem != nullptr) {
        // Retrieve parent item
        TreeItem *pParentItem = pLoginItem->parentItem();
        if (pParentItem != nullptr) {
            auto btn = QMessageBox::question(this,
                                             tr("Delete?"),
                                             tr("Remove the current credential %1/%2 ?").arg(pParentItem->name(), pLoginItem->name()),
                                             QMessageBox::Yes |
                                             QMessageBox::Cancel,
                                             QMessageBox::Yes);
            if (btn == QMessageBox::Yes)
            {
                deletingCred = true;
                m_pCredModel->removeCredential(srcIndex);
                deletingCred = false;
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::onCredentialSelected(const QModelIndex &proxyIndex, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    if (proxyIndex.isValid()) {
        QModelIndex srcIndex = m_pCredModelFilter->mapToSource(proxyIndex);
        if (srcIndex.isValid()) {
            TreeItem *pItem = m_pCredModel->getItemByIndex(srcIndex);

            // A login item was selected
            LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
            if (pLoginItem != nullptr)
                emit loginSelected(pLoginItem->uid());
            else {
                // A service item was selected
                ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pItem);
                if (pServiceItem != nullptr)
                    emit serviceSelected(pServiceItem->uid());
            }
            updateSaveDiscardState(proxyIndex);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::onLoginSelected(const QString &sItemUID)
{
    ui->credDisplayFrame->setEnabled(true);
    updateLoginDescription(sItemUID);
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::onServiceSelected(const QString &sItemUID)
{
    ui->credDisplayFrame->setEnabled(false);
    clearLoginDescription(sItemUID);
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::updateLoginDescription(const QString &sItemUID)
{
    // Retrieve login item
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(m_pCredModel->getItemByUID(sItemUID));
    if (pLoginItem != nullptr) {
        TreeItem *pParentItem = pLoginItem->parentItem();
        if (pParentItem != nullptr) {
            QString sFormat("dd/MM/yyyy");
            ui->credDisplayServiceInput->setText(pParentItem->name());
            ui->credDisplayLoginInput->setText(pLoginItem->name());
            ui->credDisplayPasswordInput->setText(pLoginItem->password());
            ui->credDisplayDescriptionInput->setText(pLoginItem->description());
            ui->credDisplayCreationDateInput->setText(pLoginItem->createdDate().toString(sFormat));
            ui->credDisplayModificationDateInput->setText(pLoginItem->updatedDate().toString(sFormat));
        }
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialsManagement::clearLoginDescription(const QString &sItemUID)
{
    TreeItem *pItem = m_pCredModel->getItemByUID(sItemUID);
    ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pItem);
    if (pServiceItem != nullptr)
        ui->credDisplayServiceInput->setText(pServiceItem->name());
    ui->credDisplayLoginInput->setText("");
    ui->credDisplayPasswordInput->setText("");
    ui->credDisplayDescriptionInput->setText("");
    ui->credDisplayCreationDateInput->setText("");
    ui->credDisplayModificationDateInput->setText("");
}
