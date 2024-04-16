#include "SystemNotificationWindows.h"
#include <QDebug>
#include <QSysInfo>
#include <QSettings>

#include "Common.h"
#include "RequestLoginNameDialog.h"
#include "RequestDomainSelectionDialog.h"
#include <windows.h>

// from ntdef.h
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

// from ntdef.h
typedef struct _WNF_STATE_NAME
{
    ULONG Data[2];
} WNF_STATE_NAME;

typedef struct _WNF_STATE_NAME* PWNF_STATE_NAME;
typedef const struct _WNF_STATE_NAME* PCWNF_STATE_NAME;

typedef struct _WNF_TYPE_ID
{
    GUID TypeId;
} WNF_TYPE_ID, *PWNF_TYPE_ID;

typedef const WNF_TYPE_ID* PCWNF_TYPE_ID;

typedef ULONG WNF_CHANGE_STAMP, *PWNF_CHANGE_STAMP;

typedef NTSTATUS (NTAPI *PNTQUERYWNFSTATEDATA)(
    _In_ PWNF_STATE_NAME StateName,
    _In_opt_ PWNF_TYPE_ID TypeId,
    _In_opt_ const VOID* ExplicitScope,
    _Out_ PWNF_CHANGE_STAMP ChangeStamp,
    _Out_writes_bytes_to_opt_(*BufferSize, *BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize);

const QString SystemNotificationWindows::SNORETOAST= "SnoreToast.exe";
const QString SystemNotificationWindows::SNORETOAST_INSTALL= "-install";
const QString SystemNotificationWindows::NOTIFICATIONS_SETTING_REGENTRY = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings";
const QString SystemNotificationWindows::DND_ENABLED_REGENTRY = "NOC_GLOBAL_SETTING_TOASTS_ENABLED";
const QString SystemNotificationWindows::TOAST_ENABLED_SETTING_REGPATH = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\PushNotifications";
const QString SystemNotificationWindows::TOAST_ENABLED_REGENTRY = "ToastEnabled";
const QString SystemNotificationWindows::ICON = "icon.png";
const bool SystemNotificationWindows::IS_WIN10_OR_ABOVE = QSysInfo::productVersion().toInt() >= WINDOWS10_VERSION;


SystemNotificationWindows::SystemNotificationWindows(QObject *parent)
    : ISystemNotification(parent)
{
    process = new QProcess();
    notificationMap = new NotificationMap();
    messageMap = new MessageMap();
    installSnoreToast();
}

SystemNotificationWindows::~SystemNotificationWindows()
{
    delete process;
    for (auto &proc : notificationMap->values())
    {
        if (proc != nullptr)
        {
            delete proc;
        }
    }
    if (notificationMap != nullptr)
    {
        delete notificationMap;
    }

    if (messageMap != nullptr)
    {
        delete messageMap;
    }
}

void SystemNotificationWindows::createNotification(const QString &title, const QString text)
{
    emit notifySystray(title, text);
}

void SystemNotificationWindows::createButtonChoiceNotification(const QString &title, const QString text, const QStringList &buttons)
{
    if (buttons.empty())
    {
        qDebug() << "createButtonChoiceNotification called without button text";
        return;
    }
    QString buttonString = "";
    for (const auto &piece : buttons)
    {
        buttonString += piece + ";";
    }

    buttonString = buttonString.replace(buttonString.size()-1, 1, "");

    QProcess *proc = new QProcess();
    notificationMap->insert(notificationId, proc);
    connect(proc, static_cast<CallbackType>(&QProcess::finished), this, &SystemNotificationWindows::callbackFunction);
    QStringList choiceArgs = { "-t", title, "-m", text, "-b", buttonString, "-id", QString::number(notificationId++), "-p", ICON, "-w"};
    proc->start(SNORETOAST, choiceArgs);
}

void SystemNotificationWindows::createTextBoxNotification(const QString &title, const QString text)
{
    QProcess *proc = new QProcess();
    notificationMap->insert(notificationId, proc);
    QStringList textNotiArgs = { "-t", title, "-m", text, "-tb", "-id", QString::number(notificationId++), "-p", ICON, "-w"};
    connect(proc, static_cast<CallbackType>(&QProcess::finished), this, &SystemNotificationWindows::callbackFunction);
    proc->start(SNORETOAST, textNotiArgs);
}

bool SystemNotificationWindows::displayLoginRequestNotification(const QString &service, QString &loginName, QString message)
{
    if (IS_WIN10_OR_ABOVE && !isDoNotDisturbEnabled())
    {
        // A text box notification is displayed on Win10
        messageMap->insert(notificationId, message);
        createTextBoxNotification(tr("A credential without a login has been detected."), tr("Login name for ") + service + ":");
        return false;
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

bool SystemNotificationWindows::displayDomainSelectionNotification(const QString &domain, const QString &subdomain, QString &serviceName, QString message)
{
    if (IS_WIN10_OR_ABOVE && !isDoNotDisturbEnabled())
    {
        // A button choice notification is displayed on Win10
        QStringList buttons;
        buttons.append({domain, subdomain});
        messageMap->insert(notificationId, message);
        createButtonChoiceNotification(tr("Subdomain Detected!"), tr("Choose the domain name:"), buttons);
        return false;
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

void SystemNotificationWindows::installSnoreToast()
{
    if (IS_WIN10_OR_ABOVE)
    {
        process->start(SNORETOAST, {SNORETOAST_INSTALL});
    }
}

bool SystemNotificationWindows::processResult(const QString &toastResponse, QString &result, size_t &id) const
{
    constexpr int RESULT_REGEX_NUM = 2;
    constexpr int ID_REGEX_NUM = 4;
    const RegExp rx("(Result:\\[)(.*)(\\]\\[)(.*)(\\])(\r\n)");
#if QT_VERSION < 0x060000
    if (rx.indexIn(toastResponse) != -1)
    {
        result = rx.cap(RESULT_REGEX_NUM).trimmed();
        id = rx.cap(ID_REGEX_NUM).toUInt();
        return true;
    }
#else
    auto match = rx.match(toastResponse);
    if (match.hasMatch())
    {
        result = match.captured(RESULT_REGEX_NUM).trimmed();
        id = match.captured(ID_REGEX_NUM).toUInt();
        return true;
    }
#endif
    return false;
}

bool SystemNotificationWindows::isDoNotDisturbEnabled() const
{
    QSettings settings(NOTIFICATIONS_SETTING_REGENTRY, QSettings::NativeFormat);
    bool isDoNotDisturb = !settings.value(DND_ENABLED_REGENTRY).isNull();
    QSettings toastSetting(TOAST_ENABLED_SETTING_REGPATH, QSettings::NativeFormat);
    if (toastSetting.contains(TOAST_ENABLED_REGENTRY))
    {
        isDoNotDisturb |= !toastSetting.value(TOAST_ENABLED_REGENTRY).toBool();
    }

    if (!isDoNotDisturb)
    {
      isDoNotDisturb = isFocusAssistEnabled();
    }

    return isDoNotDisturb;
}

bool SystemNotificationWindows::isFocusAssistEnabled() const
{
    // note: ntdll is guaranteed to be in the process address space.
    const auto h_ntdll = GetModuleHandle(TEXT("ntdll"));

    // get pointer to function
    const auto pNtQueryWnfStateData = PNTQUERYWNFSTATEDATA(GetProcAddress(h_ntdll, "NtQueryWnfStateData"));
    if (!pNtQueryWnfStateData)
    {
        qCritical() << "Error: couldn't get pointer to NtQueryWnfStateData() function.";
        return false;
    }

    // state name for active hours (Focus Assist)
    WNF_STATE_NAME WNF_SHEL_QUIETHOURS_ACTIVE_PROFILE_CHANGED{0xA3BF1C75, 0xD83063E};

    // note: we won't use it but it's required
    WNF_CHANGE_STAMP change_stamp = {0};

    // on output buffer will tell us the status of Focus Assist
    DWORD buffer = 0;
    ULONG buffer_size = sizeof(buffer);

    if (NT_SUCCESS(pNtQueryWnfStateData(&WNF_SHEL_QUIETHOURS_ACTIVE_PROFILE_CHANGED, nullptr, nullptr, &change_stamp,
        &buffer, &buffer_size)))
    {
        return (FocusAssistResult::PRIORITY_ONLY == buffer ||
                FocusAssistResult::ALARMS_ONLY == buffer);
    }
    else
    {
        qCritical() << "Error: Couldn't get focus assist status, pNtQueryWnfStateData failed." ;
        return false;
    }
}

void SystemNotificationWindows::callbackFunction(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    QString output(static_cast<QProcess*>(QObject::sender())->readAllStandardOutput());
    QString result = "";
    size_t id = 0;
    if (this->processResult(output, result, id))
    {
        if (messageMap->contains(id))
        {
            QString message = (*messageMap)[id];
            if (message.contains("request_login"))
            {
                emit sendLoginMessage(message, result);
            }
            else if (message.contains("request_domain"))
            {
                emit sendDomainMessage(message, result);
            }
            messageMap->remove(id);
        }
    }
    else
    {
        qDebug() << "No result found";
    }
}
