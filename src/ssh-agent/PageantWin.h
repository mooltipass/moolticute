#ifndef PAGEANTWIN_H
#define PAGEANTWIN_H

#include <QApplication>
#include <QWidget>
#include <qt_windows.h>

class PageantWin : public QObject
{
    Q_OBJECT
public:
    static PageantWin *Instance()
    {
        static PageantWin p;
        return &p;
    }
    ~PageantWin();

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
