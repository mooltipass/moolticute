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
#include "SSHManagement.h"
#include "ui_SSHManagement.h"
#include "Common.h"
#include "AppGui.h"

SSHManagement::SSHManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SSHManagement)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentWidget(ui->pageLocked);
    ui->progressBarLoad2->hide();

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

    ui->pushButtonUnlock->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonUnlock->setIcon(AppGui::qtAwesome()->icon(fa::unlock, whiteButtons));
    connect(ui->buttonDiscard, &QPushButton::clicked, this, &SSHManagement::buttonDiscardClicked);
    ui->buttonDiscard->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImport->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImport->setIcon(AppGui::qtAwesome()->icon(fa::plussquare, whiteButtons));
    ui->pushButtonExport->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExport->setIcon(AppGui::qtAwesome()->icon(fa::floppyo, whiteButtons));
    ui->pushButtonDelete->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDelete->setIcon(AppGui::qtAwesome()->icon(fa::trasho, whiteButtons));

    ui->widgetSpin->setPixmap(AppGui::qtAwesome()->icon(fa::circleonotch).pixmap(QSize(80, 80)));

    keysModel = new QStandardItemModel(this);
    ui->listViewKeys->setModel(keysModel);

    // Enable button when a key is selected.
    connect(ui->listViewKeys, &QListWidget::clicked, this, [this](const QModelIndex &/*index*/)
    {
        ui->pushButtonExport->setEnabled(true);
        ui->pushButtonDelete->setEnabled(true);
    });

    QMenu *menu = new QMenu(this);
    QAction *action;
    action = menu->addAction(tr("Export public key"));
    connect(action, &QAction::triggered, this, &SSHManagement::onExportPublicKey);
    action = menu->addAction(tr("Export private key"));
    connect(action, &QAction::triggered, this, &SSHManagement::onExportPrivateKey);
    ui->pushButtonExport->setMenu(menu);
}

SSHManagement::~SSHManagement()
{
    delete ui;
}

void SSHManagement::setWsClient(WSClient *c)
{
    wsClient = c;
}

void SSHManagement::on_pushButtonUnlock_clicked()
{
    //First check if the ssh service exists.
    //If not, unlock as the file would be empty
    connect(wsClient, &WSClient::dataNodeExists, this, &SSHManagement::onServiceExists);
    wsClient->serviceExists(true, MC_SSH_SERVICE);
    ui->stackedWidget->setCurrentWidget(ui->pageWait);
}

void SSHManagement::onServiceExists(const QString service, bool exists)
{
    if (service.toLower() != MC_SSH_SERVICE)
    {
        qWarning() << "SSH, Wrong service: " << service;
        return;
    }

    disconnect(wsClient, &WSClient::dataNodeExists, this, &SSHManagement::onServiceExists);

    ui->progressBarLoad->setMinimum(0);
    ui->progressBarLoad->setMaximum(0);
    ui->progressBarLoad->setValue(0);
    loaded = false;
    keysModel->clear();

    if (exists)
    {
        sshProcess = new QProcess(this);
        const auto program = QCoreApplication::applicationDirPath () + "/mc-agent";
        if (!QFile::exists(program))
        {
            QMessageBox::critical(this, "Moolticute",
                tr("mc-agent isn't bundled with the Moolticute app!\n\nCannot manage SSH keys."));
            ui->stackedWidget->setCurrentWidget(ui->pageLocked);
            return;
        }

        currentAction = Action::ListKeys;

        QStringList arguments;
        arguments << "--output_progress"
                  << "cli"
                  << "-c"
                  << "list";

        qInfo() << "Running" << program << arguments;
        connect(sshProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                [=](int exitCode, QProcess::ExitStatus exitStatus)
        {
            if (exitStatus != QProcess::NormalExit)
                qWarning() << "SSH agent exits with exit code" << exitCode << "Exit Status:" << exitStatus;

            if (loaded)
                ui->stackedWidget->setCurrentWidget(ui->pageEditSsh);
            else
                ui->stackedWidget->setCurrentWidget(ui->pageLocked);
        });

        connect(sshProcess, &QProcess::readyReadStandardOutput, this, &SSHManagement::readStdOutLoadKeys);

        sshProcess->setReadChannel(QProcess::StandardOutput);
        sshProcess->start(program, arguments);
    }
    else
    {
        loaded = true;
        ui->stackedWidget->setCurrentWidget(ui->pageEditSsh);
    }
}

void SSHManagement::handleProgressErrors(const QJsonObject &rootobj)
{
    if (rootobj.contains("error") && rootobj["error"].toBool())
    {
        // Errors go into the log file.
        auto msg = rootobj["error_message"].toString();
        if (msg.isEmpty())
        {
            msg = tr("Some internal errors occured. Please check the log and contact the dev team.");
        }
        qWarning() << "Errors occurred:" << qPrintable(msg);

        msg.clear();
        switch (currentAction)
        {
            case Action::ListKeys:
                msg = tr("Failed to retrieve keys from the device!");
                break;

            case Action::ImportKey:
                msg = tr("Failed to import key into the device!") + "\n\n" +
                      tr("Make sure it's an OpenSSH private key without a passphrase.");
                break;

            case Action::DeleteKey:
                msg = tr("Failed to delete key from the device!");
                break;

            default: break;
        }

        if (!msg.isEmpty())
        {
            QMessageBox::warning(this, "Moolticute", msg);
        }
    }
    else if (rootobj["msg"] == "progress_detailed")
    {
        qDebug() << "Progress detailed";
        QJsonObject o = rootobj["data"].toObject();
        progressChanged(o["progress_total"].toInt(), o["progress_current"].toInt());
    }
}

void SSHManagement::readStdOutLoadKeys()
{
    while (sshProcess->canReadLine())
    {
        QString line = sshProcess->readLine();
        QJsonParseError err;
        QJsonDocument jdoc = QJsonDocument::fromJson(line.toUtf8(), &err);

        if (err.error != QJsonParseError::NoError)
        {
            qWarning() << "JSON parse error " << err.errorString();
            qWarning() << "Line is: " << line;
            return;
        }

        //progress message§
        if (jdoc.isObject())
        {
            handleProgressErrors(jdoc.object());
        }
        else if (jdoc.isArray()) //keys
        {
            loaded = true;
            keysModel->clear();

            QJsonArray jarr = jdoc.array();
            for (int i = 0;i < jarr.count();i++)
            {
                QJsonObject k = jarr.at(i).toObject();
                QString pub = k["PublicKey"].toString();

                QString type = pub.section(QRegularExpression("\\s+"), 0, 0);
                QString comment = pub.section(QRegularExpression("\\s+"), 2, -1);

                if (type == "ssh-ed25519")
                    type = "ED25519";
                else if (type == "ssh-rsa")
                    type = "RSA";
                else if (type == "ssh-dsa" || type == "ssh-dss")
                    type = "DSA";
                else if (type.startsWith("ecdsa"))
                    type = "ECDSA";

                QStandardItem *item = new QStandardItem(QString("%1 - %2").arg(type).arg(comment));
                item->setIcon(AppGui::qtAwesome()->icon(fa::key));
                item->setData(pub, RolePublicKey);
                item->setData(k["PrivateKey"].toString(), RolePrivateKey);

                keysModel->appendRow(item);
            }
        }
    }

    // Clear selection and buttons after loading keys or an error occurred.
    ui->listViewKeys->clearSelection();
    ui->pushButtonExport->setEnabled(false);
    ui->pushButtonDelete->setEnabled(false);
    currentAction = Action::None;
}

void SSHManagement::progressChanged(int total, int current)
{
    ui->progressBarLoad->setMaximum(total);
    ui->progressBarLoad->setValue(current);
    ui->progressBarLoad2->setMaximum(total);
    ui->progressBarLoad2->setValue(current);
}

void SSHManagement::onExportPublicKey()
{
    QStandardItem *it = keysModel->itemFromIndex(ui->listViewKeys->currentIndex());
    if (!it) return;

    QString fname = QFileDialog::getSaveFileName(this, tr("Save public key"), QString(), tr("OpenSsh public key (*.pub *.*)"));
    if (fname.isEmpty()) return;

    QFile f(fname);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
    {
        QMessageBox::warning(this, "Moolticute", tr("Failed to open file for write"));
        return;
    }

    f.write(it->data(RolePublicKey).toString().toLocal8Bit());
    QMessageBox::information(this, "Moolticute", tr("Public key successfully exported."));
}

void SSHManagement::onExportPrivateKey()
{
    QStandardItem *it = keysModel->itemFromIndex(ui->listViewKeys->currentIndex());
    if (!it) return;

    QString fname = QFileDialog::getSaveFileName(this, tr("Save private key"), QString(), tr("OpenSsh private key (*.key *.*)"));
    if (fname.isEmpty()) return;

    QFile f(fname);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
    {
        QMessageBox::warning(this, "Moolticute", tr("Failed to open file for write"));
        return;
    }

    f.write(it->data(RolePrivateKey).toString().toLocal8Bit());
    QMessageBox::information(this, "Moolticute", tr("Private key successfully exported."));
}

void SSHManagement::buttonDiscardClicked()
{
    loaded = false;
    keysModel->clear();
    ui->stackedWidget->setCurrentWidget(ui->pageLocked);
}

void SSHManagement::on_pushButtonImport_clicked()
{
    const auto fname =
        QFileDialog::getOpenFileName(this, tr("OpenSSH private key"), QString(),
            tr("OpenSSH private key (*.key *.pem *.* *)"));
    if (fname.isEmpty())
        return;

    ui->progressBarLoad2->setMinimum(0);
    ui->progressBarLoad2->setMaximum(0);
    ui->progressBarLoad2->setValue(0);
    ui->progressBarLoad2->show();
    setEnabled(false);

    currentAction = Action::ImportKey;

    sshProcess = new QProcess(this);
    QString program = QCoreApplication::applicationDirPath () + "/mc-agent";
    QStringList arguments;
    arguments << "--output_progress"
              << "cli"
              << "-c"
              << "add"
              << "--key"
              << fname;

    qInfo() << "Running " << program << " " << arguments;
    connect(sshProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus)
    {
        if (exitStatus != QProcess::NormalExit)
            qWarning() << "SSH agent exits with exit code " << exitCode << " Exit Status : " << exitStatus;
        ui->progressBarLoad2->hide();
        setEnabled(true);
    });

    connect(sshProcess, &QProcess::readyReadStandardOutput, this, &SSHManagement::readStdOutLoadKeys);

    sshProcess->setReadChannel(QProcess::StandardOutput);
    sshProcess->start(program, arguments);
}

void SSHManagement::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QWidget::changeEvent(event);
}

void SSHManagement::on_pushButtonDelete_clicked()
{
    QStandardItem *it = keysModel->itemFromIndex(ui->listViewKeys->currentIndex());
    if (!it) return;

    if (QMessageBox::question(this, "Moolticute", tr("You are going to delete the selected key from the device.\n\nProceed?")) != QMessageBox::Yes)
        return;

    ui->progressBarLoad2->setMinimum(0);
    ui->progressBarLoad2->setMaximum(0);
    ui->progressBarLoad2->setValue(0);
    ui->progressBarLoad2->show();
    setEnabled(false);

    currentAction = Action::DeleteKey;

    sshProcess = new QProcess(this);
    QString program = QCoreApplication::applicationDirPath () + "/mc-agent";
    QStringList arguments;
    arguments << "--output_progress"
              << "cli"
              << "-c"
              << "delete"
              << QStringLiteral("--num=%1").arg(ui->listViewKeys->currentIndex().row());

    qInfo() << "Running " << program << " " << arguments;
    connect(sshProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus)
    {
        if (exitStatus != QProcess::NormalExit)
            qWarning() << "SSH agent exits with exit code " << exitCode << " Exit Status : " << exitStatus;
        ui->progressBarLoad2->hide();
        setEnabled(true);
    });

    connect(sshProcess, &QProcess::readyReadStandardOutput, this, &SSHManagement::readStdOutLoadKeys);

    sshProcess->setReadChannel(QProcess::StandardOutput);
    sshProcess->start(program, arguments);
}
