#ifndef TESTCREDENTIALSMODEL_H
#define TESTCREDENTIALSMODEL_H

#include <QObject>
#include <QByteArray>
#include <QModelIndex>
#include <QAbstractItemModel>

class TestCredentialModel : public QObject
{
    Q_OBJECT
public:
    explicit TestCredentialModel(QObject *parent = nullptr);

private Q_SLOTS:
    void noChanges();
    void oneCredentialRemoved();

private:
    QByteArray emptyLoginTestJson = R"([{"childs":[{"address":[-71,12],"date_created":"2018-01-28","date_last_used":"2018-01-28","description":"","favorite":-1,"login":"","password_enc":[205,246,128,228,207,183,151,121,154,164,68,227,85,34,65,36,54,46,44,211,209,114,140,34,194,235,32,43,138,2,2,97]},{"address":[-64,12],"date_created":"2018-01-28","date_last_used":"2018-01-28","description":"","favorite":-1,"login":"loginA","password_enc":[9,17,87,15,205,141,137,165,134,141,88,96,145,42,191,140,151,19,34,223,86,147,125,167,136,239,47,87,31,102,74,2]},{"address":[16,14],"date_created":"","date_last_used":"2018-01-28","description":"","favorite":-1,"login":"loginB","password_enc":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}],"service":"service.io"}])";
    QModelIndex findLoginIndex(QString loginName, QString serviceName, QAbstractItemModel* model);
};

#endif // TESTCREDENTIALSMODEL_H
