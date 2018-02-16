#ifndef DBEXPORTSREGISTRY_H
#define DBEXPORTSREGISTRY_H

#include <QObject>

class DbExportsRegistry : public QObject
{
    Q_OBJECT
    QString cardId;
    int credentialsDbChangeNumber;
    int dataDbChangeNumber;

    QString settingsPath;
public:
    explicit DbExportsRegistry(const QString &settingsPath, QObject *parent = nullptr);

signals:
    void dbExportRecommended();

public slots:
    void setCurrentCardDbMetadata(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber);
    void registerDbExport();

private:
    void checkIfDbMustBeExported();
    QDate getLastExportDate(const QString &cardId);
    bool isOlderThanAMonth(const QDate &lastExport);
    int getLastExportCredentialDbChangeNumber(const QString &id);
    int getLastExportDataDbChangeNumber(const QString &id);
};

#endif // DBEXPORTSREGISTRY_H
