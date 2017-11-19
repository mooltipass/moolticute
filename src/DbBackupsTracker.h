#ifndef DBBACKUPSTRACKER_H
#define DBBACKUPSTRACKER_H

#include <QCryptographicHash>
#include <QException>
#include <QFileSystemWatcher>
#include <QMap>
#include <QObject>
#include <QString>

class DbBackupsTrackerNoCardIdSet : public QException
{
public:
    void raise() const;
    DbBackupsTrackerNoCardIdSet* clone() const;
};

class DbBackupsTrackerNoBackupFileSet : public QException
{
public:
    void raise() const;
    DbBackupsTrackerNoBackupFileSet* clone() const;
};

class DbBackupsTracker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString cardId READ getCardId WRITE setCardId NOTIFY cardIdChanged)
    Q_PROPERTY(
        int credentialsDbChangeNumber READ getCredentialsDbChangeNumber WRITE
            setCredentialsDbChangeNumber NOTIFY credentialsDbChangeNumberChanged)
    Q_PROPERTY(int dataDbChangeNumber READ getDataDbChangeNumber WRITE
            setDataDbChangeNumber NOTIFY dataDbChangeNumberChanged)
public:
    explicit DbBackupsTracker(QObject* parent = nullptr);
    ~DbBackupsTracker();

    QString getTrackPath(const QString& cardId) const;
    QString getCardId() const;

    int getCredentialsDbChangeNumber() const;
    int tryGetCredentialsDbBackupChangeNumber() const;
    int tryGetDataDbBackupChangeNumber() const;
    int getDataDbChangeNumber() const;

    bool isUpdateRequired() const;
    bool isBackupRequired() const;
    bool hasBackup() const;

signals:
    void cardIdChanged(QString cardId);
    void credentialsDbChangeNumberChanged(int credentialsDbChangeNumber);
    void newTrack(const QString& cardId, const QString& path);

    void greaterDbBackupChangeNumber();
    void lowerDbBackupChangeNumber();

    void dataDbChangeNumberChanged(int dataDbChangeNumber);

public slots:
    void track(const QString path);
    void setCardId(QString cardId);
    void setCredentialsDbChangeNumber(int credentialsDbChangeNumber);
    void setDataDbChangeNumber(int dataDbChangeNumber);
    void refreshTracking();

protected slots:
    void checkDbBackupSynchronization();

private:
    void saveTracks();
    void loadTracks();
    QFileSystemWatcher watcher;
    QMap<QString, QString> tracks;
    QString cardId;
    int credentialsDbChangeNumber;
    int dataDbChangeNumber;
    void watchPath(const QString path);
    QString readFile(QString path) const;
    int extractCredentialsDbChangeNumber(const QString& content) const;
    int extractDataDbChangeNumber(const QString& content) const;
    int extractCredentialsDbChangeNumberEncryptedBackup(QJsonDocument d) const;
    int extractCredentialsDbChangeNumberLegacyBackup(QJsonDocument d) const;
    int extractDataDbChangeNumberEncryptedBackup(QJsonDocument d) const;
    int extractDataDbChangeNumberLegacyBackup(QJsonDocument d) const;
};

#endif // DBBACKUPSTRACKER_H
