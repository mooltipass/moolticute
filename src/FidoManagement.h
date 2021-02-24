#ifndef FIDOMANAGEMENT_H
#define FIDOMANAGEMENT_H

#include <QWidget>
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

private:
    Ui::FidoManagement *ui;
    WSClient *wsClient;
};

#endif // FIDOMANAGEMENT_H
