#include "BleDev.h"
#include "ui_BleDev.h"
#include "Common.h"
#include "AppGui.h"
#include "WSClient.h"
#include "DeviceDetector.h"

const QString BleDev::HEXA_CHAR_REGEXP = "[^A-Fa-f0-9*]";
const QByteArray BleDev::START_BUNDLE_BYTES = "\x78\x56\x34\x12";
const QStringList BleDev::ACCEPTED_BUNDLE_FILENAMES = {
    "bundle_not_numbered.img",
    "bundle_numbered_with_white_silkscreen.img",
    "miniblebundle.img"
  };

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
    ui->progressBarUpload->hide();
    ui->label_UploadProgress->hide();
}

void BleDev::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift)
    {
        DeviceDetector::instance().shiftPressed();
    }
    if (event->key() == Qt::Key_Control)
    {
        DeviceDetector::instance().ctrlPressed();
    }
}

void BleDev::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift)
    {
        DeviceDetector::instance().shiftReleased();
    }
    if (event->key() == Qt::Key_Control)
    {
        DeviceDetector::instance().ctrlReleased();
    }
}

void BleDev::initUITexts()
{
    const auto browseText = tr("Browse");
    ui->label_DevTab->setText(tr("BLE Developer Tab"));

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

bool BleDev::isValidBundleFile(QFile* file) const
{
    for (int i = 0; i < START_BUNDLE_BYTES.size(); ++i)
    {
        char data;
        file->read(&data, sizeof(char));
        if (data != START_BUNDLE_BYTES[i])
        {
            return false;
        }
    }

    return true;
}

bool BleDev::checkBundleFilePassword(const QFileInfo& fileInfo, QString &password, bool skipDeviceChecks /* =false */)
{
    const auto baseFilename = fileInfo.baseName();
    const auto baseNameWithExt = fileInfo.fileName();
    if (!ACCEPTED_BUNDLE_FILENAMES.contains(baseNameWithExt))
    {
        const auto INVALID_BUNDLE_TEXT = tr("Invalid bundle file is selected");
        auto fileParts = baseFilename.split("_");
        if (fileParts.size() != BUNDLE_FILE_PARTS_SIZE)
        {
            QMessageBox::warning(this, INVALID_BUNDLE_TEXT,
                                 tr("The given bundle file name is not correct."));
            return false;
        }
        const auto serial = fileParts[SERIAL_FILE_PART].toUInt();
        if (!skipDeviceChecks && wsClient->get_hwSerial() != serial)
        {
            QMessageBox::warning(this, INVALID_BUNDLE_TEXT,
                                 tr("The device serial number is not correct in bundle filename."));
            return false;
        }
        const auto bundleVersion = fileParts[BUNDLE_FILE_PART].toInt();
        if (!skipDeviceChecks && wsClient->get_bundleVersion() != bundleVersion)
        {
            QMessageBox::warning(this, INVALID_BUNDLE_TEXT,
                                 tr("The bundle version is not correct in bundle filename."));
            return false;
        }
        password = fileParts.last();
    }
    return true;
}

void BleDev::on_btnFileBrowser_clicked()
{
    if (wsClient->get_status() != Common::NoBundle &&
        (DeviceDetector::instance().isConnectedWithBluetooth() ||
        (DeviceDetector::instance().getBattery() < MIN_BATTERY_PCT_FOR_UPLOAD && !DeviceDetector::instance().isShiftPressed())))
    {
        QMessageBox::warning(this, tr("Battery too low for bundle upload"),
                             tr("Please have your device connected through USB and fully charged"));
        return;
    }
    QSettings s;

    bool skipDeviceChecks = wsClient->get_status() == Common::NoBundle &&
                                DeviceDetector::instance().isCtrlPressed();

    QString fileName = AppGui::getFileName(this, tr("Select bundle file"),
                                            s.value("last_used_path/bundle_dir", QDir::homePath()).toString(),
                                            "*.img");

    // Due to file selection dialog opened, release events are not caught
    DeviceDetector::instance().shiftReleased();
    DeviceDetector::instance().ctrlReleased();

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

    bool validBundle = isValidBundleFile(&file);
    if (!validBundle)
    {
        QMessageBox::warning(this, tr("Invalid bundle file is selected"),
                             tr("The given bundle file is not correct."));
        return;
    }

    QString password = "";
    if (!checkBundleFilePassword(QFileInfo{file}, password, skipDeviceChecks))
    {
        qCritical() << "Invalid bundle file password";
        return;
    }

    connect(wsClient, &WSClient::progressChanged, this, &BleDev::updateProgress);
    ui->progressBarUpload->show();
    ui->progressBarUpload->setMinimum(0);
    ui->progressBarUpload->setMaximum(0);
    ui->progressBarUpload->setValue(0);
    ui->label_UploadProgress->setText(tr("Starting upload bundle file."));
    ui->label_UploadProgress->show();
    wsClient->sendUploadBundle(fileName, password);
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

    QString fileName = AppGui::getSaveFileName(this, tr("Select file to fetch data"),
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
