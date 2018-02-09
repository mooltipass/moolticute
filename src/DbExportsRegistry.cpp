#include "DbExportsRegistry.h"

#include <QDate>
#include <QSettings>

DbExportsRegistry::DbExportsRegistry(const QString &settingsPath, QObject *parent) :
    QObject(parent), settingsPath(settingsPath)
{
}

QDate DbExportsRegistry::getLastExportDate(const QString &id)
{
    QDate lastExport;
    QSettings s(settingsPath, QSettings::IniFormat);
    s.sync();
    s.beginGroup("DbExportRegitry");
    if (s.contains(id))
        lastExport = s.value(id).toDate();
    s.endGroup();

    return lastExport;
}

bool DbExportsRegistry::isOlderThanAMonth(const QDate &lastExport)
{
    QDate limit = QDate::currentDate().addMonths(-1);
    return lastExport <= limit;
}

void DbExportsRegistry::setCurrentCardId(const QString &id)
{
    DbExportsRegistry::id = id;
    if (!DbExportsRegistry::id.isEmpty())
    {
        QDate date = getLastExportDate(id);

        if  (isOlderThanAMonth(date))
            emit dbExportRecommended();
    }
}

void DbExportsRegistry::registerDbExport()
{
    QSettings s(settingsPath, QSettings::IniFormat);
    s.sync();
    s.beginGroup("DbExportRegitry");
    s.setValue(id, QDate::currentDate());
    s.endGroup();
    s.sync();
}
