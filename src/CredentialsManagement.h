#ifndef CREDENTIALSMANAGEMENT_H
#define CREDENTIALSMANAGEMENT_H

#include <QtWidgets>
#include "WSClient.h"
#include "CredentialsModel.h"

namespace Ui {
class CredentialsManagement;
}

class CredentialsManagement : public QWidget
{
    Q_OBJECT

public:
    explicit CredentialsManagement(QWidget *parent = 0);
    ~CredentialsManagement();

    void setWsClient(WSClient *c);

signals:
    void wantEnterMemMode();

public slots:
    bool confirmDiscardUneditedCredentialChanges(QModelIndex idx = {});

private slots:
    void enableCredentialsManagement(bool);
    void updateQuickAddCredentialsButtonState();

    void onPasswordUnlocked(const QString & service, const QString & login, const QString & password, bool success);
    void onCredentialUpdated(const QString & service, const QString & login, const QString & description, bool success);

    void saveSelectedCredential(QModelIndex idx = {});

    void on_pushButtonEnterMMM_clicked();
    void on_buttonDiscard_clicked();
    void on_buttonSaveChanges_clicked();

    void requestPasswordForSelectedItem();
    void on_addCredentialButton_clicked();

private:
    Ui::CredentialsManagement *ui;

    CredentialsModel *credModel = nullptr;
    CredentialsFilterModel *credFilterModel = nullptr;
    WSClient *wsClient = nullptr;
};

#endif // CREDENTIALSMANAGEMENT_H
