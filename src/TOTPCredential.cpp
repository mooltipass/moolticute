#include "TOTPCredential.h"
#include "ui_TOTPCredential.h"

#include <QPushButton>

const QString TOTPCredential::BASE32_REGEXP = "[^A-Z2-7=*]";

TOTPCredential::TOTPCredential(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TOTPCredential)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    ui->labelError->setStyleSheet("QLabel { color : red; }");
    ui->labelError->hide();
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

void TOTPCredential::clearFields()
{
    ui->lineEditSecretKey->clear();
    ui->spinBoxTimeStep->setValue(DEFAULT_TIME_STEP);
    ui->spinBoxCodeSize->setValue(DEFAULT_CODE_SIZE);
}

void TOTPCredential::on_lineEditSecretKey_textChanged(const QString &arg1)
{
    if (arg1.contains(QRegExp(BASE32_REGEXP)))
    {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        ui->labelError->show();
    }
    else
    {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->labelError->hide();
    }
}
