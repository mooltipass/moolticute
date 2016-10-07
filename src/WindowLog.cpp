#include "WindowLog.h"
#include "ui_WindowLog.h"

WindowLog::WindowLog(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WindowLog)
{
    setAttribute(Qt::WA_DeleteOnClose, true); //delete the dialog on close
    ui->setupUi(this);
}

WindowLog::~WindowLog()
{
    delete ui;
}

void WindowLog::appendData(const QByteArray &logdata)
{
    ui->plainTextEdit->appendMessage(QString::fromUtf8(logdata));
}

void WindowLog::on_pushButtonClose_clicked()
{
    close();
}

void WindowLog::on_pushButtonClear_clicked()
{
    ui->plainTextEdit->clear();
}
