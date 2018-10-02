#include "SystemNotificationWindows.h"
#include <QDebug>
#include <QRegExp>
#include <QSysInfo>

#include "RequestLoginNameDialog.h"
#include "RequestDomainSelectionDialog.h"

const QString SystemNotificationWindows::SNORETOAST_FORMAT= "SnoreToast.exe -t \"%1\" -m \"%2\" %3 -p icon.png -w";
const int SystemNotificationWindows::NOTIFICATION_TIMEOUT = 20000;
const QString SystemNotificationWindows::WINDOWS10_VERSION = "10";

SystemNotificationWindows::SystemNotificationWindows(QObject *parent)
    : ISystemNotification(parent)
{
    process = new QProcess();
    /* Processing result from an other thread.
     * connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
    [=]  (int exitCode, QProcess::ExitStatus exitStatus)
    {
        qDebug() << "Exit Code: " << exitCode;
        qDebug() << "ExitStatus: " << exitStatus;
        QString output(process->readAllStandardOutput());
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
    });*/
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

bool SystemNotificationWindows::displayLoginRequestNotification(const QString &service, QString &loginName)
{
    if (QSysInfo::productVersion() == WINDOWS10_VERSION)
    {
        // A text box notification is displayed on Win10
        createTextBoxNotification(tr("A credential without a login has been detected."), tr("Login name for ") + service + ":");
        if (process->waitForFinished(NOTIFICATION_TIMEOUT))
        {
            return processResult(process->readAllStandardOutput(), loginName);
        }
        else
        {
            qDebug() << "A text box notification timeout";
            return false;
        }
    }
    else
    {
        // A RequestLoginNameDialog is displayed on bellow Win10
        bool isSuccess = false;
        RequestLoginNameDialog dlg(service);
        isSuccess = (dlg.exec() != QDialog::Rejected);
        loginName = dlg.getLoginName();
        return isSuccess;
    }
}

bool SystemNotificationWindows::displayDomainSelectionNotification(const QString &domain, const QString &subdomain, QString &serviceName)
{
    if (QSysInfo::productVersion() == WINDOWS10_VERSION)
    {
        // A text box notification is displayed on Win10
        QStringList buttons;
        buttons.append({domain, subdomain});
        createButtonChoiceNotification(tr("Subdomain Detected!"), tr("Choose the domain name:"), buttons);
        if (process->waitForFinished(NOTIFICATION_TIMEOUT))
        {
            return processResult(process->readAllStandardOutput(), serviceName);
        }
        else
        {
            qDebug() << "A text box notification timeout";
            return false;
        }
    }
    else
    {
        // A RequestLoginNameDialog is displayed on bellow Win10
        bool isSuccess = false;
        RequestDomainSelectionDialog dlg(domain, subdomain);
        isSuccess = (dlg.exec() != QDialog::Rejected);
        serviceName = dlg.getServiceName();
        return isSuccess;
    }
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
