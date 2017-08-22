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
#include "CredentialModel.h"
#include "CredentialModelFilter.h"
#include "ui_CredentialsManagement.h"
#include "Common.h"
#include "AppGui.h"
#include "TreeItem.h"
#include "LoginItem.h"
#include "ServiceItem.h"

CredentialsManagement::CredentialsManagement(QWidget *parent) :
    QWidget(parent), ui(new Ui::CredentialsManagement), m_pAddedLoginItem(nullptr)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->pageLocked);

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

    ui->pushButtonEnterMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->addCredentialButton->setStyleSheet(CSS_BLUE_BUTTON);
    ui->buttonDiscard->setDefaultText(tr("Discard all changes"));
    ui->buttonDiscard->setPressAndHoldText(tr("Hold to discard all changes"));
    connect(ui->buttonDiscard, &AnimatedColorButton::actionValidated, this, &CredentialsManagement::on_buttonDiscard_clicked);

    ui->buttonSaveChanges->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterMMM->setIcon(AppGui::qtAwesome()->icon(fa::unlock, whiteButtons));
    ui->pushButtonConfirm->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonCancel->setDefaultText(tr("Discard changes"));
    ui->pushButtonCancel->setFixedWidth(108);
    ui->pushButtonCancel->setPressAndHoldText(tr("Hold to discard"));
    connect(ui->pushButtonCancel, &AnimatedColorButton::actionValidated, this, &CredentialsManagement::on_pushButtonCancel_clicked);

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

    connect(m_pCredModel, &CredentialModel::modelLoaded, ui->credentialTreeView, &CredentialView::onModelLoaded);
    connect(m_pCredModel, &CredentialModel::modelLoaded, this, &CredentialsManagement::onModelLoaded);
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
    connect(ui->addCredPasswordInput, &QLineEdit::returnPressed, this, &CredentialsManagement::onPasswordInputReturnPressed);
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

    connect(ui->credentialTreeView, &CredentialView::expanded, this, &CredentialsManagement::onItemExpanded);
    connect(ui->credentialTreeView, &CredentialView::collapsed, this, &CredentialsManagement::onItemCollapsed);
    connect(ui->pushButtonExpandAll, &QPushButton::clicked, ui->credentialTreeView, &CredentialView::onChangeExpandedState);
    connect(ui->credentialTreeView, &CredentialView::expandedStateChanged, this, &CredentialsManagement::onExpandedStateChanged);
    connect(m_pCredModel, &CredentialModel::selectLoginItem, this, &CredentialsManagement::onSelectLoginItem, Qt::QueuedConnection);

    m_tSelectLoginTimer.setInterval(50);
    m_tSelectLoginTimer.setSingleShot(true);
    connect(&m_tSelectLoginTimer, &QTimer::timeout, this, &CredentialsManagement::onSelectLoginTimerTimeOut);
}

CredentialsManagement::~CredentialsManagement()
{
    delete ui;
}

void CredentialsManagement::setWsClient(WSClient *c)
{
    wsClient = c;
    connect(wsClient, &WSClient::memMgmtModeChanged, this, &CredentialsManagement::enableCredentialsManagement);
    connect(wsClient, &WSClient::memoryDataChanged, [=]()
    {
        m_pCredModel->load(wsClient->getMemoryData()["login_nodes"].toArray());
        ui->lineEditFilterCred->clear();
    });
    connect(wsClient, &WSClient::passwordUnlocked, this, &CredentialsManagement::onPasswordUnlocked);
}

void CredentialsManagement::setPasswordProfilesModel(PasswordProfilesModel *passwordProfilesModel)
{
    ui->addCredPasswordInput->setPasswordProfilesModel(passwordProfilesModel);
}

void CredentialsManagement::enableCredentialsManagement(bool enable)
{
    if (enable)
        ui->stackedWidget->setCurrentWidget(ui->pageUnlocked);
    else
        ui->stackedWidget->setCurrentWidget(ui->pageLocked);

    ui->pageUnlocked->setFocus();
}

void CredentialsManagement::updateQuickAddCredentialsButtonState()
{
    ui->addCredentialButton->setEnabled(ui->addCredLoginInput->hasAcceptableInput() && ui->addCredLoginInput->text().length() > 0 &&
                                        ui->addCredServiceInput->hasAcceptableInput() && ui->addCredServiceInput->text().length() > 0 &&
                                        ui->addCredPasswordInput->hasAcceptableInput() && ui->addCredPasswordInput->text().length() > 0);
}

void CredentialsManagement::onPasswordInputReturnPressed()
{
    on_addCredentialButton_clicked();
    ui->addCredServiceInput->setFocus();
}

void CredentialsManagement::on_pushButtonEnterMMM_clicked()
{
    wsClient->sendEnterMMRequest();
    emit wantEnterMemMode();
}

void CredentialsManagement::on_buttonDiscard_clicked()
{
    if (!confirmDiscardUneditedCredentialChanges())
        return;
    wsClient->sendLeaveMMRequest();
    m_pCredModel->clear();
}

void CredentialsManagement::on_buttonSaveChanges_clicked()
{
    qDebug() << m_pCredModel->getJsonChanges();
    wsClient->sendCredentialsMM(m_pCredModel->getJsonChanges());
    emit wantSaveMemMode(); //waits for the daemon to process the data
    m_pCredModel->clear();
}

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
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(lIndexes.first());

    // Do we have a login item?
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);

    // Request password
    if (pLoginItem != nullptr)
    {
        // Retrieve parent
        TreeItem *pItem = pLoginItem->parentItem();
        if (pItem != nullptr) {
            wsClient->requestPassword(pItem->name(), pLoginItem->name());
            ui->credDisplayPasswordInput->setPlaceholderText("Please Approve On Device");
        }
    }
}

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
            if (success)
                QMessageBox::information(this, tr("Moolticute"), tr("New credential %1/%2 added successfully.").arg(service, login));
            else
                QMessageBox::warning(this, tr("Failure"), tr("Couldn't Add New Credential to Device"));

            ui->addCredServiceInput->clear();
            ui->addCredLoginInput->clear();
            ui->addCredPasswordInput->clear();
        });
    }
}

void CredentialsManagement::onPasswordUnlocked(const QString & service, const QString & login,
                                               const QString & password, bool success)
{
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to query password!"));
        return;
    }

    m_pCredModel->setClearTextPassword(service, login, password);
    ui->credDisplayPasswordInput->setText(password);
    ui->credDisplayPasswordInput->setLocked(false);
}

void CredentialsManagement::onCredentialUpdated(const QString & service, const QString & login, const QString & description, bool success)
{
    Q_UNUSED(description)
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to modify %1/%2").arg(service, login));
        return;
    }
}

void CredentialsManagement::saveSelectedCredential(const QModelIndex &proxyIndex)
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    if (!srcIndex.isValid())
    {
        QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
        if (lIndexes.size() != 1)
            return;
        srcIndex = getSourceIndexFromProxyIndex(lIndexes.first());
    }

    // Do we have a login item?
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
    if (pLoginItem != nullptr) {
        m_pCredModel->updateLoginItem(srcIndex, ui->credDisplayPasswordInput->text(), ui->credDisplayDescriptionInput->text(), ui->credDisplayLoginInput->text());
        ui->credentialTreeView->refreshLoginItem(srcIndex);
    }
}

bool CredentialsManagement::confirmDiscardUneditedCredentialChanges(const QModelIndex &proxyIndex)
{   
    if (ui->stackedWidget->currentWidget() != ui->pageUnlocked || !wsClient->get_memMgmtMode() || deletingCred)
        return true;

    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    if (!srcIndex.isValid())
    {
        QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
        QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
        if (lIndexes.size() != 1)
            return true;

        srcIndex = getSourceIndexFromProxyIndex(lIndexes.first());
    }

    // Retrieve login item
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);

    if (pLoginItem != nullptr)
    {
        // Retrieve parent item
        TreeItem *pServiceItem = pLoginItem->parentItem();
        if (pServiceItem != nullptr)
        {
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

void CredentialsManagement::on_pushButtonCancel_clicked()
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve selected indexes
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() != 1)
        return;

    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(lIndexes.first());

    // Retrieve login item
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
    if (pLoginItem != nullptr)
    {
        ui->credDisplayPasswordInput->setText(pLoginItem->password());
        ui->credDisplayDescriptionInput->setText(pLoginItem->description());
        ui->credDisplayLoginInput->setText(pLoginItem->name());
        updateSaveDiscardState();
    }
}

void CredentialsManagement::updateSaveDiscardState(const QModelIndex &proxyIndex)
{
    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    if (!srcIndex.isValid())
    {
        QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
        QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
        if (lIndexes.size() != 1)
        {
            ui->pushButtonCancel->hide();
            ui->pushButtonConfirm->hide();
        }
        else srcIndex = getSourceIndexFromProxyIndex(lIndexes.first());
    }

    if (!srcIndex.isValid())
    {
        ui->pushButtonCancel->hide();
        ui->pushButtonConfirm->hide();
    }
    else
    {
        // Do we have a login item?
        LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);

        if (pLoginItem != nullptr)
        {
            // Retrieve item info
            QString sPassword = ui->credDisplayPasswordInput->text();
            QString sDescription = ui->credDisplayDescriptionInput->text();
            QString sLogin = ui->credDisplayLoginInput->text();

            bool bPasswordCondition = !sPassword.isEmpty() && (sPassword != pLoginItem->password());
            bool bDescriptionCondition = !sDescription.isEmpty() && (sDescription != pLoginItem->description());
            bool bLoginCondition = !sLogin.isEmpty() && (sLogin != pLoginItem->name());
            if (bPasswordCondition || bDescriptionCondition || bLoginCondition)
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
        else
        {
            ui->pushButtonCancel->hide();
            ui->pushButtonConfirm->hide();
        }
    }
}

void CredentialsManagement::changeCurrentFavorite(int iFavorite)
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve selected indexes
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() != 1)
        return;

    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(lIndexes.first());

    // Retrieve login item
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);

    if (pLoginItem != nullptr) {
        m_pCredModel->updateLoginItem(srcIndex, CredentialModel::FavoriteRole, iFavorite);
        ui->credentialTreeView->refreshLoginItem(srcIndex, true);
    }
}

void CredentialsManagement::on_pushButtonDelete_clicked()
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve selected indexes
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() != 1)
        return;

    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(lIndexes.at(0));

    // Retrieve login item
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);

    // Retrieve parent item
    TreeItem *pParentItem = pLoginItem->parentItem();
    if ((pLoginItem != nullptr) && (pParentItem != nullptr))
    {
        // Get service index
        QModelIndex serviceIndex = m_pCredModel->getServiceIndexByName(pLoginItem->parentItem()->name());
        QModelIndex serviceProxyIndex = m_pCredModelFilter->mapFromSource(serviceIndex);
        int iServiceRow = serviceProxyIndex.row();

        // Retrieve parent item
        auto btn = QMessageBox::question(this,
                                         tr("Delete?"),
                                         tr("<i><b>%1</b></i>: Delete credential <i><b>%2</b></i>?").arg(pParentItem->name(), pLoginItem->name()),
                                         QMessageBox::Yes |
                                         QMessageBox::Cancel,
                                         QMessageBox::Yes);
        if (btn == QMessageBox::Yes)
        {
            deletingCred = true;
            bool bSelectNextAvailableLogin = m_pCredModel->removeCredential(srcIndex);
            if (bSelectNextAvailableLogin)
            {
                int nVisibleChilds = m_pCredModelFilter->rowCount(QModelIndex());
                if (nVisibleChilds > 0)
                {
                    if (iServiceRow > (nVisibleChilds-1))
                        iServiceRow = nVisibleChilds-1;
                    serviceIndex = m_pCredModelFilter->index(iServiceRow, 0, QModelIndex());

                    QModelIndex firstLoginIndex = serviceIndex.child(0, 0);
                    if (firstLoginIndex.isValid())
                        ui->credentialTreeView->setCurrentIndex(firstLoginIndex);
                }
            }
            deletingCred = false;
        }
    }
}

void CredentialsManagement::onCredentialSelected(const QModelIndex &proxyIndex, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    if (srcIndex.isValid())
    {
        ServiceItem *pServiceItem = m_pCredModel->getServiceItemByIndex(srcIndex);
        LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
        if (pLoginItem != nullptr)
            emit loginSelected(srcIndex);
        else
        if (pServiceItem != nullptr)
            emit serviceSelected(srcIndex);
        updateSaveDiscardState(proxyIndex);
    }
}

void CredentialsManagement::onLoginSelected(const QModelIndex &srcIndex)
{
    ui->credDisplayFrame->setEnabled(true);
    updateLoginDescription(srcIndex);
}

void CredentialsManagement::onServiceSelected(const QModelIndex &srcIndex)
{
    Q_UNUSED(srcIndex);
    ui->credDisplayFrame->setEnabled(false);
    clearLoginDescription();
}

void CredentialsManagement::updateLoginDescription(const QModelIndex &srcIndex)
{
    // Retrieve login item
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
    updateLoginDescription(pLoginItem);
}

void CredentialsManagement::updateLoginDescription(LoginItem *pLoginItem)
{
    if (pLoginItem != nullptr)
    {
        TreeItem *pParentItem = pLoginItem->parentItem();
        if (pParentItem != nullptr)
        {
            ui->credDisplayServiceInput->setText(pParentItem->name());
            ui->credDisplayLoginInput->setText(pLoginItem->name());
            ui->credDisplayPasswordInput->setText(pLoginItem->password());
            ui->credDisplayDescriptionInput->setText(pLoginItem->description());
            ui->credDisplayCreationDateInput->setText(pLoginItem->createdDate().toString(Qt::DefaultLocaleShortDate));
            ui->credDisplayModificationDateInput->setText(pLoginItem->updatedDate().toString(Qt::DefaultLocaleShortDate));
            ui->credDisplayPasswordInput->setLocked(pLoginItem->passwordLocked());
        }
    }
}

void CredentialsManagement::clearLoginDescription()
{
    ui->credDisplayServiceInput->setText("");
    ui->credDisplayLoginInput->setText("");
    ui->credDisplayPasswordInput->setText("");
    ui->credDisplayDescriptionInput->setText("");
    ui->credDisplayCreationDateInput->setText("");
    ui->credDisplayModificationDateInput->setText("");
}

QModelIndex CredentialsManagement::getSourceIndexFromProxyIndex(const QModelIndex &proxyIndex)
{
    return m_pCredModelFilter->mapToSource(proxyIndex);
}

QModelIndex CredentialsManagement::getProxyIndexFromSourceIndex(const QModelIndex &srcIndex)
{
    return m_pCredModelFilter->mapFromSource(srcIndex);
}

void CredentialsManagement::onItemCollapsed(const QModelIndex &proxyIndex)
{
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    ServiceItem *pServiceItem = m_pCredModel->getServiceItemByIndex(srcIndex);
    if (pServiceItem != nullptr)
        pServiceItem->setExpanded(false);
}

void CredentialsManagement::onExpandedStateChanged(bool bIsExpanded)
{
    ui->pushButtonExpandAll->setText(bIsExpanded ? tr("Collapse All") : tr("Expand All"));
}

void CredentialsManagement::onModelLoaded(bool bClearLoginDescription)
{
    if (bClearLoginDescription)
        clearLoginDescription();
}

void CredentialsManagement::onItemExpanded(const QModelIndex &proxyIndex)
{
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    ServiceItem *pServiceItem = m_pCredModel->getServiceItemByIndex(srcIndex);
    if (pServiceItem != nullptr)
        pServiceItem->setExpanded(true);
}

void CredentialsManagement::onSelectLoginItem(LoginItem *pLoginItem)
{
    if (pLoginItem)
    {
        m_pAddedLoginItem = pLoginItem;
        QModelIndex serviceIndex = m_pCredModel->getServiceIndexByName(pLoginItem->parentItem()->name());
        if (serviceIndex.isValid())
        {
            QModelIndex proxyIndex = getProxyIndexFromSourceIndex(serviceIndex);
            if (proxyIndex.isValid())
                ui->credentialTreeView->expand(proxyIndex);
            m_tSelectLoginTimer.start();
        }
    }
}

void CredentialsManagement::onSelectLoginTimerTimeOut()
{
    if (m_pAddedLoginItem != nullptr)
    {
        QModelIndex serviceIndex = m_pCredModel->getServiceIndexByName(m_pAddedLoginItem->parentItem()->name());
        if (serviceIndex.isValid())
        {
            QModelIndex serviceProxyIndex = getProxyIndexFromSourceIndex(serviceIndex);
            if (serviceProxyIndex.isValid())
            {
                int nVisibleChilds = m_pCredModelFilter->rowCount(serviceProxyIndex);
                for (int i=0; i<nVisibleChilds; i++)
                {
                    QModelIndex loginProxyIndex = m_pCredModelFilter->index(i, 0, serviceProxyIndex);
                    if (loginProxyIndex.isValid())
                    {
                        TreeItem *pItem = m_pCredModelFilter->getItemByProxyIndex(loginProxyIndex);
                        if ((pItem != nullptr) && (pItem->name() == m_pAddedLoginItem->name()))
                            ui->credentialTreeView->setCurrentIndex(loginProxyIndex);
                    }
                }
            }
        }
        m_pAddedLoginItem = nullptr;
    }
}
