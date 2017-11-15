#include "DbBackupsTracker.h"

#include <QDebug>
#include <QException>

DbBackupsTracker::DbBackupsTracker(QObject* parent)
    : QObject(parent)
{
}

DbBackupsTracker::~DbBackupsTracker()
{
}

QString
DbBackupsTracker::getTrackPath(const QString& cpz)
{
    return tracks.value(cpz);
}

QByteArray
DbBackupsTracker::getCPZ() const
{
    return cpz;
}

int DbBackupsTracker::getDbChangeNumber() const
{
    return dbChangeNumber;
}

void DbBackupsTracker::watchPath(const QString path)
{
    watcher.removePaths(watcher.files());
    watcher.addPath(path);
}

void DbBackupsTracker::track(const QString path)
{
    if (cpz.isEmpty()) {
        DbBackupsTrackerNoCpzSet ex;
        ex.raise();
    } else {
        watchPath(path);

        tracks.insert(cpz, path);
        emit newTrack(cpz, path);
    }
}

void DbBackupsTracker::setCPZ(QByteArray cpz)
{
    if (this->cpz == cpz)
        return;

    this->cpz = cpz;
    emit cpzChanged(cpz);
}

void DbBackupsTracker::setDbChangeNumber(int dbChangeNumber)
{
    if (this->dbChangeNumber == dbChangeNumber)
        return;

    this->dbChangeNumber = dbChangeNumber;
    emit dbChangeNumberChanged(this->dbChangeNumber);
}

void DbBackupsTrackerNoCpzSet::raise() const
{
    throw * this;
}

DbBackupsTrackerNoCpzSet*
DbBackupsTrackerNoCpzSet::clone() const
{
    return new DbBackupsTrackerNoCpzSet(*this);
}
