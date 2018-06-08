#include "DbMasterController.h"
#include "AppGui.h"

#include "DbBackupsTrackerController.h"
#include "DbExportsRegistryController.h"


DbMasterController::DbMasterController(QObject *parent)
    : QObject(parent)
{
    QString regitryFile = AppGui::getDataDirPath() + "/dbExportsRegistry.ini";
    dbExportsRegistryController = new DbExportsRegistryController(regitryFile, this);
}

void DbMasterController::setMainWindow(MainWindow *window)
{
    dbExportsRegistryController->setMainWindow(window);

    if (! dbBackupsTrackerController && window && wsClient)
    {
        dbBackupsTrackerController = new DbBackupsTrackerController(window, wsClient,
                                                                    AppGui::getDataDirPath() + "/dbBackupTracks.ini", this);
        connect(dbBackupsTrackerController, SIGNAL(backupFilePathChanged(QString)),
                this, SIGNAL(backupFilePathChanged(QString)));
    }
}

void DbMasterController::setWSClient(WSClient *client)
{
    Q_ASSERT(client);

    wsClient = client;

    connect(wsClient, &WSClient::cardDbMetadataChanged, this, &DbMasterController::handleCardIdChanged);
    connect(wsClient, &WSClient::statusChanged, this, &DbMasterController::handleDeviceStatusChanged);
    connect(wsClient, &WSClient::connectedChanged, this, &DbMasterController::handleDeviceConnectedChanged);
    connect(wsClient, &WSClient::dbExported, this, &DbMasterController::registerDbExported);

    dbExportsRegistryController->setWSClient(wsClient);
}

void DbMasterController::setBackupFilePath(const QString &path)
{
    Q_ASSERT(dbBackupsTrackerController);
    dbBackupsTrackerController->setBackupFilePath(path);
}

QString DbMasterController::getBackupFilePath()
{
    if (! dbBackupsTrackerController)
        return "";

    return dbBackupsTrackerController->getBackupFilePath();
}

void DbMasterController::handleCardIdChanged(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber)
{
    if (dbBackupsTrackerController) {
        dbBackupsTrackerController->handleCardDbMetadataChanged(cardId, credentialsDbChangeNumber, dataDbChangeNumber);
        if (! dbBackupsTrackerController->getBackupFilePath().isEmpty())
            return; // monitored backup is active, no need of 'export registry' functionality
    }

    dbExportsRegistryController->handleCardIdChanged(cardId, credentialsDbChangeNumber, dataDbChangeNumber);
}

void DbMasterController::handleDeviceStatusChanged(const Common::MPStatus &status)
{
    if (dbBackupsTrackerController) {
        dbBackupsTrackerController->handleDeviceStatusChanged(status);
        if (! dbBackupsTrackerController->getBackupFilePath().isEmpty())
            return;
    }

    dbExportsRegistryController->handleDeviceStatusChanged(status);
}

void DbMasterController::handleDeviceConnectedChanged(const bool &connected)
{
    if (dbBackupsTrackerController) {
        dbBackupsTrackerController->handleDeviceConnectedChanged(connected);
        if (! dbBackupsTrackerController->getBackupFilePath().isEmpty())
            return;
    }

    dbExportsRegistryController->handleDeviceConnectedChanged(connected);
}

void DbMasterController::registerDbExported(const QByteArray &data, bool success)
{
    if (dbBackupsTrackerController) {
        dbBackupsTrackerController->handleExportDbResult(data, success);
        if (! dbBackupsTrackerController->getBackupFilePath().isEmpty())
            return;
    }

    dbExportsRegistryController->registerDbExported(data, success);
    dbExportsRegistryController->handleExportDbResult(data, success);
}
