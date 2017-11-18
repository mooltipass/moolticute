#include "DbBackupsTrackerController.h"

#include <QMessageBox>

#include "MainWindow.h"
#include "WSClient.h"

DbBackupsTrackerController::DbBackupsTrackerController(MainWindow* window, WSClient* wsClient, QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(window != nullptr);
    Q_ASSERT(wsClient != nullptr);

    this->window = window;
    this->wsClient = wsClient;

    connect(wsClient, &WSClient::cardDbMetadataChanged, this, &DbBackupsTrackerController::handleCardDbMetadataChanged);
    connect(&dbBackupsTracker, &DbBackupsTracker::greaterDbBackupChangeNumber, this, &DbBackupsTrackerController::handleGreaterDbBackupChangeNumber);
    connect(&dbBackupsTracker, &DbBackupsTracker::lowerDbBackupChangeNumber, this, &DbBackupsTrackerController::handleLowerDbBackupChangeNumber);
}

void DbBackupsTrackerController::handleCardDbMetadataChanged(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber)
{
    dbBackupsTracker.setCardId(cardId);
    dbBackupsTracker.setCredentialsDbChangeNumber(credentialsDbChangeNumber);
    dbBackupsTracker.setDataDbChangeNumber(dataDbChangeNumber);

    dbBackupsTracker.checkDbBackupSynchronization();
}

int DbBackupsTrackerController::askForImportBackup()
{
    QMessageBox msgBox(window);
    msgBox.setText("The db backup has changes to be imported. Do you want to import them?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();

    return ret;
}

void DbBackupsTrackerController::importDbBackup(QString data)
{
    if (!data.isEmpty()) {
        window->wantImportDatabase();
        wsClient->importDbFile(data.toLocal8Bit(), true);
    }
}

void DbBackupsTrackerController::handleGreaterDbBackupChangeNumber()
{
    int ret = askForImportBackup();

    if (ret == QMessageBox::Yes) {
        QString data = readDbBackupFile();
        importDbBackup(data);
    }
}

int DbBackupsTrackerController::askForExportBackup()
{
    QMessageBox msgBox(window);
    msgBox.setText("The db backup is outdated. Do you want to update it?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();

    return ret;
}

void DbBackupsTrackerController::exportDbBackup()
{
    connect(wsClient, &WSClient::dbExported, this, &DbBackupsTrackerController::handleExportDbResult);

    window->wantExportDatabase();
    wsClient->exportDbFile("SimpleCrypt");
}

void DbBackupsTrackerController::handleLowerDbBackupChangeNumber()
{
    int ret = askForExportBackup();
    if (ret == QMessageBox::Yes)
        exportDbBackup();
}

void DbBackupsTrackerController::writeDbBackup(QString file, const QByteArray& d)
{
    QFile f(file);
    f.open(QIODevice::WriteOnly);
    f.write(d);
    f.close();
}

void DbBackupsTrackerController::handleExportDbResult(const QByteArray& d, bool success)
{
    if (success) {
        QString file = getBackupFilePath();
        writeDbBackup(file, d);
    }
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
