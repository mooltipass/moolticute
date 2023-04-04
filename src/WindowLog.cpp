/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#include "WindowLog.h"
#include "ui_WindowLog.h"
#include "AppGui.h"

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

void WindowLog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QMainWindow::changeEvent(event);
}

void WindowLog::on_pushButtonSaveLog_clicked()
{
    QSettings s;
    QString fname = AppGui::getSaveFileName(this, tr("Save logs to file"),
                                                 s.value("last_used_path/save_logs_dir", QDir::homePath()).toString(),
                                                 "Text file (*.txt);;All files (*.*)");
    if (!fname.isEmpty())
    {
#if defined(Q_OS_LINUX)
        /**
         * getSaveFileName is using native dialog
         * On Linux it is not saving the choosen extension,
         * so need to add it from code.
         */
        const QString TXT_EXT = ".txt";
        if (!fname.endsWith(TXT_EXT))
        {
            fname += TXT_EXT;
        }
#endif
        QFile f(fname);
        if (!f.open(QFile::WriteOnly | QFile::Truncate))
        {
            QMessageBox::warning(this, tr("Error"), tr("Unable to write to file %1").arg(fname));
        }
        else
        {
            f.write(ui->plainTextEdit->toPlainText().toUtf8());
            QMessageBox::information(this, tr("Moolticute"), tr("Successfully wrote logs to selected file."));
        }
        f.close();

        s.setValue("last_used_path/save_logs_dir", QFileInfo(fname).canonicalPath());
    }
}

