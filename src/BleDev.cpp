#include "BleDev.h"
#include "ui_BleDev.h"
#include "Common.h"
#include "AppGui.h"
#include "WSClient.h"

const QString BleDev::HEXA_CHAR_REGEXP = "[^A-Fa-f0-9*]";

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
    ui->btnFetchDataBrowse->setIcon(AppGui::qtAwesome()->icon(fa::file, whiteButtons));
    ui->progressBarFetchData->setVisible(false);
    ui->horizontalLayout_Fetch->setAlignment(Qt::AlignLeft);
    ui->progressBarFetchData->setMinimum(0);
    ui->progressBarFetchData->setMaximum(0);
    ui->labelInvalidBundle->setStyleSheet("QLabel { color : red; }");
    ui->labelInvalidBundle->hide();

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
    connect(wsClient, &WSClient::displayUploadBundleResult, this, &BleDev::displayUploadBundleResultReceived);
}

void BleDev::clearWidgets()
{
    ui->lineEditBundlePassword->clear();
}

void BleDev::initUITexts()
{
    const auto browseText = tr("Browse");
    ui->label_DevTab->setText(tr("BLE Developer Tab"));
    ui->label_BLEDesc->setText(tr("BLE description"));

    ui->groupBoxUploadBundle->setTitle(tr("Bundle Settings"));

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

bool BleDev::checkBundlePassword(QFile* file) const
{
    int size = 0;
    QByteArray arr;
    while(size < CHECK_BUNDLE_BYTE_SIZE)
    {
        char data;
        file->read(&data, sizeof(char));
        arr.append(data);
        ++size;
    }

    QString bundlePassword = ui->lineEditBundlePassword->text();
    if (bundlePassword.size() == CHECK_BUNDLE_BYTE_SIZE * 2)
    {
        const int BASE = 16;
        for (int i = 0; i < CHECK_BUNDLE_BYTE_SIZE; ++i)
        {
            bool ok = false;
            auto passwordChar = bundlePassword.mid(i*2, 2).toUInt(&ok, BASE);
            if (!ok || arr.at(i) != static_cast<char>(passwordChar))
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
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

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical() << fileName << " is not a file.";
        QMessageBox::warning(this, tr("Invalid path"),
                             tr("The choosen path for bundle is not a file."));
        return;
    }

    bool bundlePasswordOk = checkBundlePassword(&file);
    if (!bundlePasswordOk)
    {
        QMessageBox::warning(this, tr("Invalid bundle password"),
                             tr("The given password for bundle is not correct."));
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

void BleDev::displayUploadBundleResultReceived(bool success)
{
    ui->label_UploadProgress->hide();
    ui->progressBarUpload->hide();

    disconnect(wsClient, &WSClient::progressChanged, this, &BleDev::updateProgress);

    const auto title = tr("Upload Bundle Result");
    if (success)
    {
        // Update platform after successful upload
        wsClient->sendFlashMCU();
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

void BleDev::on_lineEditBundlePassword_textChanged(const QString &arg1)
{
    if (arg1.contains(QRegExp(HEXA_CHAR_REGEXP)))
    {
        ui->btnFileBrowser->setEnabled(false);
        ui->labelInvalidBundle->show();
    }
    else
    {
        ui->btnFileBrowser->setEnabled(true);
        ui->labelInvalidBundle->hide();
    }
}
