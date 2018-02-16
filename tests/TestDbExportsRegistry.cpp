#include "TestDbExportsRegistry.h"

#include <QTest>
#include <QSignalSpy>
#include <QSettings>
#include <QSettings>
#include <QFile>

#include "../src/DbExportsRegistry.h"



void TestDbExportsRegistry::recommendExportWhenANewCardIsInserted()
{
    DbExportsRegistry registry(tmpSettingsPath);
    QSignalSpy spy(&registry, &DbExportsRegistry::dbExportRecommended);

    registry.setCurrentCardDbMetadata("222", 0, 0);

    QCOMPARE(spy.count(), 1);
}

void TestDbExportsRegistry::registerCardDbExported()
{
    DbExportsRegistry registry(tmpSettingsPath);
    QDate now = QDate::currentDate();
    registry.setCurrentCardDbMetadata("222", 1, 2);
    registry.registerDbExport();

    verifyExportedDbRecord("222", now, 1, 2);
}

void TestDbExportsRegistry::dontRecommendExportWhenCardHasBeenExported()
{
    DbExportsRegistry registry(tmpSettingsPath);
    QSignalSpy spy(&registry, &DbExportsRegistry::dbExportRecommended);

    registry.setCurrentCardDbMetadata("222", 0, 0);

    QCOMPARE(spy.count(), 0);
}

void TestDbExportsRegistry::recommendExportWhenAMonthHasPassedAndThereIsMajorCredentialsDbChangeNumber()
{
    QSettings s(tmpSettingsPath, QSettings::IniFormat);
    s.setValue("DbExportRegitry/222/date", QDate::currentDate().addMonths(-1));

    DbExportsRegistry registry(tmpSettingsPath);
    QSignalSpy spy(&registry, &DbExportsRegistry::dbExportRecommended);

    registry.setCurrentCardDbMetadata("222", 2, 1);

    QCOMPARE(spy.count(), 1);
}

void TestDbExportsRegistry::recommendExportWhenAMonthHasPassedAndThereIsMajorDataDbChangeNumber()
{
    QSettings s(tmpSettingsPath, QSettings::IniFormat);
    s.setValue("DbExportRegitry/222/date", QDate::currentDate().addMonths(-1));

    DbExportsRegistry registry(tmpSettingsPath);
    QSignalSpy spy(&registry, &DbExportsRegistry::dbExportRecommended);

    registry.setCurrentCardDbMetadata("222", 1, 3);

    QCOMPARE(spy.count(), 1);
}

void TestDbExportsRegistry::registerManyCardDbExported()
{
    DbExportsRegistry registry(tmpSettingsPath);
    QDate now = QDate::currentDate();

    registry.setCurrentCardDbMetadata("222", 1, 2);
    registry.registerDbExport();
    verifyExportedDbRecord("222", now, 1, 2);

    registry.setCurrentCardDbMetadata("333", 5, 6);
    registry.registerDbExport();
    verifyExportedDbRecord("333", now, 5, 6);
}

void TestDbExportsRegistry::setBackDateRecordAMonth(const QString &cardId)
{
    QSettings s(tmpSettingsPath, QSettings::IniFormat);
    s.setValue("DbExportRegitry/"+cardId+"/date", QDate::currentDate().addMonths(-1));
}

void TestDbExportsRegistry::recommendExportOnCredentialsDbChangeNumberWrapover()
{
    DbExportsRegistry registry(tmpSettingsPath);
    const QString cardId = "AAA";
    registry.setCurrentCardDbMetadata(cardId, 0xFF, 1);
    registry.registerDbExport();

    setBackDateRecordAMonth(cardId);

    registry.setCurrentCardDbMetadata("", -1, -1);

    QSignalSpy spy(&registry, &DbExportsRegistry::dbExportRecommended);

    registry.setCurrentCardDbMetadata(cardId, 1, 1);

    QCOMPARE(spy.count(), 1);
}

void TestDbExportsRegistry::recommendExportOnDataDbChangeNumberWithWrapover()
{
    DbExportsRegistry registry(tmpSettingsPath);
    const QString cardId = "BBB";
    registry.setCurrentCardDbMetadata(cardId, 1, 0xFF);
    registry.registerDbExport();

    setBackDateRecordAMonth(cardId);

    registry.setCurrentCardDbMetadata("", -1, -1);

    QSignalSpy spy(&registry, &DbExportsRegistry::dbExportRecommended);

    registry.setCurrentCardDbMetadata(cardId, 1, 1);

    QCOMPARE(spy.count(), 1);
}

void TestDbExportsRegistry::verifyExportedDbRecord(const QString &cardId, QDate now, const int credentialsDbCN, const int dataDbCN)
{
    QSettings s(tmpSettingsPath, QSettings::IniFormat);
    QCOMPARE(s.value("DbExportRegitry/"+cardId+"/date").toDate(), now);
    QCOMPARE(s.value("DbExportRegitry/"+cardId+"/credentialsDbChangeNumber").toInt(), credentialsDbCN);
    QCOMPARE(s.value("DbExportRegitry/"+cardId+"/dataDbChangeNumber").toInt(), dataDbCN);
}


void TestDbExportsRegistry::cleanupTestCase()
{
    QFile::remove(tmpSettingsPath);
}
