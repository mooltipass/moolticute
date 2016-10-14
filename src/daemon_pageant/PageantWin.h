#ifndef PAGEANTWIN_H
#define PAGEANTWIN_H

#include <QApplication>
#include <QWidget>
#include <qt_windows.h>

class PageantWin : public QApplication
{
    Q_OBJECT
public:
    PageantWin(int &argc, char **argv);
    ~PageantWin();

    bool initialize();

    void handleWmCopyMessage(COPYDATASTRUCT *data);

private:
    bool pageantAlreadyRunning();
    bool createPageantWindow();
    void cleanWin();

    QString getLastError();

    HINSTANCE m_hInstance;
    HWND winHandle;
};

#endif // PAGEANTWIN_H
