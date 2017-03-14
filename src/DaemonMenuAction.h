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
#ifndef DAEMONMENUACTION_H
#define DAEMONMENUACTION_H

#include <QtWidgets>

class DaemonMenuAction : public QWidgetAction
{
    Q_OBJECT
public:
    DaemonMenuAction(QWidget *parent = nullptr);

    typedef enum
    {
        StatusUnknown = 0,
        StatusRunning = 1,
        StatusStopped = 2,
        StatusRestarting = 3,
    } DaemonStatus;

    void forceRepaint();

public slots:
    void updateStatus(DaemonStatus status);

signals:
    void restartClicked();

private:
    QLabel *iconLed, *txtLabel;
    QPushButton *restartButton;
    QWidget *widget;
};

Q_DECLARE_METATYPE(DaemonMenuAction::DaemonStatus)

#endif // DAEMONMENUACTION_H
