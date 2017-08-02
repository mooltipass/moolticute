#include <QString>
#include <QtTest>

#include <QStringList>
#include "../src/FilesCache.h"

class TestsFilesCache : public QObject
{
    Q_OBJECT

public:
    TestsFilesCache();

private Q_SLOTS:
    void testSaveAndLoadFileNames();
};

TestsFilesCache::TestsFilesCache()
{
}

void TestsFilesCache::testSaveAndLoadFileNames()
{
    QList<QPair<int, QString>> testFileNames;
    testFileNames << QPair<int, QString>(3, "one");
    testFileNames << QPair<int, QString>(1, "two");
    testFileNames << QPair<int, QString>(3, "three");

    FilesCache cache("cbe9cad108aad501");

    bool result = cache.save(testFileNames);
    QVERIFY(result);

    QList<QPair<int, QString>> fileNamesInCache = cache.load();

    QVERIFY(fileNamesInCache.size() == 3);
    for (int i = 0; i < testFileNames.size(); i ++)
        QVERIFY(testFileNames.at(i) == fileNamesInCache.at(i));

    QVERIFY(cache.erase());
}


QTEST_APPLESS_MAIN(TestsFilesCache)

#include "tst_filescache.moc"
