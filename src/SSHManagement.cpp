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

    resetProgress();
    loaded = false;
    keysModel->clear();

    if (exists)
    {
        sshProcess = new QProcess(this);
        const auto program = Common::getMcAgent();
        auto actualProg = program;
        if (!QFile::exists(actualProg))
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
        ui->stackedWidget->setCurrentWidget(ui->pageEditSsh);

        msg.clear();
        switch (currentAction)
        {
            case Action::ListKeys:
                msg = tr("Failed to retrieve keys from the device!");
                break;

            case Action::ImportKey:
                msg = tr("Failed to import key into the device!\n"
                         "\n"
                         "Make sure it's an OpenSSH private key without a passphrase.\n"
                         "\n"
                         "To import password-protected OpenSSH key you can use 'ssh-add' command"
                         " if you have configured your setup correctly"
                         " (mc-agent daemon is running and SSH_AUTH_SOCK is pointing to mc-agent socket).\n"
                         "\n"
                         "The other possibility is to copy your password-protected key and reset password on it"
                         " with 'ssh-keygen -p -f <copy of your rsa key file>'."
                         );
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
        QJsonObject o = rootobj["data"].toObject();
        qDebug() << "Progress detailed" << o;
        const auto total = o["progress_total"].toInt();
        const auto current = o["progress_current"].toInt();
        const auto msg = o["progress_message"].toString();
        Q_ASSERT(!msg.isEmpty());

        ui->progressBarLoad->setMaximum(total);
        ui->progressBarLoad->setValue(current);
        ui->stackedWidget->setCurrentWidget(ui->pageWait);

        if (current == total)
        {
            // If it's an intermediate step then show it's waiting for input on device.
            ui->progressBarLoad->setMaximum(0);
            ui->progressBarLoad->setValue(0);

            // There are two phases of importing/deleting a key: getDataNodeCb and setDataNodeCb.
            // Only change back to edit page in the end.
            if ((currentAction == Action::ImportKey || currentAction == Action::DeleteKey)
                && msg == "WORKING on setDataNodeCb")
            {
                ui->stackedWidget->setCurrentWidget(ui->pageEditSsh);
            }
        }
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
}

void SSHManagement::onExportPublicKey()
{
    QStandardItem *it = keysModel->itemFromIndex(ui->listViewKeys->currentIndex());
    if (!it) return;

    QSettings s;
    QString fname = QFileDialog::getSaveFileName(this, tr("Save public key"),
                                                 s.value("last_used_path/ssh_key_dir", QDir::homePath()).toString(),
                                                 tr("OpenSsh public key (*.pub *.*)"));
    if (fname.isEmpty()) return;

    QFile f(fname);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
    {
        QMessageBox::warning(this, "Moolticute", tr("Failed to open file for write"));
        return;
    }

    s.setValue("last_used_path/ssh_key_dir", QFileInfo(fname).canonicalPath());

    f.write(it->data(RolePublicKey).toString().toLocal8Bit());
    QMessageBox::information(this, "Moolticute", tr("Public key successfully exported."));
}

void SSHManagement::onExportPrivateKey()
{
    QStandardItem *it = keysModel->itemFromIndex(ui->listViewKeys->currentIndex());
    if (!it) return;

    QSettings s;
    QString fname = QFileDialog::getSaveFileName(this, tr("Save private key"),
                                                 s.value("last_used_path/ssh_key_dir", QDir::homePath()).toString(),
                                                 tr("OpenSsh private key (*.key *.*)"));
    if (fname.isEmpty()) return;

    QFile f(fname);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
    {
        QMessageBox::warning(this, "Moolticute", tr("Failed to open file for write"));
        return;
    }

    s.setValue("last_used_path/ssh_key_dir", QFileInfo(fname).canonicalPath());

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
    QSettings s;

    QString def_dir = QDir::homePath();

    if (QDir(def_dir + "/.ssh").exists())
    {
        def_dir += "/.ssh";
    }

    const auto fname =
        QFileDialog::getOpenFileName(this, tr("OpenSSH private key"),
                                     s.value("last_used_path/ssh_key_dir", def_dir).toString(),
                                     tr("OpenSSH private key (*.key *.pem *.* *)"));
    if (fname.isEmpty())
        return;

    s.setValue("last_used_path/ssh_key_dir", QFileInfo(fname).canonicalPath());

    if (QFileInfo(fname).suffix().toLower() == "ppk")
    {
        QMessageBox::warning(this, "Moolticute",
            tr("PuTTY private keys are currently not supported!\n"
            "\n"
            "Here is workaround:\n"
            "\n"
            " * Run PuTTY Key Generator (PuTTYgen);\n"
            " * From File menu, choose 'Load private key';\n"
            " * Choose the key you tried to import now;\n"
            " * From Conversions menu, choose 'Export OpenSSH key (force new file format)';\n"
            " * Confirm saving the key without a password.\n"
            "\n"
            "Then go back to Moolticute and now you will be able to import the file"
            " you just have saved.\n"
            "\n"
            "Just then it is recommended to delete the file"
            " as it's not protected by a password.\n"
            ));
        return;
    }

    resetProgress();
    ui->stackedWidget->setCurrentWidget(ui->pageWait);
    setEnabled(false);

    currentAction = Action::ImportKey;

    sshProcess = new QProcess(this);
    QString program = Common::getMcAgent();
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

    resetProgress();
    ui->stackedWidget->setCurrentWidget(ui->pageWait);
    setEnabled(false);

    currentAction = Action::DeleteKey;

    sshProcess = new QProcess(this);
    QString program = Common::getMcAgent();
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
        setEnabled(true);
    });

    connect(sshProcess, &QProcess::readyReadStandardOutput, this, &SSHManagement::readStdOutLoadKeys);

    sshProcess->setReadChannel(QProcess::StandardOutput);
    sshProcess->start(program, arguments);
}

void SSHManagement::resetProgress()
{
    ui->progressBarLoad->setMinimum(0);
    ui->progressBarLoad->setMaximum(0);
    ui->progressBarLoad->setValue(0);
}
