#include "BleDev.h"
#include "ui_BleDev.h"

BleDev::BleDev(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BleDev)
{
    ui->setupUi(this);
}

BleDev::~BleDev()
{
    delete ui;
}
