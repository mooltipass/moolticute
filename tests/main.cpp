#include <QtTest>

#include "FilesCacheTests.h"
#include "UpdaterTests.h"
#include "DbBackupsTrackerTests.h"

// Note: This is equivalent to QTEST_APPLESS_MAIN for multiple test classes.
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

   int status = 0;

   DbBackupsTrackerTests dbBackupsTrackerTests;
   QTest::qExec(&dbBackupsTrackerTests);
//   TEST_CLASS(FilesCache, status);
//   TEST_CLASS(UpdaterTests, status);
//   TEST_CLASS(DbBackupsTrackerTests, status);

   return status;
}


