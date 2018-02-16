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
    DbBackupsTrackerNoCardIdSet *clone() const;
};

class DbBackupsTrackerNoBackupFileSet : public QException
{
public:
    void raise() const;
    DbBackupsTrackerNoBackupFileSet *clone() const;
};

class DbBackupsTracker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString cardId READ getCardId WRITE setCardId NOTIFY cardIdChanged)
    Q_PROPERTY(int credentialsDbChangeNumber READ getCredentialsDbChangeNumber
               WRITE setCredentialsDbChangeNumber NOTIFY credentialsDbChangeNumberChanged)
    Q_PROPERTY(int dataDbChangeNumber READ getDataDbChangeNumber
               WRITE setDataDbChangeNumber NOTIFY dataDbChangeNumberChanged)

public:
    explicit DbBackupsTracker(const QString settingsFilePath, QObject *parent = nullptr);
    ~DbBackupsTracker();

    QString getTrackPath(const QString &cardId) const;
    QString getCardId() const;

    int getCredentialsDbChangeNumber() const;
    int getDataDbChangeNumber() const;

    /**
     * throws: DbBackupsTrackerNoBackupFileSet
     */
    bool isUpdateRequired() const;
    /**
     * throws: DbBackupsTrackerNoBackupFileSet
     */
    bool isBackupRequired() const;
    bool hasBackup() const;

    /**
     * Guess the current tracked backup file format,
     * currently suported: "none" and "SympleCrypt"
     * throws: DbBackupsTrackerNoBackupFileSet
     */
    QString getTrackedBackupFileFormat();

signals:
    void cardIdChanged(QString cardId);
    void credentialsDbChangeNumberChanged(int credentialsDbChangeNumber);
    void newTrack(const QString &cardId, const QString &path);

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
    QFileSystemWatcher watcher;
    QMap<QString, QString> tracks;
    QString cardId;
    QString settingsFilePath;
    int credentialsDbChangeNumber;
    int dataDbChangeNumber;

    void saveTracks();
    void loadTracks();
    static QString getSettingsFilePath();

    int tryGetCredentialsDbBackupChangeNumber() const;
    int tryGetDataDbBackupChangeNumber() const;
    void watchPath(const QString path);
    QString tryReadBackupFile() const;
    QString readFile(QString path) const;

    int extractCredentialsDbChangeNumber(const QString &content) const;
    int extractDataDbChangeNumber(const QString &content) const;

    bool isALegacyBackup(const QJsonDocument &d) const;
    bool isAnEncryptedBackup(const QJsonDocument &d) const;

    int extractCredentialsDbChangeNumberEncryptedBackup(const QJsonDocument &d) const;
    int extractCredentialsDbChangeNumberLegacyBackup(const QJsonDocument &d) const;
    int extractDataDbChangeNumberEncryptedBackup(const QJsonDocument &d) const;
    int extractDataDbChangeNumberLegacyBackup(const QJsonDocument &d) const;
    bool isDbBackupChangeNumberGreater(int backupCCN, int backupDCN) const;
    bool isDbBackupChangeNumberLower(int backupCCN, int backupDCN) const;
};

#endif // DBBACKUPSTRACKER_H
