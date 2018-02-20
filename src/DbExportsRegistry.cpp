#include "DbExportsRegistry.h"

#include <QDate>
#include <QSettings>

#include "../src/DbBackupChangeNumbersComparator.h"

DbExportsRegistry::DbExportsRegistry(const QString &settingsPath, QObject *parent) :
    QObject(parent), settingsPath(settingsPath)
{
}

void DbExportsRegistry::setCurrentCardDbMetadata(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber)
{
    DbExportsRegistry::cardId = cardId;
    DbExportsRegistry::credentialsDbChangeNumber = credentialsDbChangeNumber;
    DbExportsRegistry::dataDbChangeNumber = dataDbChangeNumber;

    checkIfDbMustBeExported();
}

void DbExportsRegistry::checkIfDbMustBeExported()
{
    if (!cardId.isEmpty())
    {
        const QDate date = getLastExportDate(cardId);
        const int lastCredentialCN = getLastExportCredentialDbChangeNumber(cardId);
        const int lastDataCN = getLastExportDataDbChangeNumber(cardId);

        const bool areChangesSinceLastBackup =
                BackupChangeNumbersComparator::greaterThanWithWrapOver(credentialsDbChangeNumber, lastCredentialCN) ||
                BackupChangeNumbersComparator::greaterThanWithWrapOver(dataDbChangeNumber, lastDataCN);

        if  (isOlderThanAMonth(date) && areChangesSinceLastBackup)
            emit dbExportRecommended();
    }
}

QDate DbExportsRegistry::getLastExportDate(const QString &id)
{
    QSettings s(settingsPath, QSettings::IniFormat);
    s.sync();
    const QString key = "DbExportRegitry/"+id+"/date";
    return s.value(key, QDate()).toDate();
}

int DbExportsRegistry::getLastExportCredentialDbChangeNumber(const QString &id)
{
    QSettings s(settingsPath, QSettings::IniFormat);
    s.sync();
    const QString key = "DbExportRegitry/"+id+"/credentialsDbChangeNumber";
    return s.value(key, -1).toInt();
}

int DbExportsRegistry::getLastExportDataDbChangeNumber(const QString &id)
{
    QSettings s(settingsPath, QSettings::IniFormat);
    s.sync();
    const QString key = "DbExportRegitry/"+id+"/dataDbChangeNumber";
    return s.value(key, -1).toInt();
}

bool DbExportsRegistry::isOlderThanAMonth(const QDate &lastExport)
{
    QDate limit = QDate::currentDate().addMonths(-1);
    return lastExport <= limit;
}

void DbExportsRegistry::registerDbExport()
{
    QSettings s(settingsPath, QSettings::IniFormat);
    s.beginGroup("DbExportRegitry/"+cardId);

    s.setValue("date", QDate::currentDate());
    s.setValue("credentialsDbChangeNumber", credentialsDbChangeNumber);
    s.setValue("dataDbChangeNumber", dataDbChangeNumber);

    s.endGroup();
    s.sync();
}
