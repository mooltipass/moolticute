#ifndef DBBACKUPSTRACKERTESTS_H
#define DBBACKUPSTRACKERTESTS_H

#include <QObject>

#include "../src/DbBackupsTracker.h"

class DbBackupsTrackerTests : public QObject {
    Q_OBJECT
public:
    explicit DbBackupsTrackerTests(QObject* parent = nullptr);

signals:

private slots:
    void setCPZ();
    void setDbChangeNumber();
    void trackFile();
    void trackFileNoCpz();
    void trackDbBackupWithMajorChangeNumber();
    void trackDbBackupWithMinorChangeNumber();
    void trackingPersisted();

private:
    DbBackupsTracker tracker;
    QString getTestDbFilePath();
    QString getTestLegacyDbFilePath();
};

#endif // DBBACKUPSTRACKERTESTS_H
