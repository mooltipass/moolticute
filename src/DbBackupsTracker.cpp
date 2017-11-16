#include "DbBackupsTracker.h"

#include <QDebug>
#include <QException>
#include <QFile>
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
DbBackupsTracker::getTrackPath(const QString& cpz) const
{
    return tracks.value(cpz, "");
}

QByteArray
DbBackupsTracker::getCPZ() const
{
    return cpz;
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

int DbBackupsTracker::extractCredentialsDbChangeNumber(const QString& content) const
{
    int cn = 0;
    QJsonDocument d = QJsonDocument::fromJson(content.toLocal8Bit());
    if (d.isObject()) {
        QJsonObject root = d.object();
        if (root.contains("credentialsDbChangeNumber"))
            cn = root.value("credentialsDbChangeNumber").toInt();
    }

    return cn;
}

int DbBackupsTracker::extractDataDbChangeNumber(const QString& content) const
{
    int cn = 0;
    QJsonDocument d = QJsonDocument::fromJson(content.toLocal8Bit());
    if (d.isObject()) {
        QJsonObject root = d.object();

        if (root.contains("dataDbChangeNumber"))
            cn = root.value("dataDbChangeNumber").toInt();
    }
    return cn;
}

int DbBackupsTracker::getCredentialsDbBackupChangeNumber() const
{
    QString path = getTrackPath(cpz);
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
    if (!cpz.isEmpty())
        return tracks.contains(cpz);

    return false;
}

int DbBackupsTracker::getDataDbBackupChangeNumber() const
{
    QString path = getTrackPath(cpz);
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
    if (cpz.isEmpty()) {
        DbBackupsTrackerNoCpzSet ex;
        ex.raise();
    } else {
        watchPath(path);

        tracks.insert(cpz, path);
        emit newTrack(cpz, path);

        checkDbBackupSynchronization();
        saveTracks();
    }
}

void DbBackupsTracker::setCPZ(QByteArray cpz)
{
    if (this->cpz == cpz)
        return;

    this->cpz = cpz;
    emit cpzChanged(cpz);
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

void DbBackupsTrackerNoCpzSet::raise() const
{
    throw * this;
}

DbBackupsTrackerNoCpzSet*
DbBackupsTrackerNoCpzSet::clone() const
{
    return new DbBackupsTrackerNoCpzSet(*this);
}
