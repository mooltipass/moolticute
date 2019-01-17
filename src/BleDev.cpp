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
    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};
    //Setting style for every button
    for (auto btn : parent->findChildren<QPushButton *>())
    {
        btn->setStyleSheet(CSS_BLUE_BUTTON);
    }
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
                                            s.value("last_used_path/load_file_dir", QDir::homePath()).toString());

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

void BleDev::on_btnReflashAuxMCU_clicked()
{
    wsClient->sendFlashAuxMCU();
}
