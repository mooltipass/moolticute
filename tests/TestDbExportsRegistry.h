#ifndef TESTDBEXPORTSREGISTRY_H
#define TESTDBEXPORTSREGISTRY_H

#include <QObject>

class TestDbExportsRegistry : public QObject
{
    Q_OBJECT

    const QString tmpSettingsPath = "/tmp/test_mooltipass_db_exports_registry";
    void verifyCardExport(QDate now);

    void verifyExportedDbRecord(const QString &cardId, QDate now, const int credentialsDbCN, const int dataDbCN);

private slots:
    void recommendExportWhenANewCardIsInserted();
    void registerCardDbExported();
    void dontRecommendExportWhenCardHasBeenExported();
    void recommendExportWhenAMonthHasPassedAndThereIsMajorCredentialsDbChangeNumber();
    void recommendExportWhenAMonthHasPassedAndThereIsMajorDataDbChangeNumber();
    void registerManyCardDbExported();

    void cleanupTestCase();

};

#endif // TESTDBEXPORTSREGISTRY_H
