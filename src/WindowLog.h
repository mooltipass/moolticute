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
#ifndef WINDOWLOG_H
#define WINDOWLOG_H

#include <QtWidgets>
#include <QtCore>

namespace Ui {
class WindowLog;
}

class WindowLog : public QMainWindow
{
    Q_OBJECT

public:
    explicit WindowLog(QWidget *parent = 0);
    ~WindowLog();

    void appendData(const QByteArray &logdata);

private slots:
    void on_pushButtonClose_clicked();

    void on_pushButtonClear_clicked();

private:
    Ui::WindowLog *ui;
};

#endif // WINDOWLOG_H
