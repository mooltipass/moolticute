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
#include "DeviceDetector.h"
#include "SettingsGuiBLE.h"
#include "ParseDomain.h"

const QString CredentialsManagement::INVALID_DOMAIN_TEXT =
        tr("The following domains are invalid or private: <b><ul><li>%1</li></ul></b>They are not saved.");
const QString CredentialsManagement::INVALID_INPUT_STYLE =
        "border: 2px solid red";

CredentialsManagement::CredentialsManagement(QWidget *parent) :
    QWidget(parent), ui(new Ui::CredentialsManagement), m_pAddedLoginItem(nullptr)
{
    m_selectionCanceled= false;

    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->pageLocked);

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

    ui->pushButtonEnterMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->addCredentialButton->setStyleSheet(CSS_BLUE_BUTTON);
    ui->buttonDiscard->setText(tr("Discard all changes"));
    connect(ui->buttonDiscard, &AnimatedColorButton::pressed, this, &CredentialsManagement::on_buttonDiscard_pressed);
    connect(ui->buttonDiscard, &AnimatedColorButton::actionValidated, this, &CredentialsManagement::onButtonDiscard_confirmed);

    ui->buttonSaveChanges->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterMMM->setIcon(AppGui::qtAwesome()->icon(fa::unlock, whiteButtons));
    ui->pushButtonConfirm->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonTOTP->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDeleteTOTP->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDiscardLinking->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonLinkTo->setStyleSheet(CSS_BLUE_BUTTON);

    ui->buttonExit->setText(tr("Exit Credential Management"));
    ui->buttonExit->setStyleSheet(CSS_BLUE_BUTTON);
    connect(ui->buttonExit, &QPushButton::clicked, this, &CredentialsManagement::onButtonDiscard_confirmed);

    ui->horizontalLayout_filterCred->removeWidget(ui->toolButtonClearFilter);

    setFilterCredLayout();

    ui->pushButtonCancel->setText(tr("Discard changes"));
    connect(ui->pushButtonCancel, &AnimatedColorButton::actionValidated, this, &CredentialsManagement::on_pushButtonCancel_clicked);

    ui->pushButtonDelete->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDelete->setIcon(AppGui::qtAwesome()->icon(fa::trash, whiteButtons));
    ui->pushButtonFavorite->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonFavorite->setIcon(AppGui::qtAwesome()->icon(fa::star, whiteButtons));
    ui->toolButtonEditService->setStyleSheet(CSS_BLUE_BUTTON);
    ui->toolButtonEditService->setIcon(AppGui::qtAwesome()->icon(fa::edit));
    ui->toolButtonEditService->setToolTip(tr("Edit Service Name"));

    ui->toolButtonTOTPService->setStyleSheet(CSS_BLUE_BUTTON);
    ui->toolButtonTOTPService->setIcon(AppGui::qtAwesome()->icon(fa::clocko));
    ui->toolButtonTOTPService->setToolTip(tr("There is a TOTP Credential for the service"));
    ui->toolButtonTOTPService->setEnabled(false);
    ui->toolButtonTOTPService->hide();

    ui->labelCategory1->setText(tr("Category 1:"));
    ui->labelCategory2->setText(tr("Category 2:"));
    ui->labelCategory3->setText(tr("Category 3:"));
    ui->labelCategory4->setText(tr("Category 4:"));
    const int maxCategoryLength = 32;
    ui->lineEditCategory1->setMaxLength(maxCategoryLength);
    ui->lineEditCategory2->setMaxLength(maxCategoryLength);
    ui->lineEditCategory3->setMaxLength(maxCategoryLength);
    ui->lineEditCategory4->setMaxLength(maxCategoryLength);
    ui->pushButtonSaveCategories->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSaveCategories->setText(tr("Save"));
    ui->pushButtonSaveCategories->hide();
    ui->credDisplayCategoryInput->hide();
    ui->credDisplayCategoryLabel->hide();
    ui->credDisplayKeyAfterLoginInput->hide();
    ui->credDisplayKeyAfterLoginLabel->hide();
    ui->credDisplayKeyAfterPwdInput->hide();
    ui->credDisplayKeyAfterPwdLabel->hide();
    connect(ui->lineEditCategory1, &QLineEdit::textEdited, this, &CredentialsManagement::onCategoryEdited);
    connect(ui->lineEditCategory2, &QLineEdit::textEdited, this, &CredentialsManagement::onCategoryEdited);
    connect(ui->lineEditCategory3, &QLineEdit::textEdited, this, &CredentialsManagement::onCategoryEdited);
    connect(ui->lineEditCategory4, &QLineEdit::textEdited, this, &CredentialsManagement::onCategoryEdited);

    connect(ui->addCredLoginInput, &QLineEdit::textEdited, this, &CredentialsManagement::onLoginEdited);
    connect(ui->addCredPasswordInput, &QLineEdit::textEdited, this, &CredentialsManagement::onPasswordEdited);

    ui->pushButtonFavorite->setMenu(&m_favMenu);

    ui->framePasswordLink->hide();

    m_pCredModel = new CredentialModel(this);

    m_pCredModelFilter = new CredentialModelFilter(this);
    m_pCredModelFilter->setSourceModel(m_pCredModel);
    ui->credentialTreeView->setModel(m_pCredModelFilter);

    for (int i = 0; i < BLE_FAVORITE_NUM/MAX_BLE_CAT_NUM; ++i)
    {
        ui->credDisplayCategoryInput->addItem(m_pCredModel->getCategoryName(i), i);
    }

    initKeyAfterInput(ui->credDisplayKeyAfterLoginInput);
    initKeyAfterInput(ui->credDisplayKeyAfterPwdInput);

    m_pTOTPCred = new TOTPCredential(this);
    connect(m_pTOTPCred, &TOTPCredential::accepted, this, &CredentialsManagement::saveSelectedTOTP);
    connect(this, &CredentialsManagement::loginSelected, m_pTOTPCred, &TOTPCredential::clearFields);

    connect(m_pCredModel, &CredentialModel::modelReset, this, &CredentialsManagement::updateFavMenu);
    connect(m_pCredModel, &CredentialModel::dataChanged, this, &CredentialsManagement::updateFavMenu);

    connect(m_pCredModel, &CredentialModel::modelLoaded, ui->credentialTreeView, &CredentialView::onModelLoaded);
    connect(m_pCredModel, &CredentialModel::modelLoaded, this, &CredentialsManagement::onModelLoaded);
    connect(ui->credentialTreeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &CredentialsManagement::onCredentialSelected);
    connect(this, &CredentialsManagement::loginSelected, this, &CredentialsManagement::onLoginSelected);
    connect(this, &CredentialsManagement::serviceSelected, this, &CredentialsManagement::onServiceSelected);
    connect(ui->credDisplayPasswordInput, &LockedPasswordLineEdit::unlockRequested, this, &CredentialsManagement::requestPasswordForSelectedItem);
    connect(ui->pushButtonConfirm, &QPushButton::clicked, [this](bool)
    {
        // Save selected credential
        saveSelectedCredential();
    });

    connect(&DeviceDetector::instance(), &DeviceDetector::deviceChanged,
            this, &CredentialsManagement::updateFavMenuOnDevChanged);
    connect(&DeviceDetector::instance(), &DeviceDetector::deviceChanged,
            this, &CredentialsManagement::updateDeviceType);

    connect(ui->addCredServiceInput, &QLineEdit::textChanged, this, &CredentialsManagement::updateQuickAddCredentialsButtonState);
    connect(ui->addCredLoginInput, &QLineEdit::textChanged, this, &CredentialsManagement::updateQuickAddCredentialsButtonState);
    connect(ui->addCredPasswordInput, &QLineEdit::textChanged, this, &CredentialsManagement::updateQuickAddCredentialsButtonState);
    connect(ui->addCredPasswordInput, &QLineEdit::returnPressed, this, &CredentialsManagement::onPasswordInputReturnPressed);
    updateQuickAddCredentialsButtonState();

    connect(ui->credDisplayServiceInput, &QLineEdit::textChanged, [this] { updateSaveDiscardState(); });
    connect(ui->credDisplayLoginInput, &QLineEdit::textChanged, this, &CredentialsManagement::onDisplayLoginTextChanged);
    connect(ui->credDisplayDescriptionInput, &QLineEdit::textChanged, [this] { updateSaveDiscardState(); });
    connect(ui->credDisplayPasswordInput, &QLineEdit::textChanged, this, &CredentialsManagement::onDisplayPasswordTextChanged);
    connect(ui->credDisplayCategoryInput, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this]
            {
                ui->pushButtonFavorite->setEnabled(false);
                updateSaveDiscardState();
            });
    connect(ui->credDisplayKeyAfterLoginInput, QOverload<int>::of(&QComboBox::currentIndexChanged), [this] { updateSaveDiscardState(); });
    connect(ui->credDisplayKeyAfterPwdInput, QOverload<int>::of(&QComboBox::currentIndexChanged), [this] { updateSaveDiscardState(); });
    connect(ui->credDisplayServiceInput, &QLineEdit::editingFinished,
        [this] {
            ui->toolButtonEditService->show();
            ui->credDisplayServiceInput->setReadOnly(true);
            ui->credDisplayServiceInput->setFrame(false);
        }
    );
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
    connect(m_pCredModel, &CredentialModel::selectLoginItem, this, &CredentialsManagement::onSelectLoginItem);

    m_tSelectLoginTimer.setInterval(50);
    m_tSelectLoginTimer.setSingleShot(true);
    connect(&m_tSelectLoginTimer, &QTimer::timeout, this, &CredentialsManagement::onSelectLoginTimerTimeOut);

    connect(ui->addCredPasswordInput, &LockedPasswordLineEdit::linkRequested, this, &CredentialsManagement::onCredentialLink);
    connect(ui->addCredPasswordInput, &LockedPasswordLineEdit::linkRemoved, this, &CredentialsManagement::onCredentialLinkRemoved);
    connect(this, &CredentialsManagement::newCredentialLinked, ui->addCredPasswordInput, &LockedPasswordLineEdit::onCredentialLinked);
    connect(this, &CredentialsManagement::displayCredentialLink, ui->addCredPasswordInput, &LockedPasswordLineEdit::onDisplayLink);
    connect(this, &CredentialsManagement::hideCredentialLink, ui->addCredPasswordInput, &LockedPasswordLineEdit::onHideLink);
    connect(ui->credDisplayPasswordInput, &LockedPasswordLineEdit::linkRemoved, this, &CredentialsManagement::onSelectedCredentialLinkRemoved);
    connect(ui->credDisplayPasswordInput, &LockedPasswordLineEdit::linkRequested, this, &CredentialsManagement::onSelectedCredentialLink);
    connect(this, &CredentialsManagement::editedCredentialLinked, ui->credDisplayPasswordInput, &LockedPasswordLineEdit::onCredentialLinked);
    ui->credentialTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->credentialTreeView, &CredentialView::customContextMenuRequested, this, &CredentialsManagement::onTreeViewContextMenuRequested);

#ifdef Q_OS_LINUX
    // Force white background and black text color for CredentialsManagement to make UI visible in dark mode
    ui->credentialsListWdiget->setStyleSheet("QWidget { background-color : white; color : black; }");
    ui->pageUnlocked->setStyleSheet("QLineEdit { background-color : white; color : black; }");
    ui->quickInsertWidget->setStyleSheet("QLineEdit { background-color : white; color : black; }");
#endif
}

void CredentialsManagement::setFilterCredLayout()
{
    ui->toolButtonFavFilter->setIcon(AppGui::qtAwesome()->icon(fa::staro));
    QHBoxLayout *filterLayout = new QHBoxLayout(ui->lineEditFilterCred);
#if QT_VERSION < 0x060000
    // Margin is obsolate property for layout
    filterLayout->setMargin(0);
#else
    filterLayout->setContentsMargins(0,0,0,0);
#endif
    filterLayout->addStretch();
    filterLayout->addWidget(ui->toolButtonClearFilter);

    ui->toolButtonClearFilter->setIcon(AppGui::qtAwesome()->icon(fa::times));
    ui->toolButtonClearFilter->setVisible(false);
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
        m_loadedModelSerialiation = m_pCredModel->getJsonChanges();
        ui->lineEditFilterCred->clear();
    });
    connect(wsClient, &WSClient::passwordUnlocked, this, &CredentialsManagement::onPasswordUnlocked);
    connect(wsClient, &WSClient::mpHwVersionChanged, this, &CredentialsManagement::checkDeviceType);
    connect(wsClient, &WSClient::memMgmtModeChanged, this, &CredentialsManagement::checkDeviceType);
    connect(wsClient, &WSClient::advancedMenuChanged, this, &CredentialsManagement::checkDeviceType);
    connect(wsClient, &WSClient::deviceConnected, this, &CredentialsManagement::checkDeviceType);
    connect(wsClient, &WSClient::advancedMenuChanged, this, &CredentialsManagement::handleAdvancedModeChange);
    handleAdvancedModeChange(wsClient->get_advancedMenu());
    connect(wsClient, &WSClient::displayUserCategories, this,
             [this](const QString& cat1, const QString& cat2, const QString& cat3, const QString& cat4)
                {
                    ui->lineEditCategory1->setText(cat1);
                    ui->lineEditCategory2->setText(cat2);
                    ui->lineEditCategory3->setText(cat3);
                    ui->lineEditCategory4->setText(cat4);
                    ui->credDisplayCategoryInput->setItemText(1, cat1);
                    ui->credDisplayCategoryInput->setItemText(2, cat2);
                    ui->credDisplayCategoryInput->setItemText(3, cat3);
                    ui->credDisplayCategoryInput->setItemText(4, cat4);
                    m_pCredModel->updateCategories(cat1, cat2, cat3, cat4);
                }
    );
    connect(wsClient, &WSClient::statusChanged, this,
             [this](Common::MPStatus status)
                {
                    if (!wsClient->isMPBLE())
                    {
                        return;
                    }

                    if (Common::MPStatus::Unlocked == status && wsClient->get_advancedMenu())
                    {
                        sendGetUserCategories();
                    }
                    else if (Common::MPStatus::NoCardInserted == status)
                    {
                        m_pCredModel->setUserCategoryClean(false);
                    }
                }
    );
    connect(wsClient, &WSClient::deviceDisconnected,
             [this](){ m_pCredModel->setUserCategoryClean(false);});
}

void CredentialsManagement::setPasswordProfilesModel(PasswordProfilesModel *passwordProfilesModel)
{
    ui->addCredPasswordInput->setPasswordProfilesModel(passwordProfilesModel);
    ui->credDisplayPasswordInput->setPasswordProfilesModel(passwordProfilesModel);
}

bool CredentialsManagement::isClean() const
{
    return m_isClean;
}

void CredentialsManagement::enableCredentialsManagement(bool enable)
{
    if (enable)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageUnlocked);
        setCredentialsClean();
        if (wsClient->isCredMirroringAvailable())
        {
            emit displayCredentialLink();
        }
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->pageLocked);
        if (wsClient->isCredMirroringAvailable())
        {
            emit hideCredentialLink();
        }
        m_linkingMode = LinkingMode::OFF;
        enableNonCredentialEditWidgets();
    }

    ui->pageUnlocked->setFocus();
}

void CredentialsManagement::updateQuickAddCredentialsButtonState()
{
    bool isValidPasswordInput = (ui->addCredPasswordInput->hasAcceptableInput() && ui->addCredPasswordInput->text().length() > 0) || m_altKeyPressed;
    ui->addCredentialButton->setEnabled(ui->addCredServiceInput->hasAcceptableInput() && ui->addCredServiceInput->text().length() > 0 &&
                                        isValidPasswordInput && !m_invalidLoginName && !m_invalidPassword);
}

void CredentialsManagement::onPasswordInputReturnPressed()
{
    on_addCredentialButton_clicked();
    ui->addCredServiceInput->setFocus();
}

void CredentialsManagement::on_pushButtonEnterMMM_clicked()
{
    clearMMMUi();
    wsClient->sendEnterMMRequest();
    emit wantEnterMemMode();
}

void CredentialsManagement::on_buttonDiscard_pressed()
{
    auto modelSerialization = m_pCredModel->getJsonChanges();
    if (modelSerialization == m_loadedModelSerialiation)
    {
        wsClient->sendLeaveMMRequest();
        m_pCredModel->clear();
    }
    // Enable add credential fields when exiting MMM
    enableNonCredentialEditWidgets();
}

void CredentialsManagement::onButtonDiscard_confirmed()
{
    wsClient->sendLeaveMMRequest();
    m_pCredModel->clear();
}

void CredentialsManagement::on_buttonSaveChanges_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageLocked);
    wsClient->sendCredentialsMM(m_pCredModel->getJsonChanges());
    emit wantSaveMemMode(); //waits for the daemon to process the data
    m_pCredModel->clear();
    // Enable add credential fields when exiting MMM
    enableNonCredentialEditWidgets();
}

void CredentialsManagement::requestPasswordForSelectedItem()
{
    if (!wsClient->get_memMgmtMode())
        return;

    // Get selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() == 0)
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
            ui->credDisplayPasswordInput->setPlaceholderText(tr("Please Approve On Device"));

            setEnabled(false);
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
                                    QString{},
                                    m_credentialLinkedAddr);
        if (m_credentialLinkedAddr.size() > 0)
        {
            // Reset password input to default state after save
            ui->addCredPasswordInput->displayLinkedIcon(false);
        }
        m_credentialLinkedAddr.clear();
        ui->addCredServiceInput->clear();
        ui->addCredLoginInput->clear();
        ui->addCredPasswordInput->clear();
    }
    else
    {
        ui->addCredentialButton->setEnabled(false);
        ui->gridLayoutAddCred->setEnabled(false);
        wsClient->addOrUpdateCredential(ui->addCredServiceInput->text(),
                                        ui->addCredLoginInput->text(),
                                        ui->addCredPasswordInput->text());

        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(wsClient, &WSClient::credentialsUpdated, [this, conn](const QString & service, const QString & login, const QString &, bool success, const QString &)
        {
            disconnect(*conn);
            ui->addCredentialButton->setEnabled(true);
            ui->gridLayoutAddCred->setEnabled(true);
            if (success)
                QMessageBox::information(this, "Moolticute", tr("%1: New Login %2 added.").arg(service, login));
            else
                QMessageBox::warning(this, tr("Failure"), tr("Couldn't Add New Credential to Device"));

            ui->addCredServiceInput->clear();
            ui->addCredLoginInput->clear();
            ui->addCredPasswordInput->clear();
        });
    }
}

LoginItem * CredentialsManagement::tryGetSelectedLogin()
{
    const QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
    const QModelIndex realCurrent = pSelectionModel->currentIndex();

    LoginItem *loginItem = nullptr;
    if (realCurrent.isValid())
    {
        auto sourceIndex = getSourceIndexFromProxyIndex(realCurrent);
        loginItem  = m_pCredModel->getLoginItemByIndex(sourceIndex);
    }

    return loginItem;
}

void CredentialsManagement::onPasswordUnlocked(const QString & service, const QString & login,
                                               const QString & password, bool success)
{
    setEnabled(true);

    if (success)
    {
        LoginItem *selectedLogin = tryGetSelectedLogin();
        TreeItem *serviceItem = nullptr;
        if (selectedLogin)
            serviceItem = dynamic_cast<ServiceItem *>(selectedLogin->parentItem());

        if (selectedLogin && serviceItem)
        {
            if (service == serviceItem->name() && login == selectedLogin->name())
            {
                m_pCredModel->setClearTextPassword(service, login, password);
                ui->credDisplayPasswordInput->setText(password);
                ui->credDisplayPasswordInput->setLocked(false);
            }
        }
    } else
        ui->credDisplayPasswordInput->setPlaceholderText(tr("Password Query Was Denied"));
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

void CredentialsManagement::saveCredential(const QModelIndex currentSelectionIndex)
{
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(currentSelectionIndex);

    // Do we have a login item?
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
    if (pLoginItem != nullptr) {
        const QString newServiceName = ui->credDisplayServiceInput->text();
        m_pCredModel->updateLoginItem(srcIndex,
                                      ui->credDisplayPasswordInput->text(),
                                      ui->credDisplayDescriptionInput->text(),
                                      ui->credDisplayLoginInput->text(),
                                      ui->credDisplayCategoryInput->currentData().toInt(),
                                      ui->credDisplayKeyAfterLoginInput->currentData().toInt(),
                                      ui->credDisplayKeyAfterPwdInput->currentData().toInt());
        if (wsClient->isCredMirroringAvailable() && pLoginItem->pointedToChildAddress() != pLoginItem->pointedToChildAddressTmp())
        {
            // Save pointed to address if it was changed
            pLoginItem->setPointedToChildAddress(pLoginItem->pointedToChildAddressTmp());
            credentialDataChanged();
        }
        ui->credentialTreeView->refreshLoginItem(srcIndex);
        if (pLoginItem->parentItem()->name() != newServiceName)
        {
            pLoginItem->parentItem()->setName(newServiceName);
            const QModelIndex serviceIdx = m_pCredModel->getServiceIndexByName(newServiceName);
            const auto filterText = ui->lineEditFilterCred->text();
            if (!filterText.isEmpty() && !newServiceName.contains(filterText))
            {
                //Remove filter if new service name does not contain filter text
                ui->lineEditFilterCred->clear();
            }
            //Service name changed, sort to the correct order
            m_pCredModel->dataChanged(serviceIdx, serviceIdx);
        }
    }
}

void CredentialsManagement::saveSelectedCredential()
{
    const QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
    const QModelIndex currentSelectionIndex = pSelectionModel->currentIndex();

    if (currentSelectionIndex.isValid())
        saveCredential(currentSelectionIndex);

    if (!m_credentialLinkedAddr.isEmpty())
    {
        m_credentialLinkedAddr.clear();
    }
}

void CredentialsManagement::saveSelectedTOTP()
{
    if (!m_pTOTPCred->validateInput())
    {
        return;
    }
    const QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
    const QModelIndex currentSelectionIndex = pSelectionModel->currentIndex();

    if (currentSelectionIndex.isValid())
    {
        QModelIndex srcIndex = getSourceIndexFromProxyIndex(currentSelectionIndex);

        LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
        if (pLoginItem != nullptr) {
            m_pCredModel->setTOTP(srcIndex, m_pTOTPCred->getSecretKey(), m_pTOTPCred->getTimeStep(), m_pTOTPCred->getCodeSize());
            credentialDataChanged();
        }
    }
}

bool CredentialsManagement::confirmDiscardUneditedCredentialChanges(const QModelIndex &proxyIndex)
{   
    if (ui->stackedWidget->currentWidget() != ui->pageUnlocked || !wsClient->get_memMgmtMode())
        return true;

    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    if (!srcIndex.isValid())
    {
        QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
        QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
        if (lIndexes.size() == 0)
            return true;

        srcIndex = getSourceIndexFromProxyIndex(lIndexes.first());
    }

    // Retrieve login item
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);

    if (pLoginItem != nullptr)
    {
        if (pLoginItem->isDeleted())
        {
            // Do not need to check for discard, login is deleted
            return true;
        }
        // Retrieve parent item
        TreeItem *pServiceItem = pLoginItem->parentItem();
        if (pServiceItem != nullptr)
        {
            // Retrieve UI information
            QString sPassword = ui->credDisplayPasswordInput->text();
            QString sDescription = ui->credDisplayDescriptionInput->text();
            QString sLogin = ui->credDisplayLoginInput->text();
            int iCategory = ui->credDisplayCategoryInput->currentData().toInt();
            int iKeyAfterLogin = ui->credDisplayKeyAfterLoginInput->currentData().toInt();
            int iKeyAfterPwd = ui->credDisplayKeyAfterPwdInput->currentData().toInt();

            if ((!sPassword.isEmpty() && (sPassword != pLoginItem->password())) ||
                    (!sDescription.isEmpty() && (sDescription != pLoginItem->description())) ||
                    (!sLogin.isEmpty() && sLogin != pLoginItem->name()) ||
                    (wsClient->isMPBLE() && iCategory != 0 && iCategory != pLoginItem->category()) ||
                    (wsClient->isMPBLE() && iKeyAfterLogin != SettingsGuiBLE::DEFAULT_INDEX && iKeyAfterLogin != pLoginItem->keyAfterLogin()) ||
                    (wsClient->isMPBLE() && iKeyAfterPwd != SettingsGuiBLE::DEFAULT_INDEX && iKeyAfterPwd != pLoginItem->keyAfterPwd()))
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
                    saveCredential(proxyIndex);
                    return true;
                }
            }
        }
    }

    return true;
}

void CredentialsManagement::saveChanges()
{
    saveSelectedCredential();
    wsClient->sendCredentialsMM(m_pCredModel->getJsonChanges());
    emit wantSaveMemMode();
}

void CredentialsManagement::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
    if (wsClient->get_memMgmtMode() || !wsClient->isMPBLE())
    {
        return;
    }
    if (Qt::Key_Alt == event->key())
    {
        m_altKeyPressed = true;
        updateQuickAddCredentialsButtonState();
    }
}

void CredentialsManagement::keyReleaseEvent(QKeyEvent *event)
{
    QWidget::keyReleaseEvent(event);
    if (wsClient->get_memMgmtMode() || !wsClient->isMPBLE())
    {
        return;
    }
    if (Qt::Key_Alt == event->key())
    {
        m_altKeyPressed = false;
        updateQuickAddCredentialsButtonState();
    }
}

void CredentialsManagement::on_pushButtonConfirm_clicked()
{
    saveSelectedCredential();
    updateSaveDiscardState();
    if (wsClient->isMPBLE())
    {
        const auto& currentIndex = ui->credentialTreeView->selectionModel()->currentIndex();
        updateBleFavs(getSourceIndexFromProxyIndex(currentIndex));
    }
}

void CredentialsManagement::on_pushButtonCancel_clicked()
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve selected indexes
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() == 0)
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
        ui->credDisplayCategoryInput->setCurrentIndex(pLoginItem->category());
        auto keyAfterLoginIdx = ui->credDisplayKeyAfterLoginInput->findData(pLoginItem->keyAfterLogin());
        ui->credDisplayKeyAfterLoginInput->setCurrentIndex(keyAfterLoginIdx);
        auto keyAfterPwdIdx = ui->credDisplayKeyAfterPwdInput->findData(pLoginItem->keyAfterPwd());
        ui->credDisplayKeyAfterPwdInput->setCurrentIndex(keyAfterPwdIdx);
        if (pLoginItem->pointedToChildAddressTmp() != pLoginItem->pointedToChildAddress())
        {
            pLoginItem->setPointedToChildAddressTmp(pLoginItem->pointedToChildAddress());
            if (pLoginItem->isPointedPassword())
            {
                ui->credDisplayPasswordInput->displayLinkedIcon(true);
                ui->credDisplayPasswordInput->setPlaceholderText(m_pCredModel->getCredentialNameForAddress(pLoginItem->pointedToChildAddress()));
            }
            else
            {
                ui->credDisplayPasswordInput->displayLinkedIcon(false);
                ui->credDisplayPasswordInput->setLocked(true);
            }
        }
        auto *serviceItem = pLoginItem->parentItem();
        if (nullptr != serviceItem)
        {
            ui->credDisplayServiceInput->setText(serviceItem->name());
        }
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
        if (lIndexes.size() == 0)
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

        enableNonCredentialEditWidgets();
    }
    else
    {
        // Do we have a login item?
        LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);

        if (pLoginItem != nullptr)
        {
            // Retrieve item info
            QString sService = ui->credDisplayServiceInput->text();
            QString sPassword = ui->credDisplayPasswordInput->text();
            QString sDescription = ui->credDisplayDescriptionInput->text();
            QString sLogin = ui->credDisplayLoginInput->text();
            int iCategory = ui->credDisplayCategoryInput->currentData().toInt();
            int iKeyAfterLogin = ui->credDisplayKeyAfterLoginInput->currentData().toInt();
            int iKeyAfterPwd = ui->credDisplayKeyAfterPwdInput->currentData().toInt();


            bool bServiceCondition = sService != pLoginItem->parentItem()->name();
            bool bPasswordCondition = !sPassword.isEmpty() && (sPassword != pLoginItem->password());
            bool bDescriptionCondition = !sDescription.isEmpty() && (sDescription != pLoginItem->description());
            bool bLoginCondition = sLogin != pLoginItem->name();
            bool bCategoryCondition = wsClient->isMPBLE() && iCategory != pLoginItem->category();
            bool bKeyAfterLoginCondition = wsClient->isMPBLE() && iKeyAfterLogin != pLoginItem->keyAfterLogin();
            bool bKeyAfterPwdCondition = wsClient->isMPBLE() && iKeyAfterPwd != pLoginItem->keyAfterPwd();
            bool bPointToPwdCondition = wsClient->isCredMirroringAvailable() &&
                                          pLoginItem->pointedToChildAddress() != pLoginItem->pointedToChildAddressTmp();

            bool isServiceExist = false;
            if (bServiceCondition)
            {
                isServiceExist = isServiceNameExist(sService);

                if (isServiceExist)
                {
                    setServiceInputAttributes(tr("Service already exists"), Qt::red);
                }
            }

            if (!bServiceCondition || !isServiceExist)
            {
                setServiceInputAttributes("", Qt::black);
            }

            bool isLoginExist = false;
            if (bLoginCondition)
            {
                isLoginExist = isLoginNameExistForService(sLogin, sService);

                if (isLoginExist)
                {
                    setLoginInputAttributes(tr("Login already exists"), Qt::red);
                }
            }

            if (!bLoginCondition || !isLoginExist)
            {
                setLoginInputAttributes("", Qt::black);
            }

            if (!isLoginExist && !isServiceExist && !sService.isEmpty() &&
                    (bServiceCondition || bPasswordCondition ||
                     bDescriptionCondition || bLoginCondition || bCategoryCondition ||
                     bKeyAfterLoginCondition || bKeyAfterPwdCondition || bPointToPwdCondition)
                    && !m_invalidDisplayLoginName && !m_invalidDisplayPassword)
            {
                ui->pushButtonCancel->show();
                ui->pushButtonConfirm->show();

                disableNonCredentialEditWidgets();
            }
            else
            {
                ui->pushButtonCancel->hide();
                ui->pushButtonConfirm->hide();

                if (wsClient->isMPBLE() && pLoginItem->hasBlankPwdChanged())
                {
                    pLoginItem->setPwdBlankFlag(0);
                }

                enableNonCredentialEditWidgets();
            }
        }
        else
        {
            ui->pushButtonCancel->hide();
            ui->pushButtonConfirm->hide();

            enableNonCredentialEditWidgets();
        }
    }
}

void CredentialsManagement::changeCurrentFavorite(int iFavorite)
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

    // Retrieve selected indexes
    QModelIndexList lIndexes = pSelectionModel->selectedIndexes();
    if (lIndexes.size() == 0)
        return;

    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(lIndexes.first());

    // Retrieve login item
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);

    if (pLoginItem != nullptr)
    {
        m_pCredModel->updateLoginItem(srcIndex, CredentialModel::FavoriteRole, iFavorite);
        ui->credentialTreeView->refreshLoginItem(srcIndex, true);
        if (iFavorite == -1)
        {
            m_pCredModelFilter->refreshFavorites();
        }
    }
}

void CredentialsManagement::on_pushButtonDelete_clicked()
{
    // Retrieve selection model
    QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();

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
            ui->credDisplayFrame->setEnabled(false);
            clearLoginDescription();
            pLoginItem->setDeleted(true);

            QModelIndexList nextRow = m_pCredModelFilter->getNextRow(lIndexes.at(0));
            auto selectionModel = ui->credentialTreeView->selectionModel();
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

void CredentialsManagement::on_toolButtonClearFilter_clicked()
{
    ui->lineEditFilterCred->clear();
}

void CredentialsManagement::on_lineEditFilterCred_textChanged(const QString &text)
{
    ui->toolButtonClearFilter->setVisible(!text.isEmpty());
}

void CredentialsManagement::onCredentialSelected(const QModelIndex &current, const QModelIndex &previous)
{
    if (m_selectionCanceled || current == previous)
    {
        m_selectionCanceled = false;
        return;
    }

    if (previous.isValid())
    {
        if (!confirmDiscardUneditedCredentialChanges(previous))
        {
            m_selectionCanceled = true;
            auto selectionModel = ui->credentialTreeView->selectionModel();
            selectionModel->setCurrentIndex(previous,
                 QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            return;
        }
    }


    // Don't trust in the "current" value it's wrong when a sourceModel item is deleted.
    const QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
    const QModelIndex realCurrent = pSelectionModel->currentIndex();

    if (realCurrent.isValid())
    {
        auto sourceIndex = getSourceIndexFromProxyIndex(realCurrent);
        auto pLoginItem  = m_pCredModel->getLoginItemByIndex(sourceIndex);
        auto pServiceItem = m_pCredModel->getServiceItemByIndex(sourceIndex);
        if (pLoginItem != nullptr)
            emit loginSelected(sourceIndex);
        else
            if (pServiceItem != nullptr)
                emit serviceSelected(sourceIndex);
        updateSaveDiscardState(realCurrent);
    }

}

void CredentialsManagement::onLoginSelected(const QModelIndex &srcIndex)
{
    if (wsClient->isCredMirroringAvailable())
    {
        checkLinkingOnLoginSelected(srcIndex);
    }
    else
    {
        ui->credDisplayFrame->setEnabled(true);
    }

    updateLoginDescription(srcIndex);
    if (wsClient->isMPBLE())
    {
        updateBleFavs(srcIndex);
    }
}

void CredentialsManagement::onServiceSelected(const QModelIndex &srcIndex)
{
    Q_UNUSED(srcIndex);
    if (m_linkingMode != LinkingMode::OFF)
    {
        ui->pushButtonLinkTo->setEnabled(false);
    }
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
#if QT_VERSION < 0x060000
            ui->credDisplayCreationDateInput->setText(pLoginItem->updatedDate().toString(Qt::DefaultLocaleShortDate));
            ui->credDisplayModificationDateInput->setText(pLoginItem->accessedDate().toString(Qt::DefaultLocaleShortDate));
#else

            ui->credDisplayCreationDateInput->setText(QLocale().toString(pLoginItem->updatedDate(), QLocale::ShortFormat));
            ui->credDisplayModificationDateInput->setText(QLocale().toString(pLoginItem->accessedDate(), QLocale::ShortFormat));
#endif
            ui->credDisplayPasswordInput->setLocked(pLoginItem->passwordLocked());
            if (wsClient->isMPBLE())
            {
                ui->credDisplayPasswordInput->checkPwdBlankFlag(pLoginItem->pwdBlankFlag());
                ui->credDisplayCategoryInput->setCurrentIndex(pLoginItem->category());
                auto keyAfterLoginIdx = ui->credDisplayKeyAfterLoginInput->findData(pLoginItem->keyAfterLogin());
                ui->credDisplayKeyAfterLoginInput->setCurrentIndex(keyAfterLoginIdx);
                auto keyAfterPwdIdx = ui->credDisplayKeyAfterPwdInput->findData(pLoginItem->keyAfterPwd());
                ui->credDisplayKeyAfterPwdInput->setCurrentIndex(keyAfterPwdIdx);
                if (wsClient->isCredMirroringAvailable())
                {
                    if (pLoginItem->isPointedPassword())
                    {
                        QString pointToName = m_pCredModel->getCredentialNameForAddress(pLoginItem->pointedToChildAddress());
                        if (pointToName.isEmpty())
                        {
                            /* If pointed to address's credential is not available then it was deleted. */
                            ui->credDisplayPasswordInput->setPlaceholderText(tr("Pointed to credential is deleted"));
                        }
                        else
                        {
                            ui->credDisplayPasswordInput->setLocked(true);
                            ui->credDisplayPasswordInput->setPlaceholderText(pointToName);
                        }
                    }
                    ui->credDisplayPasswordInput->displayLinkedIcon(pLoginItem->isPointedPassword());
                }
                ui->pushButtonTOTP->setText(tr("Setup TOTP Credential"));
                if (pLoginItem->totpTimeStep() != 0)
                {
                    m_pTOTPCred->setTimeStep(pLoginItem->totpTimeStep());
                    m_pTOTPCred->setCodeSize(pLoginItem->totpCodeSize());
                    ui->pushButtonTOTP->setEnabled(false);
                    ui->toolButtonTOTPService->show();
                    ui->pushButtonDeleteTOTP->show();
                }
                else
                {
                    ui->pushButtonTOTP->setEnabled(true);
                    ui->toolButtonTOTPService->hide();
                    ui->pushButtonDeleteTOTP->hide();
                }
            }
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
    ui->credDisplayCategoryInput->setCurrentIndex(0);
    ui->credDisplayKeyAfterLoginInput->setCurrentIndex(0);
    ui->credDisplayKeyAfterPwdInput->setCurrentIndex(0);
}

QModelIndex CredentialsManagement::getSourceIndexFromProxyIndex(const QModelIndex &proxyIndex)
{
    if (proxyIndex.isValid())
        return m_pCredModelFilter->mapToSource(proxyIndex);
    return QModelIndex();
}

QModelIndex CredentialsManagement::getProxyIndexFromSourceIndex(const QModelIndex &srcIndex)
{
    return m_pCredModelFilter->mapFromSource(srcIndex);
}

void CredentialsManagement::setCredentialsClean()
{
    m_isClean = true;
    ui->buttonExit->setVisible(true);
    ui->buttonDiscard->setVisible(false);
    ui->buttonSaveChanges->setVisible(false);
    connect(m_pCredModel, &CredentialModel::dataChanged, this, &CredentialsManagement::credentialDataChanged);
    connect(m_pCredModel, &CredentialModel::rowsInserted, this, &CredentialsManagement::credentialDataChanged);
    connect(m_pCredModel, &CredentialModel::rowsRemoved, this, &CredentialsManagement::credentialDataChanged);
}

void CredentialsManagement::disableNonCredentialEditWidgets()
{
    ui->quickInsertWidget->setEnabled(false);
    ui->credentialsListWdiget->setEnabled(false);
}

void CredentialsManagement::enableNonCredentialEditWidgets()
{
    if (m_linkingMode == LinkingMode::OFF)
    {
        ui->quickInsertWidget->setEnabled(true);
    }
    ui->pushButtonFavorite->setEnabled(true);
    ui->credentialsListWdiget->setEnabled(true);
}

bool CredentialsManagement::isServiceNameExist(const QString &serviceName) const
{
    auto found = std::find_if(m_loadedModelSerialiation.begin(), m_loadedModelSerialiation.end(),
                 [&serviceName] (const QJsonValue &cred)
                    {
                        return serviceName == cred.toObject().value("service").toString();
                    }
                 );
    return found != m_loadedModelSerialiation.end();
}

bool CredentialsManagement::isLoginNameExistForService(const QString &loginName, const QString &serviceName) const
{
    auto found = std::find_if(m_loadedModelSerialiation.begin(), m_loadedModelSerialiation.end(),
                 [&loginName, &serviceName] (const QJsonValue &cred)
                    {
                        auto credObj = cred.toObject();
                        return loginName == credObj.value("login").toString() &&
                                serviceName == credObj.value("service").toString();
                    }
                 );
    return found != m_loadedModelSerialiation.end();
}

void CredentialsManagement::setServiceInputAttributes(const QString &tooltipText, Qt::GlobalColor col)
{
    QPalette pal;
    pal.setColor(QPalette::Text, col);
    ui->credDisplayServiceInput->setPalette(pal);
    ui->credDisplayServiceInput->setToolTip(tooltipText);
}

void CredentialsManagement::setLoginInputAttributes(const QString &tooltipText, Qt::GlobalColor col)
{
    QPalette pal;
    pal.setColor(QPalette::Text, col);
    ui->credDisplayLoginInput->setPalette(pal);
    ui->credDisplayLoginInput->setToolTip(tooltipText);
}

void CredentialsManagement::clearMMMUi()
{
    clearLoginDescription();
    ui->credDisplayFrame->setEnabled(false);
    m_pCredModel->clear();
    ui->credentialTreeView->repaint();
    m_linkingMode = LinkingMode::OFF;
}

void CredentialsManagement::updateBleFavs(const QModelIndex &srcIndex)
{
    if (!srcIndex.isValid())
    {
        qCritical() << "Invalid ModelIndex, cannot update BLE favorites";
        return;
    }
    const auto category = getCategory(srcIndex);
    int i = 0;
    int from = category * MAX_BLE_CAT_NUM  + 1;
    int to = from + MAX_BLE_CAT_NUM ;
    for (QAction* action : m_favMenu.actions())
    {
        bool visible = false;
        // Setting visible No Favorite and category's favs
        if (0 == i || (i >= from && i < to))
        {
            visible = true;
        }
        action->setVisible(visible);
        ++i;
    }
}

void CredentialsManagement::sendGetUserCategories()
{
    if (!m_pCredModel->isUserCategoryClean())
    {
        wsClient->sendGetUserCategories();
        m_pCredModel->setUserCategoryClean(true);
    }
}

void CredentialsManagement::initKeyAfterInput(QComboBox *cbKeyAfter)
{
    cbKeyAfter->addItem(MainWindow::DEFAULT_KEY_STRING, SettingsGuiBLE::DEFAULT_INDEX);
    cbKeyAfter->addItem(MainWindow::NONE_STRING, SettingsGuiBLE::NONE_INDEX);
    cbKeyAfter->addItem(MainWindow::TAB_STRING, SettingsGuiBLE::TAB_INDEX);
    cbKeyAfter->addItem(MainWindow::ENTER_STRING, SettingsGuiBLE::ENTER_INDEX);
    cbKeyAfter->addItem(MainWindow::SPACE_STRING, SettingsGuiBLE::SPACE_INDEX);
}

void CredentialsManagement::onItemCollapsed(const QModelIndex &proxyIndex)
{
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(proxyIndex);
    ServiceItem *pServiceItem = m_pCredModel->getServiceItemByIndex(srcIndex);
    if (pServiceItem != nullptr)
        pServiceItem->setExpanded(false);
}

void CredentialsManagement::onLoginEdited(const QString &loginName)
{
    static auto defaultStyle = ui->addCredLoginInput->styleSheet();
    checkInputLength(ui->addCredLoginInput, m_invalidLoginName, defaultStyle, loginName.size(), getMaxLoginLength());
}

void CredentialsManagement::onDisplayLoginTextChanged(const QString &loginName)
{
    static auto defaultStyle = ui->addCredLoginInput->styleSheet();
    checkInputLength(ui->credDisplayLoginInput, m_invalidDisplayLoginName, defaultStyle, loginName.size(), getMaxLoginLength());
    updateSaveDiscardState();
}

void CredentialsManagement::onPasswordEdited(const QString &password)
{
    static auto defaultStyle = ui->addCredPasswordInput->styleSheet();
    checkInputLength(ui->addCredPasswordInput, m_invalidPassword, defaultStyle, password.size(), getMaxPasswordLength());
}

void CredentialsManagement::onDisplayPasswordTextChanged(const QString &password)
{
    static auto defaultStyle = ui->addCredLoginInput->styleSheet();
    checkInputLength(ui->credDisplayPasswordInput, m_invalidDisplayPassword, defaultStyle, password.size(), getMaxPasswordLength());
    updateSaveDiscardState();
}

void CredentialsManagement::checkInputLength(QLineEdit *input, bool &isInvalid, const QString& defaultStyle, int nameSize, int maxLength)
{
    if (nameSize > maxLength)
    {
        if (!isInvalid)
        {
            input->setStyleSheet(INVALID_INPUT_STYLE);
            input->setToolTip(tr("Text is too long, maximum length is %1").arg(maxLength));
            isInvalid = true;
        }
    }
    else
    {
        if (isInvalid)
        {
            input->setStyleSheet(defaultStyle);
            input->setToolTip("");
            isInvalid = false;
        }
    }
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

void CredentialsManagement::updateFavMenu()
{
    QList<QAction*> actions = m_favMenu.actions();
    for (QAction* action : actions)
    {
        action->setEnabled(true);
    }

    // check taken favs
    if (m_pCredModel)
    {
        int services = m_pCredModel->rowCount();
        for (int i = 0; i < services; i ++)
        {
            auto service_index = m_pCredModel->index(i,0);
            int logins = m_pCredModel->rowCount(service_index);
            for (int j = 0; j < logins; j++)
            {
                auto login_index = m_pCredModel->index(j, 0, service_index);
                auto login = m_pCredModel->getLoginItemByIndex(login_index);
                int favNumber = login->favorite();
                if (favNumber >= 0 && actions.length() > favNumber &&
                        actions.at(favNumber+1) != nullptr)
                    actions.at(favNumber+1)->setEnabled(false);
            }
        }
    }
}

void CredentialsManagement::credentialDataChanged()
{
    m_isClean = false;
    ui->buttonExit->setVisible(false);
    ui->buttonSaveChanges->setVisible(true);
    ui->buttonDiscard->setVisible(true);
    disconnect(m_pCredModel, &CredentialModel::dataChanged, this, &CredentialsManagement::credentialDataChanged);
    disconnect(m_pCredModel, &CredentialModel::rowsInserted, this, &CredentialsManagement::credentialDataChanged);
    disconnect(m_pCredModel, &CredentialModel::rowsRemoved, this, &CredentialsManagement::credentialDataChanged);
}

void CredentialsManagement::checkDeviceType()
{
    if (wsClient->isMPBLE() && !wsClient->get_memMgmtMode() && wsClient->get_advancedMenu())
    {
        ui->widget_UserCategories->show();
        sendGetUserCategories();
    }
    else
    {
        ui->widget_UserCategories->hide();
    }
}

void CredentialsManagement::updateFavMenuOnDevChanged(Common::MPHwVersion newDev)
{
    m_favMenu.clear();
    QAction *action = m_favMenu.addAction(tr("Not a favorite"));
    connect(action, &QAction::triggered, [this](){ changeCurrentFavorite(Common::FAV_NOT_SET); });
    bool isBle = Common::MP_BLE == newDev;
    const auto size = isBle ? BLE_FAVORITE_NUM : MINI_FAVORITE_NUM;
    for (int i = 0; i < size; ++i)
    {
        action = m_favMenu.addAction(tr("Set as favorite #%1").arg(isBle ? (i%MAX_BLE_CAT_NUM)+1 : i+1));
        connect(action, &QAction::triggered, [this, i](){ changeCurrentFavorite(i); });
    }
}

void CredentialsManagement::updateDeviceType(Common::MPHwVersion newDev)
{
    if (Common::MP_BLE == newDev)
    {
        ui->credDisplayCategoryInput->show();
        ui->credDisplayCategoryLabel->show();
        if (wsClient->get_advancedMenu())
        {
            ui->credDisplayKeyAfterLoginInput->show();
            ui->credDisplayKeyAfterLoginLabel->show();
            ui->credDisplayKeyAfterPwdInput->show();
            ui->credDisplayKeyAfterPwdLabel->show();
        }
        ui->pushButtonTOTP->show();
        ui->pushButtonDeleteTOTP->hide();
    }
    else
    {
        ui->credDisplayCategoryInput->hide();
        ui->credDisplayCategoryLabel->hide();
        ui->credDisplayKeyAfterLoginInput->hide();
        ui->credDisplayKeyAfterLoginLabel->hide();
        ui->credDisplayKeyAfterPwdInput->hide();
        ui->credDisplayKeyAfterPwdLabel->hide();
        ui->pushButtonTOTP->hide();
        ui->pushButtonDeleteTOTP->hide();
    }
}

void CredentialsManagement::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QWidget::changeEvent(event);
}

int CredentialsManagement::getCategory(const QModelIndex &srcIndex)
{
    return m_pCredModel->getLoginItemByIndex(srcIndex)->category();
}

void CredentialsManagement::checkLinkingOnLoginSelected(const QModelIndex &srcIndex)
{
    bool isLinking = m_linkingMode != LinkingMode::OFF;
    ui->credDisplayFrame->setEnabled(!isLinking);
    if (isLinking)
    {
        LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
        ui->pushButtonLinkTo->setEnabled(!pLoginItem->isPointedPassword());
        if (LinkingMode::CREDENTIAL_EDIT == m_linkingMode && !pLoginItem->isPointedPassword())
        {
            // Check if the selected login is the same that we want to link
            QModelIndex originalIndex = getSourceIndexFromProxyIndex(m_credentialToLinkIndex);
            LoginItem *originalLogin = m_pCredModel->getLoginItemByIndex(originalIndex);
            if (pLoginItem->address() == originalLogin->address() /* Cannot link the actual credential */
                    || pLoginItem->address().isEmpty()) /* Or when credential's address is empty (new credential) */
            {
                ui->pushButtonLinkTo->setEnabled(false);
            }
        }
    }
}

QString CredentialsManagement::processMultipleDomainsInput(const QString& service, const QString &domains, const bool disable_tld_check)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    auto splitBehavior = QString::SkipEmptyParts;
#else
    auto splitBehavior = Qt::SkipEmptyParts;
#endif
    auto domainList = domains.split(MULT_DOMAIN_SEPARATOR, splitBehavior);
    QStringList validDomains;
    QStringList invalidDomains;
    QString result = "";
    auto serviceDomain = service;
    if (serviceDomain.contains('.'))
    {
        ParseDomain parsedService{service};
        serviceDomain = parsedService.domain();
    }
    for (auto& domain : domainList)
    {
        if (!domain.startsWith('.'))
        {
            domain.prepend('.');
        }
        ParseDomain dom(serviceDomain + domain);
        if (disable_tld_check || domain == dom.tld())
        {
            validDomains.append(domain);
        }
        else
        {
            qWarning() << "The following domain is not valid: " << domain;
            invalidDomains.append(domain);
        }
    }
    if (!invalidDomains.isEmpty())
    {
        QMessageBox::information(this, tr("Invalid domain"),
                                INVALID_DOMAIN_TEXT.arg(invalidDomains.join("</li><li>")));
    }
    return validDomains.join(MULT_DOMAIN_SEPARATOR);
}

bool CredentialsManagement::isUICategoryClean() const
{
    return ui->lineEditCategory1->text() == m_pCredModel->getCategoryName(1) &&
           ui->lineEditCategory2->text() == m_pCredModel->getCategoryName(2) &&
           ui->lineEditCategory3->text() == m_pCredModel->getCategoryName(3) &&
           ui->lineEditCategory4->text() == m_pCredModel->getCategoryName(4);
}

void CredentialsManagement::on_toolButtonFavFilter_clicked()
{
    bool favFilter = m_pCredModelFilter->switchFavFilter();
    ui->toolButtonFavFilter->setIcon(AppGui::qtAwesome()->icon(favFilter ? fa::star : fa::staro));
}

void CredentialsManagement::on_toolButtonEditService_clicked()
{
    ui->toolButtonEditService->hide();
    ui->credDisplayServiceInput->setReadOnly(false);
    ui->credDisplayServiceInput->setFrame(true);
    ui->credDisplayServiceInput->setFocus();
}

void CredentialsManagement::on_pushButtonSaveCategories_clicked()
{
    const auto cat1 = ui->lineEditCategory1->text();
    const auto cat2 = ui->lineEditCategory2->text();
    const auto cat3 = ui->lineEditCategory3->text();
    const auto cat4 = ui->lineEditCategory4->text();
    wsClient->sendSetUserCategories(cat1, cat2, cat3, cat4);
    ui->pushButtonSaveCategories->hide();
    m_pCredModel->updateCategories(cat1, cat2, cat3, cat4);
    emit wsClient->updateCurrentCategories(cat1, cat2, cat3, cat4);
    ui->credDisplayCategoryInput->setItemText(1, cat1);
    ui->credDisplayCategoryInput->setItemText(2, cat2);
    ui->credDisplayCategoryInput->setItemText(3, cat3);
    ui->credDisplayCategoryInput->setItemText(4, cat4);
    m_isSetCategoryClean = true;
}

void CredentialsManagement::onCategoryEdited(const QString &edited)
{
    Q_UNUSED(edited);
    if (m_isSetCategoryClean)
    {
        ui->pushButtonSaveCategories->show();
        m_isSetCategoryClean = false;
    }
    else if (isUICategoryClean())
    {
        m_isSetCategoryClean = true;
        ui->pushButtonSaveCategories->hide();
    }
}

void CredentialsManagement::handleAdvancedModeChange(bool isEnabled)
{
    ui->credDisplayCategoryInput->setEnabled(isEnabled);
    if (isEnabled)
    {
        ui->credDisplayKeyAfterLoginLabel->show();
        ui->credDisplayKeyAfterLoginInput->show();
        ui->credDisplayKeyAfterPwdLabel->show();
        ui->credDisplayKeyAfterPwdInput->show();
    }
    else
    {
        ui->credDisplayKeyAfterLoginLabel->hide();
        ui->credDisplayKeyAfterLoginInput->hide();
        ui->credDisplayKeyAfterPwdLabel->hide();
        ui->credDisplayKeyAfterPwdInput->hide();
    }
}

void CredentialsManagement::on_pushButtonTOTP_clicked()
{
    if (m_pTOTPCred)
    {
        m_pTOTPCred->show();
        return;
    }
}

void CredentialsManagement::on_pushButtonDeleteTOTP_clicked()
{
    if (QMessageBox::question(this, "Moolticute",
                              tr("TOTP Secret key is going to be removed for the credential.\nContinue?")) != QMessageBox::Yes)
    {
        return;
    }

    const QItemSelectionModel *pSelectionModel = ui->credentialTreeView->selectionModel();
    const QModelIndex currentSelectionIndex = pSelectionModel->currentIndex();

    if (currentSelectionIndex.isValid())
    {
        QModelIndex srcIndex = getSourceIndexFromProxyIndex(currentSelectionIndex);

        LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
        if (pLoginItem != nullptr) {
            pLoginItem->setTOTPCredential("", 0, 0);
            pLoginItem->setTotpCodeSize(0);
            pLoginItem->setTotpTimeStep(0);
            pLoginItem->setTOTPDeleted(true);
            credentialDataChanged();
            updateLoginDescription(srcIndex);
        }
    }
}

void CredentialsManagement::onCredentialLink()
{
    ui->credDisplayFrame->setEnabled(false);
    ui->framePasswordLink->show();
    ui->quickInsertWidget->setEnabled(false);
    m_linkingMode = LinkingMode::NEW_CREDENTIAL;
}

void CredentialsManagement::onCredentialLinkRemoved()
{
    ui->addCredPasswordInput->setPasswordVisible(false);
    ui->addCredPasswordInput->setText("");
}

void CredentialsManagement::onSelectedCredentialLink()
{
    ui->credDisplayFrame->setEnabled(false);
    ui->framePasswordLink->show();
    ui->quickInsertWidget->setEnabled(false);
    ui->pushButtonLinkTo->setEnabled(false);
    m_linkingMode = LinkingMode::CREDENTIAL_EDIT;
    m_credentialToLinkIndex = ui->credentialTreeView->selectionModel()->currentIndex();
}

void CredentialsManagement::onSelectedCredentialLinkRemoved()
{
    ui->credDisplayPasswordInput->setPasswordVisible(true);
    ui->credDisplayPasswordInput->setText("");
    ui->credDisplayPasswordInput->setPlaceholderText("");
    ui->credDisplayPasswordInput->setEnabled(true);
    QModelIndexList lIndexes = ui->credentialTreeView->selectionModel()->selectedIndexes();
    if (lIndexes.size() == 0)
        return;

    // Retrieve src index
    QModelIndex srcIndex = getSourceIndexFromProxyIndex(lIndexes.first());

    // Do we have a login item?
    LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
    pLoginItem->setPointedToChildAddressTmp(QByteArray{2, Common::ZERO_BYTE});
    updateSaveDiscardState();
}

void CredentialsManagement::on_pushButtonDiscardLinking_clicked()
{
    ui->credDisplayFrame->setEnabled(true);
    ui->framePasswordLink->hide();
    ui->quickInsertWidget->setEnabled(true);
    m_linkingMode = LinkingMode::OFF;
}


void CredentialsManagement::on_pushButtonLinkTo_clicked()
{
    QString linkToName = "";
    const QModelIndex currentSelectionIndex = ui->credentialTreeView->selectionModel()->currentIndex();
    if (currentSelectionIndex.isValid())
    {
        QModelIndex srcIndex = getSourceIndexFromProxyIndex(currentSelectionIndex);
        LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
        if (pLoginItem != nullptr)
        {
            linkToName = "<" + pLoginItem->parentItem()->name() + "/" + pLoginItem->name() + ">";
            m_credentialLinkedAddr = pLoginItem->address();
            if (m_linkingMode == LinkingMode::NEW_CREDENTIAL)
            {
                ui->addCredPasswordInput->setPasswordVisible(true);
                ui->addCredPasswordInput->setText(linkToName);
                emit newCredentialLinked();
            }
        }
    }
    if (LinkingMode::CREDENTIAL_EDIT == m_linkingMode)
    {
        ui->credentialTreeView->selectionModel()->setCurrentIndex(m_credentialToLinkIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        QModelIndex srcIndex = getSourceIndexFromProxyIndex(m_credentialToLinkIndex);
        LoginItem *pLoginItem = m_pCredModel->getLoginItemByIndex(srcIndex);
        if (pLoginItem != nullptr)
        {
            pLoginItem->setPointedToChildAddressTmp(m_credentialLinkedAddr);
        }
        ui->credDisplayPasswordInput->setPasswordVisible(true);
        ui->credDisplayPasswordInput->setPlaceholderText(linkToName);
        emit editedCredentialLinked();
        updateSaveDiscardState();
    }
    ui->credDisplayFrame->setEnabled(true);
    ui->framePasswordLink->hide();
    ui->quickInsertWidget->setEnabled(true);
    m_linkingMode = LinkingMode::OFF;
}

void CredentialsManagement::onTreeViewContextMenuRequested(const QPoint& pos)
{
    if (!wsClient->isMultipleDomainsAvailable())
    {
        return;
    }
    auto sourceIndex = getSourceIndexFromProxyIndex(ui->credentialTreeView->indexAt(pos));
    auto* pServiceItem = m_pCredModel->getServiceItemByIndex(sourceIndex);
    if (pServiceItem)
    {
        m_enableMultipleDomainMenu.clear();
        auto multipleDomainsDialogFunc = [this, pServiceItem](){
                        QString defaultDomains = pServiceItem->multipleDomains();
                        QString serviceName = pServiceItem->name();
                        QString serviceDomain = serviceName;
                        bool serviceNameChanged = false;
                        if (defaultDomains.isEmpty())
                        {
                            /* During enabling multiple domains cut the tld
                             * from service name and display it on the dialog
                             */
                            if (serviceDomain.contains('.'))
                            {
                                ParseDomain parsedService{serviceDomain};
                                serviceDomain = parsedService.domain();
                                defaultDomains = parsedService.tld() + MULT_DOMAIN_SEPARATOR;
                                serviceNameChanged = true;
                            }
                        }
                        QSettings s;
                        bool ok = false;
                        bool disable_tld_check = s.value("settings/disable_tld_check", false).toBool();
                        QString text = QInputDialog::getText(this, tr("Multiple Domains for %1").arg(pServiceItem->name()),
                                                                 tr("Enter comma separated domain extensions:"), QLineEdit::Normal,
                                                                 defaultDomains, &ok);
                        if (ok)
                        {
                            text = processMultipleDomainsInput(serviceName, text, disable_tld_check);
                            if (!text.isEmpty())
                            {
                                pServiceItem->setMultipleDomains(text);
                                credentialDataChanged();
                                if (serviceNameChanged)
                                {
                                    // Need to change service name (remove tld)
                                    pServiceItem->setName(serviceDomain);
                                }
                            }
                        }
                    };
        if (pServiceItem->isMultipleDomainSet())
        {
            // Context menu, if there are multiple domains set
            auto* action = m_enableMultipleDomainMenu.addAction(tr("Edit multiple domains"));
            connect(action, &QAction::triggered, multipleDomainsDialogFunc);
            auto* actionRemove = m_enableMultipleDomainMenu.addAction(tr("Remove multiple domains"));
            connect(actionRemove, &QAction::triggered, [pServiceItem, this](){
                QString multipleDomains = pServiceItem->multipleDomains();
                QString domain = multipleDomains.split(MULT_DOMAIN_SEPARATOR).first();
                pServiceItem->setName(pServiceItem->name() + domain);
                pServiceItem->setMultipleDomains("");
                credentialDataChanged();
            });
        }
        else
        {
            auto* action = m_enableMultipleDomainMenu.addAction(tr("Enable multiple domains"));
            connect(action, &QAction::triggered, multipleDomainsDialogFunc);
        }

        m_enableMultipleDomainMenu.popup(ui->credentialTreeView->mapToGlobal(pos));
    }
}
