#include "DbBackupsTracker.h"

#include <QDebug>
#include <QException>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTimer>
#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#include "DbBackupChangeNumbersComparator.h"

DbBackupsTracker::DbBackupsTracker(const QString settingsFilePath, QObject* parent):
    QObject(parent), settingsFilePath(settingsFilePath)
{
    loadTracks();
    connect(&watcher, &QFileSystemWatcher::fileChanged,
            this, &DbBackupsTracker::checkDbBackupSynchronization);
}

DbBackupsTracker::~DbBackupsTracker()
{
}

QString DbBackupsTracker::getTrackPath(const QString& cardId) const
{
    return tracks.value(cardId, "");
}

QString DbBackupsTracker::getCardId() const
{
    return cardId;
}

int DbBackupsTracker::getCredentialsDbChangeNumber() const
{
    return credentialsDbChangeNumber;
}

QString DbBackupsTracker::readFile(QString path) const
{
    QString content;
    QFile f(path);
    if (f.exists())
    {
        f.open(QIODevice::ReadOnly);
        content = f.readAll();
    }
    f.close();
    return content;
}

int DbBackupsTracker::extractCredentialsDbChangeNumberEncryptedBackup(const QJsonDocument &d) const
{
    QJsonObject root = d.object();
    if (root.contains("credentialsDbChangeNumber"))
        return root.value("credentialsDbChangeNumber").toInt();

    return -1;
}

int DbBackupsTracker::extractCredentialsDbChangeNumberLegacyBackup(const QJsonDocument &d) const
{
    QJsonArray root = d.array();
    QJsonValue val = root.at(root.size() - 3);
    if (val.isDouble())
        return val.toInt();

    return -1;
}

int DbBackupsTracker::extractCredentialsDbChangeNumber(const QString &content) const
{
    int cn = -1;
    QJsonDocument d = QJsonDocument::fromJson(content.toLocal8Bit());
    if (isALegacyBackup(d))
        cn = extractCredentialsDbChangeNumberLegacyBackup(d);
    else if (isAnEncryptedBackup(d))
        cn = extractCredentialsDbChangeNumberEncryptedBackup(d);

    return cn;
}

int DbBackupsTracker::extractDataDbChangeNumberEncryptedBackup(const QJsonDocument &d) const
{
    QJsonObject root = d.object();
    if (root.contains("dataDbChangeNumber"))
        return root.value("dataDbChangeNumber").toInt();

    return -1;
}

bool DbBackupsTracker::isALegacyBackup(const QJsonDocument &d) const
{
    return d.isArray();
}

bool DbBackupsTracker::isAnEncryptedBackup(const QJsonDocument &d) const
{
    return d.isObject();
}

int DbBackupsTracker::extractDataDbChangeNumberLegacyBackup(const QJsonDocument &d) const
{
    QJsonArray root = d.array();
    QJsonValue val = root.at(root.size() - 2);
    if (val.isDouble())
        return val.toInt();

    return -1;
}

int DbBackupsTracker::extractDataDbChangeNumber(const QString &content) const
{
    int cn = -1;
    QJsonDocument d = QJsonDocument::fromJson(content.toLocal8Bit());
    if (d.isObject())
        cn = extractDataDbChangeNumberEncryptedBackup(d);
    else if (d.isArray())
        cn = extractDataDbChangeNumberLegacyBackup(d);
    return cn;
}

int DbBackupsTracker::tryGetCredentialsDbBackupChangeNumber() const
{
    QString content = tryReadBackupFile();
    return extractCredentialsDbChangeNumber(content);
}

int DbBackupsTracker::getDataDbChangeNumber() const
{
    return dataDbChangeNumber;
}

bool DbBackupsTracker::isUpdateRequired() const
{
    try
    {
        int backupCCN = tryGetCredentialsDbBackupChangeNumber();
        int backupDCN = tryGetDataDbBackupChangeNumber();

        return isDbBackupChangeNumberGreater(backupCCN, backupDCN);
    }
    catch (DbBackupsTrackerNoBackupFileSet)
    {
        return false;
    }
}

bool DbBackupsTracker::isBackupRequired() const
{
    try
    {
        int backupCCN = tryGetCredentialsDbBackupChangeNumber();
        int backupDCN = tryGetDataDbBackupChangeNumber();

        return isDbBackupChangeNumberLower(backupCCN, backupDCN);
    }
    catch (DbBackupsTrackerNoBackupFileSet)
    {
        return false;
    }
}

bool DbBackupsTracker::hasBackup() const
{
    if (!cardId.isEmpty())
        return tracks.contains(cardId);

    return false;
}

QString DbBackupsTracker::getTrackedBackupFileFormat()
{
    QString content = tryReadBackupFile();
    QJsonDocument d = QJsonDocument::fromJson(content.toLocal8Bit());

    if (isALegacyBackup(d))
        return "none";

    if (isAnEncryptedBackup(d))
        return "SympleCrypt";

    return "none";
}

int DbBackupsTracker::tryGetDataDbBackupChangeNumber() const
{
    QString content = tryReadBackupFile();
    return extractDataDbChangeNumber(content);
}

void DbBackupsTracker::watchPath(const QString path)
{
    QStringList files = watcher.files();
    if (!files.isEmpty())
        watcher.removePaths(files);

    qDebug() << "Watching path " << path;
    watcher.addPath(path);
}

QString DbBackupsTracker::tryReadBackupFile() const
{
    QString path = getTrackPath(cardId);
    if (path.isEmpty())
    {
        DbBackupsTrackerNoBackupFileSet ex;
        ex.raise();
    }
    else
    {
        return readFile(path);
    }

    return "";
}

void DbBackupsTracker::track(const QString path)
{
    if (cardId.isEmpty())
    {
        DbBackupsTrackerNoCardIdSet ex;
        ex.raise();
    }
    else
    {
        watchPath(path);

        tracks.insert(cardId, path);
        emit newTrack(cardId, path);

        checkDbBackupSynchronization();
        saveTracks();
    }
}

void DbBackupsTracker::setCardId(QString cardId)
{
    if (this->cardId == cardId)
        return;

    this->cardId = cardId;
    emit cardIdChanged(cardId);
}

void DbBackupsTracker::setCredentialsDbChangeNumber(int dbChangeNumber)
{
    if (this->credentialsDbChangeNumber == dbChangeNumber)
        return;

    this->credentialsDbChangeNumber = dbChangeNumber;
    emit credentialsDbChangeNumberChanged(this->credentialsDbChangeNumber);
}

void DbBackupsTracker::setDataDbChangeNumber(int dataDbChangeNumber)
{
    if (this->dataDbChangeNumber == dataDbChangeNumber)
        return;

    this->dataDbChangeNumber = dataDbChangeNumber;
    emit dataDbChangeNumberChanged(this->dataDbChangeNumber);
}


bool DbBackupsTracker::isDbBackupChangeNumberGreater(int backupCCN, int backupDCN) const
{
    bool result = false;
    result = result || BackupChangeNumbersComparator::greaterThanWithWrapOver(backupCCN, credentialsDbChangeNumber);
    result = result || BackupChangeNumbersComparator::greaterThanWithWrapOver(backupDCN, dataDbChangeNumber);
    return result;
}

bool DbBackupsTracker::isDbBackupChangeNumberLower(int backupCCN, int backupDCN) const
{
    bool result = false;
    result = result || BackupChangeNumbersComparator::lowerThanWithWrapOver(backupCCN, credentialsDbChangeNumber);
    result = result || BackupChangeNumbersComparator::lowerThanWithWrapOver(backupDCN, dataDbChangeNumber);
    return result;
}

void DbBackupsTracker::checkDbBackupSynchronization()
{
    try
    {
        int backupCCN = tryGetCredentialsDbBackupChangeNumber();
        int backupDCN = tryGetDataDbBackupChangeNumber();

        // < 0 values are a failure to read the file.
        if (backupCCN < 0 || backupDCN < 0)
            return;

        qDebug () << "Backup file changed: " << backupCCN << " - " << backupDCN;

        if (isDbBackupChangeNumberGreater(backupCCN, backupDCN))
            emit greaterDbBackupChangeNumber();

        if (isDbBackupChangeNumberLower(backupCCN, backupDCN))
            emit lowerDbBackupChangeNumber();
    }
    catch (DbBackupsTrackerNoBackupFileSet)
    {
        qDebug() << "No backup file for " << cardId;
    }
}

void DbBackupsTracker::refreshTracking()
{
    if (tracks.contains(cardId))
        track(tracks.value(cardId));
}

void DbBackupsTracker::saveTracks()
{
    QSettings s(settingsFilePath, QSettings::IniFormat);
    s.beginGroup("BackupsTracks");
    for (QString k : tracks.keys())
        s.setValue(k, tracks.value(k));

    s.endGroup();

    s.sync();
    if (s.status() != QSettings::NoError)
        qWarning() << "Unable to save settings " << s.status();
}

void DbBackupsTracker::loadTracks()
{
    tracks.clear();

    QSettings s(settingsFilePath, QSettings::IniFormat);
    s.sync();
    if (s.status() != QSettings::NoError)
        qWarning() << "Unable to load settings " << s.status();

    s.beginGroup("BackupsTracks");
    for (QString k : s.allKeys())
    {
        QString v = s.value(k).toString();
        tracks.insert(k, v);
        emit newTrack(k, v);

        if (k.compare(cardId) == 0)
            watchPath(v);
    }

    s.endGroup();
}

void DbBackupsTrackerNoCardIdSet::raise() const
{
    throw *this;
}

DbBackupsTrackerNoCardIdSet *DbBackupsTrackerNoCardIdSet::clone() const
{
    return new DbBackupsTrackerNoCardIdSet(*this);
}

void DbBackupsTrackerNoBackupFileSet::raise() const
{
    throw *this;
}

DbBackupsTrackerNoBackupFileSet *DbBackupsTrackerNoBackupFileSet::clone() const
{
    return new DbBackupsTrackerNoBackupFileSet(*this);
}
