/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Calaos is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Calaos is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#ifndef PAGEANTWIN_H
#define PAGEANTWIN_H

#include <QApplication>
#include <QWidget>
#include <QThread>
#include <qt_windows.h>

/* This implements a Putty Agent like interface.
 * It creates a fake Putty Agent window and listen to SSH requests
 * and pass them to SshAgent class.
 * It allows all existing putty compatible softwares to use moolticuted
 * as a key storage.
 *
 * PageantWin needs to be started in a thread to avoid blocking the WndProc
 * function when a request is done by a client
 */

class PageantWin: public QThread
{
    Q_OBJECT
public:
    static PageantWin *Instance()
    {
        static PageantWin p;
        return &p;
    }
    ~PageantWin();

    virtual void run();

    void handleWmCopyMessage(COPYDATASTRUCT *data);

private:
    PageantWin();

    bool pageantAlreadyRunning();
    bool createPageantWindow();
    void cleanWin();
    bool checkSecurityId(QString mapname);
    PSID getUserSid();
    PSID getDefaultSid();

    QString getLastError();

    HINSTANCE m_hInstance;
    HWND winHandle;
    PSID usersid = nullptr;
};

#endif // PAGEANTWIN_H
