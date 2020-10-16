#include "TOTPCredential.h"
#include "ui_TOTPCredential.h"

TOTPCredential::TOTPCredential(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TOTPCredential)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
}

TOTPCredential::~TOTPCredential()
{
    delete ui;
}

QString TOTPCredential::getSecretKey() const
{
    return ui->lineEditSecretKey->text();
}

int TOTPCredential::getTimeStep() const
{
    return ui->spinBoxTimeStep->value();
}

int TOTPCredential::getCodeSize() const
{
    return ui->spinBoxCodeSize->value();
}
