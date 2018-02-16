#ifndef TESTDBEXPORTSREGISTRY_H
#define TESTDBEXPORTSREGISTRY_H

#include <QObject>

class TestDbExportsRegistry : public QObject
{
    Q_OBJECT

    const QString tmpSettingsPath = "/tmp/test_mooltipass_db_exports_registry";
    void verifyCardExport(QDate now);

    void verifyExportedDbRecord(const QString &cardId, QDate now, const int credentialsDbCN, const int dataDbCN);

    void setBackDateRecordAMonth();

    void setBackDateRecordAMonth(const QString &cardId);

private slots:
    void recommendExportWhenANewCardIsInserted();
    void registerCardDbExported();
    void dontRecommendExportWhenCardHasBeenExported();
    void recommendExportWhenAMonthHasPassedAndThereIsMajorCredentialsDbChangeNumber();
    void recommendExportWhenAMonthHasPassedAndThereIsMajorDataDbChangeNumber();
    void registerManyCardDbExported();

    void recommendExportOnCredentialsDbChangeNumberWrapover();
    void recommendExportOnDataDbChangeNumberWithWrapover();
    void cleanupTestCase();

};

#endif // TESTDBEXPORTSREGISTRY_H
