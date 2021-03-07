#ifndef FIDOMANAGEMENT_H
#define FIDOMANAGEMENT_H

#include <QtWidgets>
#include "WSClient.h"
#include "CredentialModel.h"
#include "CredentialModelFilter.h"

namespace Ui {
class FidoManagement;
}

class FidoManagement : public QWidget
{
    Q_OBJECT

public:
    explicit FidoManagement(QWidget *parent = nullptr);
    void setWsClient(WSClient *c);
    ~FidoManagement();

signals:
    void wantEnterMemMode();
    void wantExitMemMode();

private slots:
    void on_pushButtonEnterFido_clicked();
    void loadModel();
    void enableMemManagement(bool enable);

    void on_pushButtonSaveExitFidoMMM_clicked();

    void on_pushButtonDiscard_clicked();

    void on_pushButtonDiscard_pressed();

    void on_pushButtonDelete_clicked();

    void onItemExpanded(const QModelIndex &proxyIndex);
    void onItemCollapsed(const QModelIndex &proxyIndex);
    void onDeleteFidoNodesFailed();

private:
    QModelIndex getSourceIndexFromProxyIndex(const QModelIndex &proxyIndex);
    QModelIndex getProxyIndexFromSourceIndex(const QModelIndex &srcIndex);


    Ui::FidoManagement *ui;
    WSClient *wsClient;

    CredentialModel *m_pCredModel = nullptr;
    CredentialModelFilter *m_pCredModelFilter = nullptr;
    QStandardItem *currentItem = nullptr;
    QList<FidoCredential> deletedList;
};

#endif // FIDOMANAGEMENT_H
