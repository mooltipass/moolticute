#include "DbBackupsTrackerTests.h"

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

DbBackupsTrackerTests::DbBackupsTrackerTests(QObject* parent)
    : QObject(parent)
    , tracker(this)
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
    DbBackupsTracker* t = new DbBackupsTracker();
    t->setCardId("00000");
    t->track("/tmp/file1");

    t->deleteLater();

    t = new DbBackupsTracker();
    t->setCardId("00000");
    Q_ASSERT(t->hasBackup());

    t->deleteLater();
}
