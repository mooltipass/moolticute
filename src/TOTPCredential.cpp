#include "TOTPCredential.h"
#include "ui_TOTPCredential.h"

#include <QPushButton>
#include <QMessageBox>

const QString TOTPCredential::BASE32_REGEXP = "^(?:[A-Z2-7]{8})*(?:[A-Z2-7]{2}={0,6}|[A-Z2-7]{4}={0,4}|[A-Z2-7]{5}={0,3}|[A-Z2-7]{7}={0,1})?$";
const QString TOTPCredential::BASE32_CHAR_REGEXP = "[^A-Z2-7=*]";

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

void TOTPCredential::setTimeStep(int timeStep)
{
    ui->spinBoxTimeStep->setValue(timeStep);
}

int TOTPCredential::getCodeSize() const
{
    return ui->spinBoxCodeSize->value();
}

void TOTPCredential::setCodeSize(int codeSize)
{
    ui->spinBoxCodeSize->setValue(codeSize);
}

bool TOTPCredential::validateInput()
{
    if (!getSecretKey().contains(QRegExp(BASE32_REGEXP)))
    {
        QMessageBox::warning(this, tr("Invalid Secret Key"), tr("The entered Secret Key is not a valid Base32 string"));
        ui->lineEditSecretKey->clear();
        return false;
    }
    return true;
}

void TOTPCredential::clearFields()
{
    ui->lineEditSecretKey->clear();
    ui->spinBoxTimeStep->setValue(DEFAULT_TIME_STEP);
    ui->spinBoxCodeSize->setValue(DEFAULT_CODE_SIZE);
}

void TOTPCredential::on_lineEditSecretKey_textChanged(const QString &arg1)
{
    if (arg1.contains(QRegExp(BASE32_CHAR_REGEXP)))
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
