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
    void trackEncryptedDbBackMajorCredentialsDbChangeNumber();
    void trackEncryptedDbBackupWithMinorCredentialsDbChangeNumber();
    void trackEncryptedDbBackupMajorDataDbChangeNumber();
    void trackEncryptedDbBackupWithMinorDataDbChangeNumber();

    void trackLegacyDbBackupWithMajorChangeNumber();
    void trackLegacyDbBackupWithMinorChangeNumber();
    void trackLegacyDbBackupMajorDataDbChangeNumber();
    void trackLegacyDbBackupWithMinorDataDbChangeNumber();
    void trackingPersisted();

    void getFileFormatLegacy();
    void getFileFormatSympleCrypt();

private:
    DbBackupsTracker tracker;
    QString getTestsDataDirPath();
};

#endif // DBBACKUPSTRACKERTESTS_H
