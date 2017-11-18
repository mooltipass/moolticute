#ifndef DBBACKUPSTRACKERCONTROLLER_H
#define DBBACKUPSTRACKERCONTROLLER_H

#include <QObject>

#include "DbBackupsTracker.h"

class WSClient;
class MainWindow;
class DbBackupsTrackerController : public QObject {
    Q_OBJECT
public:
    explicit DbBackupsTrackerController(MainWindow* window,
        WSClient* wsClient,
        QObject* parent = nullptr);

signals:

public slots:

protected slots:
    void handleCardDbMetadataChanged(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber);
    void handleGreaterDbBackupChangeNumber();
    void handleLowerDbBackupChangeNumber();
    void handleExportDbResult(const QByteArray &d, bool success);
private:
    DbBackupsTracker dbBackupsTracker;
    MainWindow* window;
    WSClient* wsClient;

    int askForImportBackup();
    int askForExportBackup();
    QString readDbBackupFile();
    void importDbBackup(QString data);
    void exportDbBackup();
    QString getBackupFilePath();
    void writeDbBackup(QString file, const QByteArray& d);
};

#endif // DBBACKUPSTRACKERCONTROLLER_H
