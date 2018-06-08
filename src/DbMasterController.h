#ifndef DBMASTERCONTROLLER_H
#define DBMASTERCONTROLLER_H

#include <QObject>
#include "Common.h"

class DbExportsRegistryController;
class DbBackupsTrackerController;
class MainWindow;
class WSClient;


/**
 * This class is a coordinator for DbBackupsTrackerController and DbExportsRegistryController.
 *
 * That classes were created without any knowledge about each other.
 * But we need to disable DbExportsRegistryController (periodical backup creation)
 * in a case if monitored backup is configured (done by DbBackupsTrackerController).
 *
 * So the aim of this class is following:
 * to receive all related signals from WSClient and pass them to
 * DbBackupsTrackerController first, then, if a monitored backup is not configured
 * for current CardId, to DbExportsRegistryController.
 *
 * This way we will avoid simultaniously and uncoordinated use of promptWidget in MainWindow.
 */
class DbMasterController : public QObject
{
    Q_OBJECT
public:
    explicit DbMasterController(QObject *parent = nullptr);

    void setMainWindow(MainWindow *window);
    void setWSClient(WSClient *client);

    void setBackupFilePath(const QString &path);
    QString getBackupFilePath();


signals:
    void backupFilePathChanged(const QString &path);

private slots:
    void handleCardIdChanged(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber);
    void handleDeviceStatusChanged(const Common::MPStatus &status);
    void handleDeviceConnectedChanged(const bool &connected);
    void registerDbExported(const QByteArray &data, bool success);

private:
    MainWindow *window = nullptr;
    WSClient *wsClient = nullptr;

    DbExportsRegistryController *dbExportsRegistryController = nullptr;
    DbBackupsTrackerController *dbBackupsTrackerController = nullptr;
};

#endif // DBMASTERCONTROLLER_H
