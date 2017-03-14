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
#include "CredentialsManagement.h"
#include "ui_CredentialsManagement.h"
#include "Common.h"
#include "AppGui.h"

CredentialsManagement::CredentialsManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CredentialsManagement)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->pageLocked);

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

//    ui->credDisplayFrame->setStyleSheet("background-color:#FFFFFF");
    ui->pushButtonEnterMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->addCredentialButton->setStyleSheet(CSS_BLUE_BUTTON);
    ui->buttonDiscard->setStyleSheet(CSS_GREY_BUTTON);
    ui->buttonSaveChanges->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterMMM->setIcon(AppGui::qtAwesome()->icon(fa::unlock, whiteButtons));

    credModel = new CredentialsModel(this);
    credFilterModel = new CredentialsFilterModel(this);

    credFilterModel->setSourceModel(credModel);
    ui->credentialsListView->setModel(credFilterModel);

    QDataWidgetMapper* mapper = new QDataWidgetMapper(this);
    mapper->setItemDelegate(new CredentialViewItemDelegate(mapper));
    mapper->setModel(credFilterModel);
    mapper->addMapping(ui->credDisplayServiceInput, CredentialsModel::ServiceIdx);
    mapper->addMapping(ui->credDisplayLoginInput, CredentialsModel::LoginIdx);
    mapper->addMapping(ui->credDisplayPasswordInput, CredentialsModel::PasswordIdx);
    mapper->addMapping(ui->credDisplayDescriptionInput,  CredentialsModel::DescriptionIdx);
    mapper->addMapping(ui->credDisplayCreationDateInput,  CredentialsModel::DateCreatedIdx);
    mapper->addMapping(ui->credDisplayModificationDateInput,  CredentialsModel::DateModifiedIdx);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

    delete ui->credentialsListView->selectionModel();
    ui->credentialsListView->setSelectionModel(
                new ConditionalItemSelectionModel(ConditionalItemSelectionModel::TestFunction(std::bind(&CredentialsManagement::confirmDiscardUneditedCredentialChanges, this, std::placeholders::_1)),
                                                  credFilterModel));

    connect(ui->credentialsListView->selectionModel(), &QItemSelectionModel::currentRowChanged, mapper, [this, mapper](const QModelIndex & idx)
    {
        mapper->setCurrentIndex(idx.row());
    });
    connect(ui->credDisplayPasswordInput, &LockedPasswordLineEdit::unlockRequested,
            this, &CredentialsManagement::requestPasswordForSelectedItem);

    connect(ui->credDisplayButtonBox, &QDialogButtonBox::clicked, [mapper, this](QAbstractButton* btn)
    {
        if(ui->credDisplayButtonBox->button(QDialogButtonBox::Save) == btn)
        {
            saveSelectedCredential();
        }
        if(ui->credDisplayButtonBox->button(QDialogButtonBox::Reset) == btn)
        {
            mapper->revert();
        }
    });

    connect(ui->addCredServiceInput, &QLineEdit::textChanged,this, &CredentialsManagement::updateQuickAddCredentialsButtonState);
    connect(ui->addCredLoginInput, &QLineEdit::textChanged, this, &CredentialsManagement::updateQuickAddCredentialsButtonState);
    connect(ui->addCredPasswordInput, &QLineEdit::textChanged, this, &CredentialsManagement::updateQuickAddCredentialsButtonState);
    updateQuickAddCredentialsButtonState();

    connect(ui->lineEditFilterCred, &QLineEdit::textChanged, [=](const QString &t)
    {
        credFilterModel->setFilter(t);
    });
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
        credModel->load(wsClient->getMemoryData()["login_nodes"].toArray());
        ui->lineEditFilterCred->clear();
    });
    connect(wsClient, &WSClient::passwordUnlocked, this, &CredentialsManagement::onPasswordUnlocked);
}

void CredentialsManagement::enableCredentialsManagement(bool enable)
{
    if (enable)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageUnlocked);
        connect(wsClient, &WSClient::credentialsUpdated, this, &CredentialsManagement::onCredentialUpdated);
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->pageLocked);
        disconnect(wsClient, &WSClient::credentialsUpdated, this, &CredentialsManagement::onCredentialUpdated);
    }
}

void CredentialsManagement::updateQuickAddCredentialsButtonState()
{
    ui->addCredentialButton->setEnabled(ui->addCredLoginInput->hasAcceptableInput() && ui->addCredLoginInput->text().length() > 0 &&
                                        ui->addCredServiceInput->hasAcceptableInput() && ui->addCredServiceInput->text().length() > 0 &&
                                        ui->addCredPasswordInput->hasAcceptableInput() && ui->addCredPasswordInput->text().length() > 0);
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
}

void CredentialsManagement::on_buttonSaveChanges_clicked()
{
    QMessageBox::information(this, "Moolticute", "Not implemented yet!");
}

void CredentialsManagement::requestPasswordForSelectedItem()
{
    if (!wsClient->get_memMgmtMode()) return;

    QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();

    if (indexes.size() != 1)
        return;

    QModelIndex idx = credFilterModel->mapToSource(indexes.at(0));

    const auto &cred = credModel->at(idx.row());
    wsClient->requestPassword(cred.service, cred.login);
}

void CredentialsManagement::on_addCredentialButton_clicked()
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

void CredentialsManagement::onPasswordUnlocked(const QString & service, const QString & login,
                                    const QString & password, bool success)
{
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to query password!"));
        return;
    }

    credModel->setClearTextPassword(service, login, password);
}

void CredentialsManagement::onCredentialUpdated(const QString & service, const QString & login, const QString & description, bool success) {
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to modify %1/%2").arg(service, login));
        return;
    }

    credModel->update(service, login, description);
}

void CredentialsManagement::saveSelectedCredential(QModelIndex idx)
{
    QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
    if (!idx.isValid())
    {
        QModelIndexList indexes = selection->selectedIndexes();

        if (indexes.size() != 1)
            return;

        idx = credFilterModel->mapToSource(indexes.at(0));
    }
    const auto &cred = credModel->at(idx.row());

    QString password = ui->credDisplayPasswordInput->text();
    QString description = ui->credDisplayDescriptionInput->text();

    if (password != cred.password ||
        description != cred.description)
    {
        wsClient->addOrUpdateCredential(cred.service, cred.login, password, description);
    }
}

bool CredentialsManagement::confirmDiscardUneditedCredentialChanges(QModelIndex idx)
{
    if (ui->stackedWidget->currentWidget() != ui->pageUnlocked ||
        !wsClient->get_memMgmtMode())
        return true;

    if (!idx.isValid())
    {
        QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
        QModelIndexList indexes = selection->selectedIndexes();
        if (indexes.size() != 1)
            return true;

        idx = credFilterModel->mapToSource(indexes.at(0));
    }
    const auto &cred = credModel->at(idx.row());

    QString password = ui->credDisplayPasswordInput->text();
    QString description = ui->credDisplayDescriptionInput->text();

    if (password != cred.password ||
        description != cred.description)
    {
        auto btn = QMessageBox::question(this,
                                         tr("Discard Modifications ?"),
                                         tr("You have modified %1/%2 - Do you want to discard the modifications ?").arg(cred.service, cred.login),
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
            saveSelectedCredential(idx);
            return true;
        }
    }
    return true;
}
