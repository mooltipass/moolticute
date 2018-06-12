#include <QtTest>

#include "FilesCacheTests.h"
#include "UpdaterTests.h"
#include "DbBackupsTrackerTests.h"
#include "TestTreeItem.h"
#include "TestCredentialModel.h"
#include "TestCredentialModelFilter.h"
#include "TestDbExportsRegistry.h"
#include "TestParseDomain.h"

// Note: This is equivalent to QTEST_APPLESS_MAIN for multiple test classes.
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

    int status = 0;
    const auto runTest = [&status](QObject *test) {
        status += QTest::qExec(test);
    };

    {
        TestTreeItem testTreeItem;
        runTest(&testTreeItem);
    }

    {
        TestCredentialModel testCredentialsModel;
        runTest(&testCredentialsModel);
    }

    {
        TestCredentialModelFilter testCredentialsModelFilter;
        runTest(&testCredentialsModelFilter);
    }

    {
        DbBackupsTrackerTests dbBackupsTrackerTests;
        runTest(&dbBackupsTrackerTests);
    }

    {
        TestDbExportsRegistry testDbExportsRegistry;
        runTest(&testDbExportsRegistry);
    }

    {
        TestParseDomain testParseDomain;
        runTest(&testParseDomain);
    }

    return status;
}


