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
    QStringList testFileNames = {"one", "two", "three"};
    FilesCache cache("testCardCPZ");

    bool result = cache.save(testFileNames);
    QVERIFY(result);

    QStringList fileNamesInCache = cache.load();

    QVERIFY(fileNamesInCache.size() == 3);
    for (int i = 0; i < testFileNames.size(); i ++)
        QVERIFY(testFileNames.at(i) == fileNamesInCache.at(i));

    QVERIFY(cache.erase());
}


QTEST_APPLESS_MAIN(TestsFilesCache)

#include "tst_filescache.moc"
