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
    const QByteArray cpz = "123456";
    QSignalSpy spy(&tracker, &DbBackupsTracker::cpzChanged);
    tracker.setCPZ(cpz);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(tracker.getCPZ(), cpz);
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
    QByteArray cpz = "123456";

    QSignalSpy spy(&tracker, &DbBackupsTracker::newTrack);
    tracker.setCPZ(cpz);
    tracker.track(p);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(tracker.getTrackPath(cpz), p);
}

void DbBackupsTrackerTests::trackFileNoCpz()
{
    tracker.setCPZ(QByteArray());
    QSignalSpy spy(&tracker, &DbBackupsTracker::newTrack);

    bool fired = false;
    try {
        tracker.track("/tmp/file");
    } catch (DbBackupsTrackerNoCpzSet& e) {
        fired = true;
    }

    Q_ASSERT(fired);
    QCOMPARE(spy.count(), 0);
}

QString DbBackupsTrackerTests::getTestDbFilePath()
{
    QString file(__FILE__);
    QFileInfo fileInfo(file);
    file = fileInfo.absoluteDir().absolutePath() + "/data/testdb";

    return file;
}

void DbBackupsTrackerTests::trackDbBackupWithMajorChangeNumber()
{
    QByteArray cpz = "123456";
    tracker.setCPZ(cpz);
    tracker.setCredentialsDbChangeNumber(0);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestDbFilePath();

    QSignalSpy spy(&tracker, &DbBackupsTracker::greaterDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCpzSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }
    spy.wait(100);
    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isUpdateRequired());
}

void DbBackupsTrackerTests::trackDbBackupWithMinorChangeNumber()
{
    QByteArray cpz = "123456";
    tracker.setCPZ(cpz);
    tracker.setCredentialsDbChangeNumber(30);
    tracker.setDataDbChangeNumber(0);

    QString file = getTestDbFilePath();

    QSignalSpy spy(&tracker, &DbBackupsTracker::lowerDbBackupChangeNumber);

    try {
        tracker.track(file);
    } catch (DbBackupsTrackerNoCpzSet& e) {
        QFAIL("DbBackupsTracker No Cpz Set");
    }
    spy.wait(100);
    QCOMPARE(spy.count(), 1);
    Q_ASSERT(tracker.isBackupRequired());
}

void DbBackupsTrackerTests::trackingPersisted()
{
    DbBackupsTracker* t = new DbBackupsTracker();
    t->setCPZ("00000");
    t->track("/tmp/file1");

    t->deleteLater();

    t = new DbBackupsTracker();
    t->setCPZ("00000");
    Q_ASSERT(t->hasBackup());

    t->deleteLater();
}
