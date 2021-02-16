#ifndef DBBACKUPSTRACKERCONTROLLER_H
#define DBBACKUPSTRACKERCONTROLLER_H

#include <QObject>

#include "Common.h"
#include "DbBackupsTracker.h"

class WSClient;
class MainWindow;
class QMessageBox;

class DbBackupsTrackerController : public QObject
{
    Q_OBJECT
public:
    explicit DbBackupsTrackerController(MainWindow *window,
                                        WSClient *wsClient,
                                        const QString &settingsFilePath,
                                        QObject *parent = nullptr);

    QString getBackupFilePath();

    // they are not slots anymore. Will be called by dbMasterController.
    void handleCardDbMetadataChanged(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber);
    void handleExportDbResult(const QByteArray &d, bool success);
    void handleDeviceStatusChanged(const Common::MPStatus &status);
    void handleDeviceConnectedChanged(const bool &connected);

signals:
    void backupFilePathChanged(const QString &path);

public slots:
    void setBackupFilePath(const QString &path);

protected slots:
    void handleGreaterDbBackupChangeNumber();
    void handleLowerDbBackupChangeNumber();
    void handleNewTrack(const QString &cardId, const QString &path);
    void handleFirmwareVersionChange(const QString &version);

private:
    DbBackupsTracker dbBackupsTracker;
    MainWindow *window;
    WSClient *wsClient;
    bool isExportRequestMessageVisible;
    QMessageBox *askImportMessage;
    bool signalsConnected = false;

    void askForImportBackup();
    void askForExportBackup();
    void hideExportRequestIfVisible();
    void hideImportRequestIfVisible();
    QString readDbBackupFile();
    void importDbBackup(QString data);
    void exportDbBackup();
    void writeDbBackup(QString file, const QByteArray &d);
    void clearTrackerCardInfo();
    void connectDbBackupsTracker();
    void disconnectDbBackupsTracker();
};

#endif // DBBACKUPSTRACKERCONTROLLER_H
