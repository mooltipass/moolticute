#include "DbBackupsTrackerController.h"

#include <QMessageBox>

#include "MainWindow.h"
#include "PromptWidget.h"
#include "WSClient.h"

void DbBackupsTrackerController::connectDbBackupsTracker()
{
    connect(&dbBackupsTracker, &DbBackupsTracker::greaterDbBackupChangeNumber,
            this, &DbBackupsTrackerController::handleGreaterDbBackupChangeNumber);
    connect(&dbBackupsTracker, &DbBackupsTracker::lowerDbBackupChangeNumber,
            this, &DbBackupsTrackerController::handleLowerDbBackupChangeNumber);
    connect(&dbBackupsTracker, &DbBackupsTracker::newTrack,
            this, &DbBackupsTrackerController::handleNewTrack);
}

void DbBackupsTrackerController::disconnectDbBackupsTracker()
{
    disconnect(&dbBackupsTracker, &DbBackupsTracker::greaterDbBackupChangeNumber,
               this, &DbBackupsTrackerController::handleGreaterDbBackupChangeNumber);
    disconnect(&dbBackupsTracker, &DbBackupsTracker::lowerDbBackupChangeNumber,
               this, &DbBackupsTrackerController::handleLowerDbBackupChangeNumber);
    disconnect(&dbBackupsTracker, &DbBackupsTracker::newTrack,
               this, &DbBackupsTrackerController::handleNewTrack);
}

DbBackupsTrackerController::DbBackupsTrackerController(MainWindow *window, WSClient *wsClient, const QString &settingsFilePath, QObject *parent):
    QObject(parent),
    dbBackupsTracker(settingsFilePath),
    isExportRequestMessageVisible(false),
    askImportMessage(nullptr)
{
    Q_ASSERT(window != nullptr);
    Q_ASSERT(wsClient != nullptr);

    this->window = window;
    this->wsClient = wsClient;

    connect(wsClient, &WSClient::fwVersionChanged,
            this, &DbBackupsTrackerController::handleFirmwareVersionChange);
    connect(wsClient, &WSClient::cardDbMetadataChanged,
            this, &DbBackupsTrackerController::handleCardDbMetadataChanged);
    connect(wsClient, &WSClient::statusChanged,
            this, &DbBackupsTrackerController::handleDeviceStatusChanged);
    connect(wsClient, &WSClient::connectedChanged,
            this, &DbBackupsTrackerController::handleDeviceConnectedChanged);

    handleFirmwareVersionChange(wsClient->get_fwVersion());
    handleDeviceStatusChanged(wsClient->get_status());
    handleDeviceConnectedChanged(wsClient->isConnected());

    //Delay the init in the next event loop call. Not in constructor.
    QTimer::singleShot(0, [=]()
    {
        handleCardDbMetadataChanged(wsClient->get_cardId(),
                                    wsClient->get_credentialsDbChangeNumber(),
                                    wsClient->get_dataDbChangeNumber());
    });
}

void DbBackupsTrackerController::setBackupFilePath(const QString &path)
{
    hideExportRequestIfVisible();
    hideImportRequestIfVisible();
    try
    {
        dbBackupsTracker.track(path);
    }
    catch (DbBackupsTrackerNoCardIdSet)
    {
        // Just ignore it
    }
}

void DbBackupsTrackerController::handleCardDbMetadataChanged(QString cardId,
                                                             int credentialsDbChangeNumber,
                                                             int dataDbChangeNumber)
{
    qDebug() << "Card meta data changed " << cardId << " - " << credentialsDbChangeNumber << " - " << dataDbChangeNumber;
    window->hidePrompt();

    dbBackupsTracker.setCardId(cardId);
    dbBackupsTracker.setCredentialsDbChangeNumber(credentialsDbChangeNumber);
    dbBackupsTracker.setDataDbChangeNumber(dataDbChangeNumber);

    dbBackupsTracker.refreshTracking();

    QString path = dbBackupsTracker.getTrackPath(cardId);
    emit backupFilePathChanged(path);
}

void DbBackupsTrackerController::askForImportBackup()
{
    askImportMessage = new QMessageBox(window);
    askImportMessage->setWindowTitle(tr("Import db backup"));
    askImportMessage->setText(tr("Credentials in the backup file are more recent. "
                                 "Do you want to import credentials to the device?"));

    askImportMessage->addButton(QMessageBox::Yes);
    askImportMessage->addButton(QMessageBox::No);
    askImportMessage->setDefaultButton(QMessageBox::No);
    askImportMessage->setModal(true);

    int btn  = askImportMessage->exec();
    askImportMessage->deleteLater();
    askImportMessage = nullptr;

    if (btn == QMessageBox::Yes)
    {
        QString data = readDbBackupFile();
        importDbBackup(data);
    }
}

void DbBackupsTrackerController::importDbBackup(QString data)
{
    if (!data.isEmpty())
    {
        connect(wsClient, &WSClient::dbImported,
                window, &MainWindow::handleBackupImported);

        window->wantImportDatabase();
        wsClient->importDbFile(data.toLocal8Bit(), true);
    }
}

void DbBackupsTrackerController::handleGreaterDbBackupChangeNumber()
{
    qDebug() << "Backup file is greater than device";

    hideExportRequestIfVisible();
    hideImportRequestIfVisible();
    askForImportBackup();
}

void DbBackupsTrackerController::askForExportBackup()
{
    std::function<void()> onAccept = [this]()
    {
        exportDbBackup();
        isExportRequestMessageVisible = false;
    };

    std::function<void()> onReject = [this]()
    {
        QMessageBox::StandardButton btn = QMessageBox::warning(
                                              window, tr("Be careful"),
                                              tr("By denying you can lose your changes. Do you want to continue?"),
                                              QMessageBox::Yes | QMessageBox::No);

        if (btn == QMessageBox::Yes)
        {
            window->hidePrompt();
            isExportRequestMessageVisible = false;
        }
    };

    PromptMessage *message = new PromptMessage(tr("Credentials on the device are more recent. "
                                                  "Do you want export credentials to backup file?"),
                                               onAccept, onReject);
    window->showPrompt(message);
    isExportRequestMessageVisible = true;
}

void DbBackupsTrackerController::exportDbBackup()
{
    connect(wsClient, &WSClient::dbExported, this,
            &DbBackupsTrackerController::handleExportDbResult);

    QString format;
    try
    {
        format = dbBackupsTracker.getTrackedBackupFileFormat();
    }
    catch (DbBackupsTrackerNoBackupFileSet)
    {
        format = "SympleCrypt";
    }


    window->wantExportDatabase();
    wsClient->exportDbFile(format);
}

void DbBackupsTrackerController::handleLowerDbBackupChangeNumber()
{
    qDebug() << "Device is newer than backup";

    hideImportRequestIfVisible();
    hideExportRequestIfVisible();
    askForExportBackup();
}

void DbBackupsTrackerController::writeDbBackup(QString file, const QByteArray& d)
{
    QFile f(file);
    f.open(QIODevice::WriteOnly);
    f.write(d);
    f.close();
}

void DbBackupsTrackerController::clearTrackerCardInfo()
{
    dbBackupsTracker.setCardId("");
    dbBackupsTracker.setDataDbChangeNumber(0);
    dbBackupsTracker.setCredentialsDbChangeNumber(0);
}


void DbBackupsTrackerController::handleExportDbResult(const QByteArray &d, bool success)
{
    if (success)
    {
        QString file = getBackupFilePath();
        writeDbBackup(file, d);

        window->handleBackupExported();
    }
    else
    {
        QMessageBox::warning(window, tr("Error"), tr(d));
    }
}

void DbBackupsTrackerController::handleNewTrack(const QString &cardId, const QString &path)
{
    QString currentCardId = dbBackupsTracker.getCardId();
    if (currentCardId.compare(cardId) == 0)
        emit backupFilePathChanged(path);
    else
        emit backupFilePathChanged(QString());
}

void DbBackupsTrackerController::handleDeviceStatusChanged(const Common::MPStatus &status)
{
    if (status != Common::Unlocked)
    {
        clearTrackerCardInfo();

        hideExportRequestIfVisible();
        hideImportRequestIfVisible();
    }
}

void DbBackupsTrackerController::handleDeviceConnectedChanged(const bool &)
{
    clearTrackerCardInfo();

    hideExportRequestIfVisible();
    hideImportRequestIfVisible();
}

void DbBackupsTrackerController::handleFirmwareVersionChange(const QString &)
{
    if (wsClient->isFw12())
    {
        qDebug() << "Setup db tracking";
        window->showDbBackTrackingControls(true);
        connectDbBackupsTracker();
    }
    else
    {
        qDebug() << "Db tracking is not available for fw < 1.2";
        window->showDbBackTrackingControls(false);
        disconnectDbBackupsTracker();
    }
}

void DbBackupsTrackerController::hideExportRequestIfVisible()
{
    if (isExportRequestMessageVisible)
    {
        window->hidePrompt();
        isExportRequestMessageVisible = false;
    }
}

void DbBackupsTrackerController::hideImportRequestIfVisible()
{
    if (askImportMessage != nullptr)
        askImportMessage->reject();
}

QString DbBackupsTrackerController::getBackupFilePath()
{
    QString id = dbBackupsTracker.getCardId();
    QString file = dbBackupsTracker.getTrackPath(id);

    return file;
}

QString DbBackupsTrackerController::readDbBackupFile()
{
    QString file = getBackupFilePath();

    QFile f(file);
    f.open(QIODevice::ReadOnly);
    QString data = f.readAll();
    f.close();

    return data;
}
