#include "BleDev.h"
#include "ui_BleDev.h"
#include "Common.h"
#include "AppGui.h"
#include "WSClient.h"

BleDev::BleDev(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BleDev)
{
    ui->setupUi(this);
    ui->label_bundleText->setMaximumWidth(ui->label_bundleText->sizeHint().width());
    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};
    //Setting style for every button
    for (auto btn : parent->findChildren<QPushButton *>())
    {
        btn->setStyleSheet(CSS_BLUE_BUTTON);
    }

    ui->progressBarUpload->hide();
    ui->label_UploadProgress->hide();

    ui->btnFileBrowser->setIcon(AppGui::qtAwesome()->icon(fa::file, whiteButtons));
    initUITexts();
}

BleDev::~BleDev()
{
    delete ui;
}

void BleDev::setWsClient(WSClient *c)
{
    wsClient = c;
    connect(wsClient, &WSClient::displayPlatInfo, this, &BleDev::displayPlatInfoReceived);
    connect(wsClient, &WSClient::displayUploadBundleResult, this, &BleDev::displayUploadBundleResultReceived);
}

void BleDev::clearWidgets()
{
    ui->lineEditBundlePath->clear();
    ui->lineEditAuxMCUMaj->clear();
    ui->lineEditAuxMCUMin->clear();
    ui->lineEditMainMCUMaj->clear();
    ui->lineEditMainMCUMin->clear();
}

void BleDev::initUITexts()
{
    ui->label_DevTab->setText(tr("BLE Developer Tab"));
    ui->label_BLEDesc->setText(tr("BLE description"));

    ui->groupBoxUploadBundle->setTitle(tr("Upload Bundle"));
    ui->label_bundleText->setText(tr("Select Bundle File:"));
    ui->btnFileBrowser->setText(tr("Browse"));
    ui->btnUpload->setText(tr("Upload"));

    ui->groupBoxPlatInfo->setTitle(tr("Platform informations"));
    ui->label_AuxMCUMaj->setText(tr("Aux MCU major:"));
    ui->label_AuxMCUMin->setText(tr("Aux MCU minor:"));
    ui->label_MainMCUMaj->setText(tr("Main MCU major:"));
    ui->label_MainMCUMin->setText(tr("Main MCU minor:"));
    ui->btnPlatInfo->setText(tr("Get Plat Info"));

    ui->groupBoxSettings->setTitle(tr("Settings"));
    ui->label_ReflashAuxMCU->setText(tr("Flash Aux MCU:"));
    ui->btnReflashAuxMCU->setText(tr("Flash"));
    ui->label_FlashMainMCU->setText(tr("Flash Main MCU:"));
    ui->btnFlashMainMCU->setText(tr("Flash"));
}

void BleDev::on_btnFileBrowser_clicked()
{
    QSettings s;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Select bundle file"),
                                            s.value("last_used_path/bundle_dir", QDir::homePath()).toString());

    if (fileName.isEmpty())
        return;

    ui->lineEditBundlePath->setText(fileName);
    s.setValue("last_used_path/bundle_dir", QFileInfo(fileName).canonicalPath());
}

void BleDev::on_btnPlatInfo_clicked()
{
    wsClient->sendPlatInfoRequest();
}

void BleDev::displayPlatInfoReceived(int auxMajor, int auxMinor, int mainMajor, int mainMinor)
{
    ui->lineEditAuxMCUMaj->setText(QString::number(auxMajor));
    ui->lineEditAuxMCUMin->setText(QString::number(auxMinor));
    ui->lineEditMainMCUMaj->setText(QString::number(mainMajor));
    ui->lineEditMainMCUMin->setText(QString::number(mainMinor));
}

void BleDev::displayUploadBundleResultReceived(bool success)
{
    ui->label_UploadProgress->hide();
    ui->progressBarUpload->hide();
    disconnect(wsClient, &WSClient::progressChanged, this, &BleDev::updateProgress);

    const auto title = tr("Upload Bundle Result");
    if (success)
    {
        QMessageBox::information(this, title,
                                 tr("Upload bundle finished successfully."));
    }
    else
    {
        QMessageBox::critical(this, title,
                                 tr("Upload bundle finished with error."));
    }
}

void BleDev::on_btnReflashAuxMCU_clicked()
{
    wsClient->sendFlashMCU("aux");
}

void BleDev::on_btnFlashMainMCU_clicked()
{
    wsClient->sendFlashMCU("main");
}

void BleDev::on_btnUpload_clicked()
{
    QString filePath = ui->lineEditBundlePath->text();
    QFileInfo file(filePath);

    if (!file.exists() || !file.isFile())
    {
        qCritical() << filePath << " is not a file.";
        QMessageBox::warning(this, tr("Invalid path"),
                             tr("The choosen path for bundle is not a file."));
        return;
    }
    connect(wsClient, &WSClient::progressChanged, this, &BleDev::updateProgress);
    ui->progressBarUpload->show();
    ui->progressBarUpload->setMinimum(0);
    ui->progressBarUpload->setMaximum(0);
    ui->progressBarUpload->setValue(0);
    ui->label_UploadProgress->setText(tr("Starting upload bundle file."));
    ui->label_UploadProgress->show();
    wsClient->sendUploadBundle(filePath);
}

void BleDev::updateProgress(int total, int curr, QString msg)
{
    ui->progressBarUpload->setMaximum(total);
    ui->progressBarUpload->setValue(curr);
    ui->label_UploadProgress->setText(msg);
}
