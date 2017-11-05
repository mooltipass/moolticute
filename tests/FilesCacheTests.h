#include <QString>
#include <QtTest>

#include <QStringList>
#include "../src/FilesCache.h"

class FilesCacheTests : public QObject
{
    Q_OBJECT

public:
    FilesCacheTests();

private Q_SLOTS:
    void testSaveAndLoadFileNames();
};




