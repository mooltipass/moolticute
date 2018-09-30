#include "SystemNotificationWindows.h"
#include <QDebug>
#include <QRegExp>

SystemNotificationWindows::SystemNotificationWindows(QObject *parent)
    : ISystemNotification(parent)
{
    process = new QProcess();
    connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
    [=]  (int exitCode, QProcess::ExitStatus exitStatus)
    {
        qDebug() << "Exit Code: " << exitCode;
        qDebug() << "ExitStatus: " << exitStatus;
        QString output(process->readAllStandardOutput());
        qDebug() << output;
        QString result = "";
        bool isResultOK = this->processResult(output, result);
        if (isResultOK)
        {
            qDebug() << result;
        }
        else
        {
            qDebug() << "No result found";
        }
    });
}

SystemNotificationWindows::~SystemNotificationWindows()
{
    delete process;
}

void SystemNotificationWindows::createNotification(const QString &title, const QString text)
{
    QString notification = SNORETOAST_FORMAT.arg(title, text, "");
    qDebug() << notification;
    process->start(notification);
}

void SystemNotificationWindows::createButtonChoiceNotification(const QString &title, const QString text, const QStringList &buttons)
{
    if (buttons.empty())
    {
        qDebug() << "createButtonChoiceNotification called without button text";
        return;
    }
    QString buttonString = "-b \"";
    for (const auto &piece : buttons)
    {
        buttonString += piece + ";";
    }

    buttonString = buttonString.replace(buttonString.size()-1, 1, "\"");

    QString notification = SNORETOAST_FORMAT.arg(title, text, buttonString);
    qDebug() << notification;
    process->start(notification);
}

void SystemNotificationWindows::createTextBoxNotification(const QString &title, const QString text)
{
    QString notification = SNORETOAST_FORMAT.arg(title, text, "-tb");
    process->start(notification);
}

bool SystemNotificationWindows::processResult(const QString &toastResponse, QString &result) const
{
    constexpr int RESULT_REGEX_NUM = 2;
    const QRegExp rx("(Result:\\[)(.*)(\\]\r\n)");

    if (rx.indexIn(toastResponse) != -1) {
        result = rx.cap(RESULT_REGEX_NUM).trimmed();
        return true;
    }
    return false;
}
