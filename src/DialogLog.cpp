#include "DialogLog.h"
#include "ui_DialogLog.h"

DialogLog::DialogLog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogLog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_DeleteOnClose, true); //delete the dialog on close
    ui->setupUi(this);    
}

DialogLog::~DialogLog()
{
    delete ui;
}

void DialogLog::appendData(const QByteArray &logdata)
{
    ui->plainTextEdit->appendMessage(QString::fromUtf8(logdata));
}
