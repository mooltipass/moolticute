#include "FidoManagement.h"
#include "ui_FidoManagement.h"
#include "Common.h"
#include "AppGui.h"

FidoManagement::FidoManagement(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FidoManagement)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->pageLocked);

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

    ui->pushButtonEnterFido->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonEnterFido->setIcon(AppGui::qtAwesome()->icon(fa::unlock, whiteButtons));

    ui->pushButtonSaveExitFidoMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonDiscard->setStyleSheet(CSS_BLUE_BUTTON);
}

void FidoManagement::setWsClient(WSClient *c)
{
    wsClient = c;
}

FidoManagement::~FidoManagement()
{
    delete ui;
}

void FidoManagement::on_pushButtonEnterFido_clicked()
{
    wsClient->sendEnterMMRequest(true);
    emit wantEnterMemMode();
}
