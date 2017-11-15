#ifndef DBBACKUPSTRACKER_H
#define DBBACKUPSTRACKER_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QException>
#include <QFileSystemWatcher>

class DbBackupsTrackerNoCpzSet : public QException {
public:
    void raise() const;
    DbBackupsTrackerNoCpzSet* clone() const;
};

class DbBackupsTracker : public QObject {
    Q_OBJECT
    Q_PROPERTY(QByteArray cpz READ getCPZ WRITE setCPZ NOTIFY cpzChanged)
    Q_PROPERTY(int dbChangeNumber READ getDbChangeNumber WRITE setDbChangeNumber
            NOTIFY dbChangeNumberChanged)
public:
    explicit DbBackupsTracker(QObject* parent = nullptr);
    ~DbBackupsTracker();

    QString getTrackPath(const QString& cpz);
    QByteArray getCPZ() const;

    int getDbChangeNumber() const;

signals:
    void cpzChanged(QByteArray cpz);
    void dbChangeNumberChanged(int dbChangeNumber);
    void newTrack(const QString& cpz, const QString& path);

public slots:
    void track(const QString path);
    void setCPZ(QByteArray cpz);
    void setDbChangeNumber(int dbChangeNumber);

private:
    QFileSystemWatcher watcher;
    QMap<QString, QString> tracks;
    QByteArray cpz;
    int dbChangeNumber;
    void watchPath(const QString path);
};

#endif // DBBACKUPSTRACKER_H
