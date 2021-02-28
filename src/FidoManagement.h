#ifndef FIDOMANAGEMENT_H
#define FIDOMANAGEMENT_H

#include <QtWidgets>
#include "WSClient.h"

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

private:
    Ui::FidoManagement *ui;
    WSClient *wsClient;

    QStandardItemModel *filesModel;
    QStandardItem *currentItem = nullptr;
    QStringList deletedList;
};

#endif // FIDOMANAGEMENT_H
