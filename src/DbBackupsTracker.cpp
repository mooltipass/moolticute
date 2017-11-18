#include "DbBackupsTracker.h"

#include <QDebug>
#include <QException>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

DbBackupsTracker::DbBackupsTracker(QObject* parent)
    : QObject(parent)
{
    loadTracks();

    connect(&watcher, &QFileSystemWatcher::fileChanged,
        this, &DbBackupsTracker::checkDbBackupSynchronization);
}

DbBackupsTracker::~DbBackupsTracker()
{
}

QString
DbBackupsTracker::getTrackPath(const QString& cardId) const
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
    if (f.exists()) {
        f.open(QIODevice::ReadOnly);
        content = f.readAll();
    }
    f.close();
    return content;
}

int DbBackupsTracker::extractCredentialsDbChangeNumberEncryptedBackup(QJsonDocument d) const
{
    QJsonObject root = d.object();
    if (root.contains("credentialsDbChangeNumber"))
        return root.value("credentialsDbChangeNumber").toInt();

    return 0;
}

int DbBackupsTracker::extractCredentialsDbChangeNumberLegacyBackup(QJsonDocument d) const
{
    QJsonArray root = d.array();
    QJsonValue val = root.at(root.size() - 3);
    if (val.isDouble())
        return val.toInt();

    return 0;
}

int DbBackupsTracker::extractCredentialsDbChangeNumber(const QString& content) const
{
    int cn = 0;
    QJsonDocument d = QJsonDocument::fromJson(content.toLocal8Bit());
    if (d.isObject())
        cn = extractCredentialsDbChangeNumberEncryptedBackup(d);
    else if (d.isArray())
        cn = extractCredentialsDbChangeNumberLegacyBackup(d);

    return cn;
}

int DbBackupsTracker::extractDataDbChangeNumberEncryptedBackup(QJsonDocument d) const
{
    QJsonObject root = d.object();
    if (root.contains("dataDbChangeNumber"))
        return root.value("dataDbChangeNumber").toInt();

    return 0;
}

int DbBackupsTracker::extractDataDbChangeNumberLegacyBackup(QJsonDocument d) const
{
    QJsonArray root = d.array();
    QJsonValue val = root.at(root.size() - 2);
    if (val.isDouble())
        return val.toInt();

    return 0;
}

int DbBackupsTracker::extractDataDbChangeNumber(const QString& content) const
{
    int cn = 0;
    QJsonDocument d = QJsonDocument::fromJson(content.toLocal8Bit());
    if (d.isObject())
        cn = extractDataDbChangeNumberEncryptedBackup(d);
    else if (d.isArray())
        cn = extractDataDbChangeNumberLegacyBackup(d);
    return cn;
}

int DbBackupsTracker::getCredentialsDbBackupChangeNumber() const
{
    QString path = getTrackPath(cardId);
    QString content = readFile(path);

    return extractCredentialsDbChangeNumber(content);
}

int DbBackupsTracker::getDataDbChangeNumber() const
{
    return dataDbChangeNumber;
}

bool DbBackupsTracker::isUpdateRequired() const
{
    int backupCCN = getCredentialsDbBackupChangeNumber();
    int backupDCN = getDataDbBackupChangeNumber();

    return (backupCCN > credentialsDbChangeNumber || backupDCN > dataDbChangeNumber);
}

bool DbBackupsTracker::isBackupRequired() const
{
    int backupCCN = getCredentialsDbBackupChangeNumber();
    int backupDCN = getDataDbBackupChangeNumber();

    return (backupCCN < credentialsDbChangeNumber || backupDCN < dataDbChangeNumber);
}

bool DbBackupsTracker::hasBackup() const
{
    if (!cardId.isEmpty())
        return tracks.contains(cardId);

    return false;
}

int DbBackupsTracker::getDataDbBackupChangeNumber() const
{
    QString path = getTrackPath(cardId);
    QString content = readFile(path);

    return extractDataDbChangeNumber(content);
}

void DbBackupsTracker::watchPath(const QString path)
{
    watcher.removePaths(watcher.files());
    watcher.addPath(path);
}

void DbBackupsTracker::track(const QString path)
{
    if (cardId.isEmpty()) {
        DbBackupsTrackerNoCardIdSet ex;
        ex.raise();
    } else {
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

void DbBackupsTracker::checkDbBackupSynchronization()
{
    int backupCCN = getCredentialsDbBackupChangeNumber();
    int backupDCN = getDataDbBackupChangeNumber();

    if (backupCCN > credentialsDbChangeNumber || backupDCN > dataDbChangeNumber)
        emit greaterDbBackupChangeNumber();

    if (backupCCN < credentialsDbChangeNumber || backupDCN < dataDbChangeNumber)
        emit lowerDbBackupChangeNumber();
}

void DbBackupsTracker::saveTracks()
{
    QSettings s;
    s.beginGroup("BackupsTracks");
    for (QString k : tracks.keys())
        s.setValue(k, tracks.value(k));

    s.endGroup();
}

void DbBackupsTracker::loadTracks()
{
    tracks.clear();

    QSettings s;
    s.beginGroup("BackupsTracks");
    for (QString k : s.allKeys()) {
        QString v = s.value(k).toString();
        tracks.insert(k, v);
    }

    s.endGroup();
}

void DbBackupsTrackerNoCardIdSet::raise() const
{
    throw * this;
}

DbBackupsTrackerNoCardIdSet*
DbBackupsTrackerNoCardIdSet::clone() const
{
    return new DbBackupsTrackerNoCardIdSet(*this);
}
