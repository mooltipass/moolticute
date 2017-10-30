#include "FilesCacheTests.h"

FilesCacheTests::FilesCacheTests()
{
}

void FilesCacheTests::testSaveAndLoadFileNames()
{
    QList<QVariantMap> testFiles;
    for (int i = 0; i< 3; i++)
    {
        QVariantMap item;
        item.insert("revision", 0);
        item.insert("name", QString("file %1").arg(i));
        item.insert("size", 1024*i);
        testFiles << item;
    }

    FilesCache cache;
    cache.setDbChangeNumber(0);
    cache.setCardCPZ("cbe9cad108aad501");

    bool result = cache.save(testFiles);
    QVERIFY(result);

    QList<QVariantMap> fileInCache = cache.load();

    QVERIFY(fileInCache.size() == 3);
    for (int i = 0; i < testFiles.size(); i ++)
        QVERIFY(testFiles.at(i) == fileInCache.at(i));

    QVERIFY(cache.erase());
}
