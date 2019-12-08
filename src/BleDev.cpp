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

    ui->progressBarUpload->hide();
    ui->label_UploadProgress->hide();

#ifdef Q_OS_WIN
    ui->btnFileBrowser->setMinimumWidth(140);
#endif
    ui->btnFileBrowser->setIcon(AppGui::qtAwesome()->icon(fa::file, whiteButtons));
    ui->btnUpdatePlatform->setIcon(AppGui::qtAwesome()->icon(fa::upload, whiteButtons));
    ui->btnFetchDataBrowse->setIcon(AppGui::qtAwesome()->icon(fa::file, whiteButtons));
    ui->progressBarFetchData->setVisible(false);
    ui->horizontalLayout_Fetch->setAlignment(Qt::AlignLeft);
    ui->progressBarFetchData->setMinimum(0);
    ui->progressBarFetchData->setMaximum(0);

#if defined(Q_OS_LINUX)
    ui->label_FetchDataFile->setMaximumWidth(150);
#endif
    initUITexts();
}

BleDev::~BleDev()
{
    delete ui;
}

void BleDev::setWsClient(WSClient *c)
{
    wsClient = c;
    connect(wsClient, &WSClient::displayDebugPlatInfo, this, &BleDev::displayDebugPlatInfoReceived);
    connect(wsClient, &WSClient::displayUploadBundleResult, this, &BleDev::displayUploadBundleResultReceived);
}

void BleDev::clearWidgets()
{
    ui->lineEditAuxMCUMaj->clear();
    ui->lineEditAuxMCUMin->clear();
    ui->lineEditMainMCUMaj->clear();
    ui->lineEditMainMCUMin->clear();
}

void BleDev::initUITexts()
{
    const auto browseText = tr("Browse");
    ui->label_DevTab->setText(tr("BLE Developer Tab"));
    ui->label_BLEDesc->setText(tr("BLE description"));

    ui->groupBoxUploadBundle->setTitle(tr("Bundle Settings"));

    ui->groupBoxPlatInfo->setTitle(tr("Platform informations"));
    ui->label_AuxMCUMaj->setText(tr("Aux MCU major:"));
    ui->label_AuxMCUMin->setText(tr("Aux MCU minor:"));
    ui->label_MainMCUMaj->setText(tr("Main MCU major:"));
    ui->label_MainMCUMin->setText(tr("Main MCU minor:"));
    ui->btnPlatInfo->setText(tr("Get Plat Info"));

    ui->groupBoxFetchData->setTitle(tr("Data Fetch"));
    ui->label_FetchDataFile->setText(tr("Storage file:"));
    ui->btnFetchDataBrowse->setText(browseText);
    ui->btnFetchAccData->setText(FETCH_ACC_DATA_TEXT);
    ui->btnFetchRandomData->setText(FETCH_RANDOM_DATA_TEXT);
}

void BleDev::fetchData(const Common::FetchType &fetchType)
{
    QString fileName = ui->lineEditFetchData->text();
    if (fileName.isEmpty())
    {
        return;
    }
    bool isAccFetch = Common::FetchType::ACCELEROMETER == fetchType;
    auto& selectedButton = isAccFetch ? ui->btnFetchAccData : ui->btnFetchRandomData;
    auto& inactiveButton = isAccFetch ? ui->btnFetchRandomData : ui->btnFetchAccData;
    if (Common::FetchState::STOPPED == fetchState)
    {
        ui->progressBarFetchData->show();
        selectedButton->setText(tr("Stop Fetch"));
        inactiveButton->hide();
        fetchState = Common::FetchState::STARTED;
        wsClient->sendFetchData(fileName, fetchType);
    }
    else
    {
        ui->progressBarFetchData->hide();
        selectedButton->setText(isAccFetch ? FETCH_ACC_DATA_TEXT : FETCH_RANDOM_DATA_TEXT);
        inactiveButton->show();
        fetchState = Common::FetchState::STOPPED;
        wsClient->sendStopFetchData();
    }
}

void BleDev::on_btnFileBrowser_clicked()
{
    QSettings s;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Select bundle file"),
                                            s.value("last_used_path/bundle_dir", QDir::homePath()).toString(),
                                            "*.img");

    if (fileName.isEmpty())
    {
        return;
    }

    s.setValue("last_used_path/bundle_dir", QFileInfo(fileName).canonicalPath());

    QFileInfo file(fileName);

    if (!file.exists() || !file.isFile())
    {
        qCritical() << fileName << " is not a file.";
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
    wsClient->sendUploadBundle(fileName);
}

void BleDev::on_btnPlatInfo_clicked()
{
    wsClient->sendPlatInfoRequest();
}

void BleDev::displayDebugPlatInfoReceived(int auxMajor, int auxMinor, int mainMajor, int mainMinor)
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



void BleDev::updateProgress(int total, int curr, QString msg)
{
    ui->progressBarUpload->setMaximum(total);
    ui->progressBarUpload->setValue(curr);
    ui->label_UploadProgress->setText(msg);
}

void BleDev::on_btnFetchDataBrowse_clicked()
{
    QSettings s;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Select file to fetch data"),
                                            s.value("last_used_path/fetchdata_dir", QDir::homePath()).toString(),
                                            "*.bin");

    if (fileName.isEmpty())
    {
        return;
    }
#if defined(Q_OS_LINUX)
            /**
             * getSaveFileName is using native dialog
             * On Linux it is not saving the choosen extension,
             * so need to add it from code.
             */
            const QString BIN_EXT = ".bin";
            if (!fileName.endsWith(BIN_EXT))
            {
                fileName += BIN_EXT;
            }
#endif

    ui->lineEditFetchData->setText(fileName);
    s.setValue("last_used_path/fetchdata_dir", fileName.mid(0, fileName.lastIndexOf('/')));
}

void BleDev::on_btnFetchAccData_clicked()
{
    fetchData(Common::FetchType::ACCELEROMETER);
}

void BleDev::on_btnFetchRandomData_clicked()
{
    fetchData(Common::FetchType::RANDOM_BYTES);
}

void BleDev::on_btnUpdatePlatform_clicked()
{
    wsClient->sendFlashMCU();
}
