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
#include "PageantWin.h"
#include "Common.h"
#include "version.h"
#include "ressource.h"

#define PAGEANT_NAME    L"Pageant"
#define AGENT_COPYDATA_ID 0x804e50ba
#define AGENT_MAX_MSGLEN  8192

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

PageantWin::PageantWin(int &argc, char **argv):
    QApplication(argc, argv)
{
}

PageantWin::~PageantWin()
{
    cleanWin();
}

bool PageantWin::initialize()
{
    Common::installMessageOutputHandler();

    QCoreApplication::setOrganizationName("Raoulh");
    QCoreApplication::setOrganizationDomain("raoulh.org");
    QCoreApplication::setApplicationName("Moolticute");

    qInfo() << "Moolticute Pageant Daemon version: " << APP_VERSION;
    qInfo() << "(c) 2016 Raoul Hecky";
    qInfo() << "https://github.com/raoulh/moolticute";
    qInfo() << "------------------------------------";

    if (pageantAlreadyRunning())
    {
        qCritical() << "A Pageant is already running. Abort.";
        return false;
    }

    if (!createPageantWindow())
    {
        qCritical() << "Can't create pageant window";
        return false;
    }

    return true;
}

bool PageantWin::pageantAlreadyRunning()
{
    HWND hwnd;
    hwnd = FindWindow(PAGEANT_NAME, PAGEANT_NAME);
    return hwnd;
}

void PageantWin::cleanWin()
{
    UnregisterClass(PAGEANT_NAME, m_hInstance);
}

bool PageantWin::createPageantWindow()
{
    m_hInstance = (HINSTANCE)::GetModuleHandle(NULL);

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = PAGEANT_NAME;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        qDebug() << "Failed to register class: " << getLastError();
        return false;
    }

    winHandle = CreateWindow(PAGEANT_NAME, PAGEANT_NAME,
                             WS_OVERLAPPEDWINDOW | WS_VSCROLL,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             100, 100, NULL, NULL, m_hInstance, NULL);
    if (!winHandle)
    {
        qDebug() << "Failed to create window: " << getLastError();
        return false;
    }

    ShowWindow(winHandle, SW_HIDE);

    return true;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_COPYDATA:
        dynamic_cast<PageantWin *>(qApp)->handleWmCopyMessage((COPYDATASTRUCT *)lParam);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

QString PageantWin::getLastError()
{
    auto e = GetLastError();
    QString rc = QString::fromLatin1("#%1: ").arg(e);
    ushort *lpMsgBuf;

    const int len = FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_SYSTEM
                        | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, e, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (len)
    {
        rc = QString::fromUtf16(lpMsgBuf, len);
        LocalFree(lpMsgBuf);
    }
    else
        rc += QString::fromLatin1("<unknown error>");

    return rc;
}

void PageantWin::handleWmCopyMessage(COPYDATASTRUCT *data)
{
    if (data->dwData != AGENT_COPYDATA_ID)
        return; //not a putty message

    qDebug() << "Received PUTTY message";

    char *mapname = (char *)data->lpData;
    if (mapname[data->cbData - 1] != '\0')
    {
        qWarning() << "Wrong putty message";
        return;
    }

    qDebug() << "mapname is: " << mapname;
}
