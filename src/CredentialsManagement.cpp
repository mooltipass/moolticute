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
        ui->credDisplayFrame->setEnabled(idx.isValid());
        updateSaveDiscardState(idx);
    });
    connect(ui->credDisplayPasswordInput, &LockedPasswordLineEdit::unlockRequested,
            this, &CredentialsManagement::requestPasswordForSelectedItem);

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
        ui->stackedWidget->setCurrentWidget(ui->pageUnlocked);
    else
        ui->stackedWidget->setCurrentWidget(ui->pageLocked);
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
    wsClient->sendCredentialsMM(credModel->getJsonChanges());
    emit wantSaveMemMode(); //waits for the daemon to process the data
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
    if (wsClient->get_memMgmtMode())
    {
        QMessageBox::information(this, tr("Moolticute"), tr("New credential %1/%2 added successfully.").arg(ui->addCredServiceInput->text(),
                                                                                                            ui->addCredLoginInput->text()));
        ui->addCredServiceInput->clear();
        ui->addCredLoginInput->clear();
        ui->addCredPasswordInput->clear();

        credModel->update(ui->addCredServiceInput->text(),
                          ui->addCredLoginInput->text(),
                          ui->addCredPasswordInput->text(),
                          QString());
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

void CredentialsManagement::onCredentialUpdated(const QString & service, const QString & login, const QString & description, bool success)
{
    Q_UNUSED(description)
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to modify %1/%2").arg(service, login));
        return;
    }
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
    auto cred = credModel->at(idx.row());

    QString password = ui->credDisplayPasswordInput->text();
    QString description = ui->credDisplayDescriptionInput->text();
    QString login = ui->credDisplayLoginInput->text();

    if (password != cred.password ||
        description != cred.description ||
        login != cred.login)
    {
        cred.login = login;
        cred.password = password;
        cred.description = description;
        credModel->update(idx, cred);
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
    QString login = ui->credDisplayLoginInput->text();

    if (password != cred.password ||
        description != cred.description ||
        login != cred.login)
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

void CredentialsManagement::on_pushButtonConfirm_clicked()
{
    QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();
    if (indexes.size() != 1)
        return;

    QModelIndex idx = credFilterModel->mapToSource(indexes.at(0));
    saveSelectedCredential(idx);

    updateSaveDiscardState();
}

void CredentialsManagement::on_pushButtonCancel_clicked()
{
    QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();
    if (indexes.size() != 1)
        return;

    QModelIndex idx = credFilterModel->mapToSource(indexes.at(0));
    const auto &cred = credModel->at(idx.row());

    auto btn = QMessageBox::question(this,
                                     tr("Discard Modifications ?"),
                                     tr("You have modified %1/%2 - Do you want to discard the modifications ?").arg(cred.service, cred.login),
                                     QMessageBox::Discard |
                                     QMessageBox::Cancel,
                                     QMessageBox::Cancel);
    if (btn == QMessageBox::Discard)
    {
        ui->credDisplayPasswordInput->setText(cred.password);
        ui->credDisplayDescriptionInput->setText(cred.description);
        ui->credDisplayLoginInput->setText(cred.login);
        updateSaveDiscardState();
    }
}

void CredentialsManagement::updateSaveDiscardState(QModelIndex idx)
{
    if (!idx.isValid())
    {
        QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
        QModelIndexList indexes = selection->selectedIndexes();
        if (indexes.size() != 1)
        {
            ui->pushButtonCancel->hide();
            ui->pushButtonConfirm->hide();
        }
        else
        {
            idx = credFilterModel->mapToSource(indexes.at(0));
        }
    }

    if (!idx.isValid())
    {
        ui->pushButtonCancel->hide();
        ui->pushButtonConfirm->hide();
    }
    else
    {
        const auto &cred = credModel->at(idx.row());

        QString password = ui->credDisplayPasswordInput->text();
        QString description = ui->credDisplayDescriptionInput->text();
        QString login = ui->credDisplayLoginInput->text();

        if (password != cred.password ||
            description != cred.description ||
            login != cred.login)
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
}

void CredentialsManagement::changeCurrentFavorite(int fav)
{
    QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();
    if (indexes.size() != 1)
        return;

    QModelIndex idx = credFilterModel->mapToSource(indexes.at(0));
    auto cred = credModel->at(idx.row());
    cred.favorite = fav;
    credModel->update(idx, cred);
}

void CredentialsManagement::on_pushButtonDelete_clicked()
{
    QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();
    if (indexes.size() != 1)
        return;

    QModelIndex idx = credFilterModel->mapToSource(indexes.at(0));
    auto cred = credModel->at(idx.row());

    auto btn = QMessageBox::question(this,
                                     tr("Delete?"),
                                     tr("Remove the current credential %1/%2 ?").arg(cred.service, cred.login),
                                     QMessageBox::Yes |
                                     QMessageBox::Cancel,
                                     QMessageBox::Cancel);
    if (btn == QMessageBox::Yes)
    {
        credModel->removeCredential(idx);
    }
}
