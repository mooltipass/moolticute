#include "DbBackupsTrackerTests.h"

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
    QSignalSpy spy(&tracker, &DbBackupsTracker::dbChangeNumberChanged);
    tracker.setDbChangeNumber(n);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(tracker.getDbChangeNumber(), n);
}

void DbBackupsTrackerTests::TrackFile()
{
    QString p = "/tmp/file";
    QString cpz = "123456";

    QSignalSpy spy(&tracker, &DbBackupsTracker::newTrack);
    tracker.setCPZ(cpz.toLocal8Bit());
    tracker.track(p);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(tracker.getTrackPath(cpz), p);
}

void DbBackupsTrackerTests::TrackFileNoCpz()
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

void DbBackupsTrackerTests::TrackFileChanged()
{
    QTemporaryFile file;
    QString p = "/tmp/file";
    QString cpz = "123456";

    QSignalSpy spy(&tracker, &DbBackupsTracker::newTrack);
    tracker.setCPZ(cpz.toLocal8Bit());
    tracker.track(p);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(tracker.getTrackPath(cpz), p);
}
