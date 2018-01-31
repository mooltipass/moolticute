#include "TestCredentialModel.h"

#include <QJsonDocument>
#include <QJsonArray>

#include "../src/CredentialModel.h"

TestCredentialModel::TestCredentialModel(QObject *parent) : QObject(parent)
{

}

CredentialModel *TestCredentialModel::createCredentialModelWithThreeLogins()
{
    CredentialModel *model = new CredentialModel();
    QJsonArray array = QJsonDocument::fromJson(emptyLoginTestJson).array();
    model->load(array);

    return model;
}

void TestCredentialModel::noChanges()
{
    CredentialModel *model = createCredentialModelWithThreeLogins();

    Q_ASSERT(model->rowCount());
    Q_ASSERT(!model->getJsonChanges().isEmpty());
}

void TestCredentialModel::oneCredentialRemoved()
{
    CredentialModel *model = createCredentialModelWithThreeLogins();

    QString loginName = "";
    QString serviceName = "service.io";

    QModelIndex requiredIdx = findLoginIndex(loginName, serviceName, model);
    Q_ASSERT(requiredIdx.isValid());

    model->removeCredential(requiredIdx);

    QJsonArray result = model->getJsonChanges();
    Q_ASSERT(result.count() == 2);

    QJsonObject l1 = result.at(0).toObject();
    QJsonObject l2 = result.at(1).toObject();

    Q_ASSERT(l1.value("login").toString().compare("loginA") == 0);
    Q_ASSERT(l2.value("login").toString().compare("loginB") == 0);
}

QModelIndex TestCredentialModel::findLoginIndex(QString loginName, QString serviceName, QAbstractItemModel *model)
{
    bool found = false;
    QModelIndex requiredIdx;

    for (int r = 0; r < model->rowCount() && !found; r++) {
        const QModelIndex sIdx = model->index(r, 0);
        const QString sItemName = sIdx.data(Qt::DisplayRole).toString();
        if (serviceName.compare(sItemName) == 0) {
            for (int sr = 0; sr < model->rowCount(sIdx) && !found; sr ++) {
                const QModelIndex lIdx = model->index(r, 0, sIdx);
                const QString lItemName = lIdx.data(Qt::DisplayRole).toString();
                if (loginName.compare(lItemName) == 0) {
                    requiredIdx = lIdx;
                    found = true;
                }
            }
        }
    }

    return requiredIdx;
}
