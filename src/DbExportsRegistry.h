#ifndef DBEXPORTSREGISTRY_H
#define DBEXPORTSREGISTRY_H

#include <QObject>

class DbExportsRegistry : public QObject
{
    Q_OBJECT
    QString id;
    QString settingsPath;
public:
    explicit DbExportsRegistry(const QString &settingsPath, QObject *parent = nullptr);

signals:
    void dbExportRecommended();

public slots:
    void setCurrentCardId(const QString &id);
    void registerDbExport();

private:
    QDate getLastExportDate(const QString &id);

    bool isOlderThanAMonth(const QDate &lastExport);

};

#endif // DBEXPORTSREGISTRY_H
