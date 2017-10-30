#include <QtTest>

#include "FilesCacheTests.h"
#include "UpdaterTests.h"

#define TEST_CLASS(TestClass, status) \
{ \
   TestClass tc; \
   status |= QTest::qExec(&tc, argc, argv);\
}


// Note: This is equivalent to QTEST_APPLESS_MAIN for multiple test classes.
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

   int status = 0;
   TEST_CLASS(FilesCache, status);
   TEST_CLASS(UpdaterTests, status);

   return status;
}


