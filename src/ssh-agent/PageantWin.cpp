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
#include <aclapi.h>
#include "SshAgent.h"

#define PAGEANT_NAME    L"Pageant"
#define AGENT_COPYDATA_ID 0x804e50ba
#define AGENT_MAX_MSGLEN  8192

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

PageantWin::PageantWin()
{
    qInfo() << "Starting Moolticute Pageant";

    if (pageantAlreadyRunning())
    {
        qCritical() << "A Pageant is already running. Abort.";
        return;
    }

    if (!createPageantWindow())
    {
        qCritical() << "Can't create pageant window";
        return;
    }
}

PageantWin::~PageantWin()
{
    cleanWin();
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
        PageantWin::Instance()->handleWmCopyMessage((COPYDATASTRUCT *)lParam);
        return 1;
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

    if (!checkSecurityId(mapname))
    {
        qWarning() << "SID does not match, aborting.";
        return;
    }

    QSharedMemory sh;
    sh.setNativeKey(mapname);
    if (!sh.attach(QSharedMemory::ReadWrite))
    {
        qWarning() << "Failed to attach memory:" << mapname;
        return;
    }

    qDebug() << "mapname is: " << mapname;

    quint32 msgSize = qFromBigEndian<quint32>(sh.data());
    qDebug() << "mapsize is: " << msgSize;

    SshAgent agent;
    QByteArray res = agent.processRequest(sh.data());
    memcpy(sh.data(), res.data(), res.size());
}

PSID PageantWin::getUserSid()
{
    HANDLE proc = NULL, tok = NULL;
    TOKEN_USER *user = NULL;
    DWORD toklen, sidlen;
    PSID sid = NULL, ret = NULL;

    if (usersid)
        return usersid;

    if ((proc = OpenProcess(MAXIMUM_ALLOWED, FALSE,
                            GetCurrentProcessId())) == NULL)
        goto cleanup;

    if (!OpenProcessToken(proc, TOKEN_QUERY, &tok))
        goto cleanup;

    if (!GetTokenInformation(tok, TokenUser, NULL, 0, &toklen) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        goto cleanup;

    if ((user = (TOKEN_USER *)LocalAlloc(LPTR, toklen)) == NULL)
        goto cleanup;

    if (!GetTokenInformation(tok, TokenUser, user, toklen, &toklen))
        goto cleanup;

    sidlen = GetLengthSid(user->User.Sid);

    sid = (PSID)malloc(sidlen);

    if (!CopySid(sidlen, sid, user->User.Sid))
        goto cleanup;

    /* Success. Move sid into the return value slot, and null it out
     * to stop the cleanup code freeing it. */
    ret = usersid = sid;
    sid = NULL;

cleanup:
    if (proc != NULL)
        CloseHandle(proc);
    if (tok != NULL)
        CloseHandle(tok);
    if (user != NULL)
        LocalFree(user);
    if (sid != NULL)
        free(sid);

    return ret;
}

/*
 * Versions of Pageant prior to 0.61 expected this SID on incoming
 * communications. For backwards compatibility, and more particularly
 * for compatibility with derived works of PuTTY still using the old
 * Pageant client code, we accept it as an alternative to the one
 * returned from get_user_sid() in winpgntc.c.
 */
PSID PageantWin::getDefaultSid()
{
    HANDLE proc = NULL;
    DWORD sidlen;
    PSECURITY_DESCRIPTOR psd = NULL;
    PSID sid = NULL, copy = NULL, ret = NULL;

    if ((proc = OpenProcess(MAXIMUM_ALLOWED, FALSE,
                            GetCurrentProcessId())) == NULL)
        goto cleanup;

    if (GetSecurityInfo(proc, SE_KERNEL_OBJECT, OWNER_SECURITY_INFORMATION,
                        &sid, NULL, NULL, NULL, &psd) != ERROR_SUCCESS)
        goto cleanup;

    sidlen = GetLengthSid(sid);

    copy = (PSID)malloc(sidlen);

    if (!CopySid(sidlen, copy, sid))
        goto cleanup;

    /* Success. Move sid into the return value slot, and null it out
     * to stop the cleanup code freeing it. */
    ret = copy;
    copy = NULL;

cleanup:
    if (proc != NULL)
        CloseHandle(proc);
    if (psd != NULL)
        LocalFree(psd);
    if (copy != NULL)
        free(copy);

    return ret;
}

bool PageantWin::checkSecurityId(QString mapname)
{
    HANDLE hfilemap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, (wchar_t*)mapname.utf16());
    PSID mapowner, ourself, ourselfDef;
    PSECURITY_DESCRIPTOR psd = NULL;
    bool ret = true;

    if ((ourself = getUserSid()) == NULL)
    {
        qWarning() << "Failed to get user SID";
        CloseHandle(hfilemap);
        return false;
    }

    if ((ourselfDef = getDefaultSid()) == NULL)
    {
        qWarning() << "Failed to get default SID";
        CloseHandle(hfilemap);
        return false;
    }

    int rc;
    if ((rc = GetSecurityInfo(hfilemap, SE_KERNEL_OBJECT,
                              OWNER_SECURITY_INFORMATION,
                              &mapowner, NULL, NULL, NULL,
                              &psd) != ERROR_SUCCESS))
    {
        qWarning() << "Failed to get SID info from filemap";
        CloseHandle(hfilemap);
        free(ourselfDef);
        return false;
    }

    if (!EqualSid(mapowner, ourself) &&
        !EqualSid(mapowner, ourselfDef))
    {
        qWarning() << "Security ID mismatch!";
        ret = false;
    }

    CloseHandle(hfilemap);
    LocalFree(psd);
    free(ourselfDef);

    return ret;
}
