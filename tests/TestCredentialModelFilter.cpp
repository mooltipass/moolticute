#include "TestCredentialModelFilter.h"

#include <QJsonObject>

#include "../src/TreeItem.h"
#include "../src/LoginItem.h"
#include "../src/CredentialModel.h"
#include "../src/CredentialModelFilter.h"
#include "TestCredentialModel.h"

TestCredentialModelFilter::TestCredentialModelFilter(QObject *parent) : QObject(parent)
{

}

void TestCredentialModelFilter::findAndRemoveCredentialFromSourceModel()
{
    CredentialModel *sourceModel = TestCredentialModel::createCredentialModelWithThreeLogins();
    CredentialModelFilter *filter = new CredentialModelFilter();
    filter->setSourceModel(sourceModel);

    QModelIndex idx = TestCredentialModel::findLoginIndex("", "service.io", filter);
    Q_ASSERT(idx.isValid());

    QModelIndex srcIdx = filter->mapToSource(idx);

    LoginItem *loginItem = sourceModel->getLoginItemByIndex(srcIdx);
    Q_ASSERT(loginItem != nullptr);

    QJsonObject o = loginItem->toJson();
    Q_ASSERT(o.value("login").toString().compare("") == 0);
    Q_ASSERT(o.value("service").toString().compare("service.io") == 0);

    sourceModel->removeCredential(srcIdx);

    QJsonArray result = sourceModel->getJsonChanges();
    Q_ASSERT(result.count() == 2);

    QJsonObject l1 = result.at(0).toObject();
    QJsonObject l2 = result.at(1).toObject();

    Q_ASSERT(l1.value("login").toString().compare("loginA") == 0);
    Q_ASSERT(l2.value("login").toString().compare("loginB") == 0);

    filter->deleteLater();
    sourceModel->deleteLater();
}
