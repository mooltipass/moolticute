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
    void TrackFile();
    void TrackFileNoCpz();
    void TrackFileChanged();

private:
    DbBackupsTracker tracker;
};

#endif // DBBACKUPSTRACKERTESTS_H
