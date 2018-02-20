#include "DbBackupsTrackerTests.h"

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

DbBackupsTrackerTests::DbBackupsTrackerTests(QObject* parent)
    : QObject(parent), tracker("/tmp/test_db_backups_tracker.info", this)
{
}

void DbBackupsTrackerTests::setCPZ()
{
    const QString cpz = "123456";
    QSignalSpy spy(&tracker, &DbBackupsTracker::cardIdChanged);
    tracker.setCardId(cpz);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(tracker.getCardId(), cpz);
}

void DbBackupsTrackerTests::setDbChangeNumber()
{
    const int n = 3;
    QSignalSpy spy(&tracker, &DbBackupsTracker::credentialsDbChangeNumberChanged);
    tracker.setCredentialsDbChangeNumber(n);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(tracker.getCredentialsDbChangeNumber(), n);
}

void DbBackupsTrackerTests::trackFile()
{
    QString p = "/tmp/file";
    const QString cpz = "123456";

    QSignalSpy spy(&tracker, &DbBackupsTracker::newTrack);
    tracker.setCardId(cpz);
    tracker.track(p);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(tracker.getTrackPath(cpz), p);
}

void DbBackupsTrackerTests::trackFileNoCpz()
{
    tracker.setCardId("");
    QSignalSpy spy(&tracker, &DbBackupsTracker::newTrack);

    bool fired = false;
    try {
        tracker.track("/tmp/file");
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        fired = true;
    }

    Q_ASSERT(fired);
    QCOMPARE(spy.count(), 0);
}

QString DbBackupsTrackerTests::getTestsDataDirPath()
{
    QString dir(__FILE__);
    QFileInfo fileInfo(dir);
    dir = fileInfo.absoluteDir().absolutePath() + "/data/";

    return dir;
}

void DbBackupsTrackerTests::trackEncryptedDbBackMajorCredentialsDbChangeNumber()
{
    const QString cardId = "123456";
    tracker.setCardId(cardId);
    tracker.setCredentialsDbChangeNumber(0);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestsDataDirPath() + "tests_backup";

    QSignalSpy spy(&tracker, &DbBackupsTracker::greaterDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }
    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isUpdateRequired());
}

void DbBackupsTrackerTests::trackEncryptedDbBackupWithMinorCredentialsDbChangeNumber()
{
    const QString cardId = "123456";
    tracker.setCardId(cardId);
    tracker.setCredentialsDbChangeNumber(30);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestsDataDirPath() + "tests_backup";

    QSignalSpy spy(&tracker, &DbBackupsTracker::lowerDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }

    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isBackupRequired());
}

void DbBackupsTrackerTests::trackEncryptedDbBackupMajorDataDbChangeNumber()
{
    const QString cardId = "123456";
    tracker.setCardId(cardId);
    tracker.setCredentialsDbChangeNumber(0);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestsDataDirPath() + "tests_backup1";

    QSignalSpy spy(&tracker, &DbBackupsTracker::greaterDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }

    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isUpdateRequired());
}

void DbBackupsTrackerTests::trackEncryptedDbBackupWithMinorDataDbChangeNumber()
{
    const QString cardId = "123456";
    tracker.setCardId(cardId);
    tracker.setCredentialsDbChangeNumber(0);
    tracker.setDataDbChangeNumber(30);

    QString file = getTestsDataDirPath() + "tests_backup1";

    QSignalSpy spy(&tracker, &DbBackupsTracker::lowerDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }

    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isBackupRequired());
}

void DbBackupsTrackerTests::trackLegacyDbBackupWithMajorChangeNumber()
{
    const QString cardId = "123456";
    tracker.setCardId(cardId);
    tracker.setCredentialsDbChangeNumber(0);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestsDataDirPath() + "tests_legacy_backups";

    QSignalSpy spy(&tracker, &DbBackupsTracker::greaterDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }
    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isUpdateRequired());
}

void DbBackupsTrackerTests::trackLegacyDbBackupWithMinorChangeNumber()
{
    const QString cardId = "123456";
    tracker.setCardId(cardId);
    tracker.setCredentialsDbChangeNumber(30);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestsDataDirPath() + "tests_legacy_backups";

    QSignalSpy spy(&tracker, &DbBackupsTracker::lowerDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }

    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isBackupRequired());
}


void DbBackupsTrackerTests::trackLegacyDbBackupMajorDataDbChangeNumber()
{
    const QString cardId = "123456";
    tracker.setCardId(cardId);
    tracker.setCredentialsDbChangeNumber(0);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestsDataDirPath() + "tests_legacy_backups1";

    QSignalSpy spy(&tracker, &DbBackupsTracker::greaterDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }

    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isUpdateRequired());
}

void DbBackupsTrackerTests::trackLegacyDbBackupWithMinorDataDbChangeNumber()
{
    const QString cardId = "123456";
    tracker.setCardId(cardId);
    tracker.setCredentialsDbChangeNumber(30);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestsDataDirPath() + "tests_legacy_backups1";

    QSignalSpy spy(&tracker, &DbBackupsTracker::lowerDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }

    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isBackupRequired());
}

void DbBackupsTrackerTests::trackingPersisted()
{
    DbBackupsTracker* t = new DbBackupsTracker("/tmp/test_db_backups_tracker.info");
    t->setCardId("00000");
    QTemporaryFile file;
    file.open();

    t->track(file.fileName());
    t->deleteLater();

    t = new DbBackupsTracker("/tmp/test_db_backups_tracker.info");
    t->setCardId("00000");
    Q_ASSERT(t->hasBackup());

    t->deleteLater();

    file.close();
}

void DbBackupsTrackerTests::credentialChangenumberWrapOver()
{
    // the monitored file change number is higher than the one the device
    // *and* the difference between both numbers is less than 0x60
    const QString cardId = "123456";
    tracker.setCardId(cardId);
    tracker.setCredentialsDbChangeNumber(22);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestsDataDirPath() + "tests_backup2";

    QSignalSpy spy(&tracker, &DbBackupsTracker::greaterDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }
    QCOMPARE(spy.count(), 0);
    Q_ASSERT(!tracker.isUpdateRequired());

    // monitored file change number is 0xFF and
    // the change number on the device is 0x00
    tracker.setCredentialsDbChangeNumber(0x00);
    tracker.setDataDbChangeNumber(0x00);

    file = getTestsDataDirPath() + "tests_backup3";

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }
    QCOMPARE(spy.count(), 0);
    Q_ASSERT(!tracker.isUpdateRequired());

    // monitored file change number is 0x00 and
    // the change number on the device is 0xFF
    tracker.setCredentialsDbChangeNumber(0xFF);
    tracker.setDataDbChangeNumber(0xFF);

    file = getTestsDataDirPath() + "tests_backup4";

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCardIdSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }
    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isUpdateRequired());

}

void DbBackupsTrackerTests::getFileFormatLegacy()
{
    DbBackupsTracker* t = new DbBackupsTracker("/tmp/test_db_backups_tracker.info");
    t->setCardId("00000");
    QString file = getTestsDataDirPath() + "tests_legacy_backups";
    t->track(file);

    QCOMPARE(QString("none"), t->getTrackedBackupFileFormat());
    t->deleteLater();
}

void DbBackupsTrackerTests::getFileFormatSympleCrypt()
{
    DbBackupsTracker* t = new DbBackupsTracker("/tmp/test_db_backups_tracker.info");
    t->setCardId("00000");
    QString file = getTestsDataDirPath() + "tests_backup";
    t->track(file);

    QCOMPARE(QString("SympleCrypt"), t->getTrackedBackupFileFormat());
    t->deleteLater();
}
