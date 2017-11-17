#ifndef DBBACKUPSTRACKER_H
#define DBBACKUPSTRACKER_H

#include <QException>
#include <QFileSystemWatcher>
#include <QMap>
#include <QObject>
#include <QString>
#include <QCryptographicHash>

class DbBackupsTrackerNoCpzSet : public QException {
public:
    void raise() const;
    DbBackupsTrackerNoCpzSet* clone() const;
};

class DbBackupsTracker : public QObject {
    Q_OBJECT
    Q_PROPERTY(QByteArray cpz READ getCPZ WRITE setCPZ NOTIFY cpzChanged)
    Q_PROPERTY(int credentialsDbChangeNumber READ getCredentialsDbChangeNumber WRITE setCredentialsDbChangeNumber
            NOTIFY credentialsDbChangeNumberChanged)
    Q_PROPERTY(int dataDbChangeNumber READ getDataDbChangeNumber WRITE setDataDbChangeNumber
            NOTIFY dataDbChangeNumberChanged)
public:
    explicit DbBackupsTracker(QObject* parent = nullptr);
    ~DbBackupsTracker();

    QString getTrackPath(const QByteArray& cpz) const;
    QByteArray getCPZ() const;

    int getCredentialsDbChangeNumber() const;
    int getCredentialsDbBackupChangeNumber() const;
    int getDataDbBackupChangeNumber() const;
    int getDataDbChangeNumber() const;

    bool isUpdateRequired() const;
    bool isBackupRequired() const;
    bool hasBackup() const;

signals:
    void cpzChanged(QByteArray cpz);
    void credentialsDbChangeNumberChanged(int credentialsDbChangeNumber);
    void newTrack(const QString& cpz, const QString& path);

    void greaterDbBackupChangeNumber();
    void lowerDbBackupChangeNumber();

    void dataDbChangeNumberChanged(int dataDbChangeNumber);

public slots:
    void track(const QString path);
    void setCPZ(QByteArray cpz);
    void setCredentialsDbChangeNumber(int credentialsDbChangeNumber);
    void setDataDbChangeNumber(int dataDbChangeNumber);
    void checkDbBackupSynchronization();

private:
    void saveTracks();
    void loadTracks();
    QFileSystemWatcher watcher;
    QMap<QString, QString> tracks;
    QByteArray cpz, cpzHash;
    int credentialsDbChangeNumber;
    int dataDbChangeNumber;
    void watchPath(const QString path);
    QString readFile(QString path) const;
    int extractCredentialsDbChangeNumber(const QString& content) const;
    int extractDataDbChangeNumber(const QString& content) const;
    QByteArray getCpzHash(const QByteArray &cpz) const;
};

#endif // DBBACKUPSTRACKER_H
