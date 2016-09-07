#include "DialogEdit.h"
#include "ui_DialogEdit.h"
#include <QtAwesome.h>

DialogEdit::DialogEdit(CredentialsModel *creds, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogEdit),
    credentials(creds)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QtAwesome *awesome = new QtAwesome(this);
    awesome->initFontAwesome();

    ui->setupUi(this);

    ui->pushButtonGen->setIcon(awesome->icon(fa::key));

    for (int i = 0;i < credentials->rowCount();i++)
    {
        ui->comboBoxService->addItem(credentials->item(i)->text());
    }
}

DialogEdit::~DialogEdit()
{
    delete ui;
}

void DialogEdit::setService(const QString &s)
{
    ui->comboBoxService->setCurrentText(s);
}

void DialogEdit::setLogin(const QString &s)
{
    ui->lineEditLogin->setText(s);
}

void DialogEdit::setPassword(const QString &s)
{
    ui->lineEditPass->setText(s);
}

void DialogEdit::setDescription(const QString &s)
{
    ui->lineEditDesc->setText(s);
}

QString DialogEdit::getService()
{
    return ui->comboBoxService->currentText();
}

QString DialogEdit::getLogin()
{
    return ui->lineEditLogin->text();
}

QString DialogEdit::getPassword()
{
    return ui->lineEditPass->text();
}

QString DialogEdit::getDescription()
{
    return ui->lineEditDesc->text();
}
