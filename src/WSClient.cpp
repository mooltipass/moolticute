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
#include "WSClient.h"
#include "SystemNotifications/SystemNotification.h"
#include "SettingsGuiHelper.h"
#include "DeviceDetector.h"

#define WS_URI                      "ws://localhost"
#define QUERY_RANDOM_NUMBER_TIME    10 * 60 * 1000 //10 min

WSClient::WSClient(QObject *parent):
    QObject(parent)
{
    openWebsocket();

    randomNumTimer = new QTimer(this);
    connect(randomNumTimer, &QTimer::timeout, this, &WSClient::queryRandomNumbers);
    connect(SystemNotification::instance().getNotification(), &ISystemNotification::sendLoginMessage, this, &WSClient::sendLoginJson);
    connect(SystemNotification::instance().getNotification(), &ISystemNotification::sendDomainMessage, this, &WSClient::sendDomainJson);
    randomNumTimer->start(QUERY_RANDOM_NUMBER_TIME);
    m_settingsHelper = new SettingsGuiHelper(this);
}

WSClient::~WSClient()
{
    if (wsocket)
    {
        if (wsocket->state() == QAbstractSocket::ConnectedState)
            wsocket->close();
        delete wsocket;
        wsocket = nullptr;
    }
}

void WSClient::openWebsocket()
{
    if (!wsocket)
    {
        wsocket = new QWebSocket();
        connect(wsocket, &QWebSocket::connected, this, &WSClient::onWsConnected);
        connect(wsocket, &QWebSocket::disconnected, this, &WSClient::onWsDisconnected);
        connect(wsocket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(onWsError()));
    }

    QString url = QString("%1:%2").arg(WS_URI).arg(MOOLTICUTE_DAEMON_PORT);
    wsocket->open(url);
}

void WSClient::closeWebsocket()
{
    if (wsocket)
    {
        wsocket->deleteLater();
        wsocket = nullptr;
    }
}

void WSClient::sendJsonData(const QJsonObject &data)
{
    if (!isConnected())
        return;

    QJsonDocument jdoc(data);
    // qDebug().noquote() << jdoc.toJson();
    wsocket->sendTextMessage(jdoc.toJson());
}

void WSClient::onWsConnected()
{
    qDebug() << "Websocket connected";
    connect(wsocket, &QWebSocket::textMessageReceived, this, &WSClient::onTextMessageReceived);
    queryRandomNumbers();
    emit wsConnected();
}

bool WSClient::isConnected() const
{
    return wsocket &&
            wsocket->state() == QAbstractSocket::ConnectedState;
}

bool WSClient::isDeviceConnected() const
{
    return get_connected();
}

void WSClient::onWsDisconnected()
{
    qDebug() << "Websocket disconnect";

    set_memMgmtMode(false);
    force_connected(false);
    closeWebsocket();

    Q_EMIT wsDisconnected();

    //Auto reconnect websocket connection on failure
    QTimer::singleShot(500, [=]()
    {
        qDebug() << "Websocket open()";
        openWebsocket();
    });
}

void WSClient::onWsError()
{
    qDebug() << "Websocket error: " << (wsocket?wsocket->errorString():"No websocket object");
    closeWebsocket();

    //Auto reconnect websocket connection on failure
    QTimer::singleShot(500, [=]()
    {
        qDebug() << "Websocket open()";
        openWebsocket();
    });
}

void WSClient::onTextMessageReceived(const QString &message)
{
    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(message.toUtf8(), &err);

    if (err.error != QJsonParseError::NoError)
    {
        qWarning() << "JSON parse error " << err.errorString();
        return;
    }
    QJsonObject rootobj = jdoc.object();
    qDebug().noquote() << "New message: " << Common::maskLog(message);

    if (rootobj["msg"] == "mp_connected")
    {
        set_connected(true);
        emit deviceConnected();
    }
    else if (rootobj["msg"] == "mp_disconnected")
    {
        set_memMgmtMode(false);
        set_connected(false);
        emit deviceDisconnected();
    }
    else if (rootobj["msg"] == "status_changed")
    {
        QString st = rootobj["data"].toString();
        set_status(Common::statusFromString(st));
    }
    else if (rootobj["msg"] == "param_changed")
    {
        m_settingsHelper->updateParameters(rootobj["data"].toObject());
    }
    else if (rootobj["msg"] == "memorymgmt_changed")
    {
        force_memMgmtMode(rootobj["data"].toBool());

        memData = QJsonObject();
        emit memoryDataChanged();
    }
    else if (rootobj["msg"] == "memorymgmt_data")
    {
        memData = rootobj["data"].toObject();
        emit memoryDataChanged();
    }
    else if (rootobj["msg"] == "ask_password")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        emit passwordUnlocked(o["service"].toString(), o["login"].toString(), o["password"].toString(), success);
    }
    else if (rootobj["msg"] == "version_changed")
    {
        QJsonObject o = rootobj["data"].toObject();
        if (o["hw_version"].toString().contains("mini"))
        {
            set_mpHwVersion(Common::MP_Mini);
        }
        else if (o["hw_version"].toString().contains("ble"))
        {
            set_mpHwVersion(Common::MP_BLE);
            set_auxMCUVersion(o["aux_mcu_version"].toString());
            set_mainMCUVersion(o["main_mcu_version"].toString());
            set_bundleVersion(o["bundle_version"].toInt());
        }
        else
        {
            set_mpHwVersion(Common::MP_Classic);
        }
        DeviceDetector::instance().setDeviceType(get_mpHwVersion());
        set_fwVersion(o["hw_version"].toString());
        set_hwSerial(o["hw_serial"].toInt());
        set_hwMemory(o["flash_size"].toInt());
    }
    else if (rootobj["msg"] == "request_domain")
    {
        QJsonObject o = rootobj["data"].toObject();
        QString domain = o["domain"].toString();
        QString subdomain = o["subdomain"].toString();
        if (!domain.isEmpty() && !subdomain.isEmpty())
        {
            QString service;
            bool abortRequest = false;
            abortRequest = !SystemNotification::instance().displayDomainSelectionNotification(domain, subdomain, service, message);
            if (abortRequest)
            {
                return;
            }

            if (service.isEmpty())
            {
                service = domain;
            }

            rootobj["msg"] = "set_credential";
            o["service"] = service;
            o["saveDomainConfirmed"] = "1";
            rootobj["data"] = o;
            sendJsonData(rootobj);
        }
    }
    else if (rootobj["msg"] == "request_login")
    {
        QJsonObject o = rootobj["data"].toObject();
        if (o["login"].toString().isEmpty())
        {
            QString loginName;
            bool abortRequest = false;
            abortRequest = !SystemNotification::instance().displayLoginRequestNotification(o["service"].toString(), loginName, message);
            if (abortRequest)
            {
                return;
            }

            rootobj["msg"] = "set_credential";
            if (loginName.isEmpty())
            {
                o["saveLoginConfirmed"] = "1";
            }
            else
            {
                o["login"] = loginName;
            }
            rootobj["data"] = o;
            sendJsonData(rootobj);
        }
    }
    else if (rootobj["msg"] == "credential_detected")
    {
        SystemNotification::instance().createNotification(tr("Credentials Detected!"), tr("Please Approve their Storage on the Mooltipass"));
    }
    else if (rootobj["msg"] == "set_credential")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o["failed"].toBool();
        auto message = success ? o["description"].toString() : o["error_message"].toString();
        emit credentialsUpdated(o["service"].toString(), o["login"].toString(), o["description"].toString(), success);
    }
    else if (rootobj["msg"] == "set_credentials")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o["failed"].toBool();
        auto message = success ? o["description"].toString() : o["error_message"].toString();
        emit credentialsUpdated(o["service"].toString(), o["login"].toString(), o["description"].toString(), success);
    }
    else if (rootobj["msg"] == "show_app")
    {
        emit showAppRequested();
    }
    else if (rootobj["msg"] == "memcheck")
    {
        QJsonObject o = rootobj["data"].toObject();
        if (o.value("failed").toBool())
        {
            emit memcheckFinished(false);
        }
        else
        {
            const auto freeBlocks = o.value("free_blocks").toInt();
            const auto totalBlocks = o.value("total_blocks").toInt();
            emit memcheckFinished(true, freeBlocks, totalBlocks);
        }
    }
    else if (rootobj["msg"] == "progress_detailed")
    {
        QJsonObject o = rootobj["data"].toObject();
        QString progressString = o["progress_message"].toString();
        // TODO: Add internationalization code here
        QJsonArray progressStringArgs = o["progress_message_args"].toArray();
        for (QJsonValue arg: progressStringArgs)
        {
            switch (arg.type()) {
            case QJsonValue::Bool:
                progressString = progressString.arg(arg.toBool());
                break;
            case QJsonValue::Double:
                progressString = progressString.arg(arg.toDouble(), 0, 'g', -1);
                break;
            case QJsonValue::String:
                progressString = progressString.arg(arg.toString());
                break;
            default:
                qWarning() << "Usuported argument in progress message: " << arg;
                break;
            }
        }
        emit progressChanged(o["progress_total"].toInt(), o["progress_current"].toInt(), progressString);
    }
    else if (rootobj["msg"] == "device_uid")
    {
        QJsonObject o = rootobj["data"].toObject();
        set_uid((qint64)o["uid"].toDouble());
    }
    else if (rootobj["msg"] == "get_data_node")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        QByteArray b = QByteArray::fromBase64(o["node_data"].toString().toLocal8Bit());
        emit dataFileRequested(o["service"].toString(), b, success);
    }
    else if (rootobj["msg"] == "set_data_node")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        emit dataFileSent(o["service"].toString(), success);
    }
    else if (rootobj["msg"] == "delete_data_node")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        emit dataFileDeleted(o["service"].toString(), success);
    }
    else if (rootobj["msg"] == "data_node_exists")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit dataNodeExists(o["service"].toString(), o["exists"].toBool());
    }
    else if (rootobj["msg"] == "credential_exists")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit credentialsExists(o["service"].toString(), o["exists"].toBool());
    }
    else if (rootobj["msg"] == "export_database")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        QByteArray b;
        if (success)
            b = QByteArray::fromBase64(o["file_data"].toString().toLocal8Bit());
        else
            b = o["error_message"].toString().toLocal8Bit();

        emit dbExported(b, success);
    }
    else if (rootobj["msg"] == "import_database")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        if (!success)
            emit dbImported(success, o["error_message"].toString());
        else
            emit dbImported(success, "");
    }
    else if (rootobj["msg"] == "import_csv")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        if (!success)
            emit dbImported(success, o["error_message"].toString());
        else
            emit dbImported(success, "");
    }
    else if (rootobj["msg"] == "failed_memorymgmt")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        if (!success)
            emit memMgmtModeFailed(o["error_code"].toInt(), o["error_message"].toString());
    }
    else if (rootobj["msg"] == "files_cache_list")
    {
        filesCache = rootobj["data"].toArray();
        emit filesCacheChanged(rootobj["sync"].toBool());
    }
    else if (rootobj["msg"] == "get_random_numbers")
    {
        std::vector<qint64> ints;
        QJsonArray jarr = rootobj["data"].toArray();
        for (int i = 0;i < jarr.size();i++)
            ints.push_back(jarr.at(i).toInt());

        qDebug() << "update seed: " << jarr;
        Common::updateSeed(ints);
    } else if (rootobj["msg"] == "card_db_metadata") {
        QJsonObject data = rootobj["data"].toObject();

        QString cardId = data["cardId"].toString();
        int credentialsDbChangeNumber = data["credentialsDbChangeNumber"].toInt();
        int dataDbChangeNumber = data["dataDbChangeNumber"].toInt();

        set_cardId(cardId);
        set_credentialsDbChangeNumber(credentialsDbChangeNumber);
        set_dataDbChangeNumber(dataDbChangeNumber);

        emit cardDbMetadataChanged(cardId, credentialsDbChangeNumber, dataDbChangeNumber);
    }
    else if (rootobj["msg"] == "card_reset")
    {
        QJsonObject o = rootobj["data"].toObject();
        if (o.value("failed").toBool())
            emit cardResetFinished(true);
        else
            emit cardResetFinished(false);
    }
    else if (rootobj["msg"] == "show_status_notification_warning")
    {
        emit displayStatusWarning();
    }
    else if (rootobj["msg"] == "show_emulator_disconnect")
    {
        SystemNotification::instance().createNotification(tr("Device connected"), tr("Disconnecting emulator from Moolticute"));
    }
    else if (rootobj["msg"] == "delete_data_nodes")
    {
        emit deleteDataNodesFinished();
    }
    else if (rootobj["msg"] == "get_debug_platinfo")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit displayDebugPlatInfo(o["aux_major"].toInt(), o["aux_minor"].toInt(), o["main_major"].toInt(), o["main_minor"].toInt());
    }
    else if (rootobj["msg"] == "upload_bundle")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit displayUploadBundleResult(o["success"].toBool());
    }
    else if (rootobj["msg"] == "send_hibp")
    {
        QJsonObject o = rootobj["data"].toObject();
        QString message = o["credinfo"].toString() + HIBP_COMPROMISED_FORMAT;
        QString pwnedNum = o["pwnednum"].toString();
        SystemNotification::instance().createNotification(tr("Password Compromised"), message.arg(pwnedNum));
    }
    else if(rootobj["msg"] == "get_available_users")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit displayAvailableUsers(o["num"].toString());
    }
    else if(rootobj["msg"] == "get_user_categories")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit displayUserCategories(o["category_1"].toString(),
                                   o["category_2"].toString(),
                                   o["category_3"].toString(),
                                   o["category_4"].toString());
    }
    else if(rootobj["msg"] == "set_user_categories")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        if (!success)
        {
            SystemNotification::instance().createNotification(tr("Error"), tr("Set user category failed."));
            sendGetUserCategories();
        }
    }
    else if(rootobj["msg"] == "send_user_settings")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit updateUserSettingsOnUI(o);
    }
    else if (rootobj["msg"] == "device_languages")
    {
        emit updateBLEDeviceLanguage(rootobj["data"].toObject());
    }
    else if (rootobj["msg"] == "keyboard_layouts")
    {
        emit updateBLEKeyboardLayout(rootobj["data"].toObject());
    }
    else if (rootobj["msg"] == "send_battery")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit updateBatteryPercent(o["battery"].toInt());
    }
    else if (rootobj["msg"] == "request_security_challenge")
    {
        QJsonObject o = rootobj["data"].toObject();
        if (o.value("failed").toBool())
        {
            emit challengeResultFailed();
        }
        else
        {
            emit challengeResultReceived(o["challenge_response"].toString());
        }
    }
}

bool WSClient::isFwVersion(int version) const
{
    static QRegularExpression regVersion("v([0-9]+)\\.([0-9]+)(.*)");
    QRegularExpressionMatchIterator i = regVersion.globalMatch(m_fwVersion);
    if (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        int v = match.captured(1).toInt() * 10 +
                match.captured(2).toInt();
        return v >= version;
    }

    return false;
}

bool WSClient::isMPMini() const
{
    return  get_mpHwVersion() == Common::MP_Mini;
}

bool WSClient::isMPBLE() const
{
    return  get_mpHwVersion() == Common::MP_BLE;
}


bool WSClient::requestDeviceUID(const QByteArray & key)
{
    m_uid = -1;
    if(!isConnected())
        return false;
    sendJsonData({{ "msg", "request_device_uid" },
                  { "data", QJsonObject{ {"key", QString::fromUtf8(key) } } }
                 });
    return true;
}


void WSClient::sendEnterMMRequest(bool wantData)
{
    sendJsonData({{ "msg", "start_memorymgmt" },
                  { "data", QJsonObject{ {"want_data", wantData } } }
                 });
}

void WSClient::sendLeaveMMRequest()
{
    sendJsonData({{ "msg", "exit_memorymgmt" }});
}

void WSClient::addOrUpdateCredential(const QString &service, const QString &login,
                                     const QString &password, const QString &description)
{
    QJsonObject o = {{ "service", service.toLower()},
                     { "login",   login},
                     { "password", password },
                     { "description", description},
                     { "saveManualCredential", 1}};
    sendJsonData({{ "msg", "set_credential" },
                  { "data", o }});
}

void WSClient::requestPassword(const QString &service, const QString &login)
{
    QJsonObject d = {{ "service", service },
                     { "login", login }};
    sendJsonData({{ "msg", "ask_password" },
                  { "data", d }});
}

void WSClient::requestDataFile(const QString &service)
{
    QJsonObject d = {{ "service", service }};
    sendJsonData({{ "msg", "get_data_node" },
                  { "data", d }});
}

void WSClient::sendDataFile(const QString &service, const QByteArray &data)
{
    QJsonObject d = {{ "service", service.toLower() },
                     { "node_data", QString(data.toBase64()) }};
    sendJsonData({{ "msg", "set_data_node" },
                  { "data", d }});
}

void WSClient::deleteDataFilesAndLeave(const QStringList &services)
{
    QJsonArray s;
    for (const QString &srv: qAsConst(services))
        s.append(srv.toLower());
    QJsonObject d = {{ "services", s }};
    sendJsonData({{ "msg", "delete_data_nodes" },
                  { "data", d }});
}

void WSClient::requestResetCard()
{
    sendJsonData({{ "msg", "reset_card" }});
}

void WSClient::requestAvailableUserNumber()
{
    sendJsonData({{ "msg", "get_available_users" }});
}

void WSClient::serviceExists(bool isDatanode, const QString &service)
{
    QJsonObject d = {{ "service", service }};
    sendJsonData({{ "msg", isDatanode? "data_node_exists": "credential_exists" },
                  { "data", d }});
}

void WSClient::sendCredentialsMM(const QJsonArray &creds)
{
    sendJsonData({{ "msg", "set_credentials" },
                  { "data", creds }});
}

void WSClient::exportDbFile(const QString &encryption)
{
    QJsonObject d = {{ "encryption", encryption }};
    sendJsonData({{ "msg", "export_database" },
                  { "data", d }});
}

void WSClient::importDbFile(const QByteArray &fileData, bool noDelete)
{
    QJsonObject d = {{ "file_data", QString(fileData.toBase64()) },
                     { "no_delete", noDelete }};
    sendJsonData({{ "msg", "import_database" },
                  { "data", d }});
}

void WSClient::importCSVFile(const QList<QStringList> &fileData)
{
    QJsonArray creds;

    for ( int i = 0; i < fileData.size(); ++i )
    {
        QStringList ll = fileData[i];
        if (ll.size() < 3) {
            qWarning() << "Skiping short line:" << ll.join(",");
            continue;
        }

        QJsonObject o;
        o["service"] = ll[0];
        o["login"] = ll[1];
        o["password"] = ll[2];
        creds.append(o);
    }

    sendJsonData({{ "msg", "import_csv" },
                  { "data", creds }});
}

void WSClient::sendListFilesCacheRequest()
{
    qDebug() << "Sending cache files request";
    sendJsonData({{ "msg", "list_files_cache" }});
}

void WSClient::sendRefreshFilesCacheRequest()
{
    sendJsonData({{ "msg", "refresh_files_cache" }});
}

void WSClient::sendPlatInfoRequest()
{
    sendJsonData({{ "msg", "get_debug_platinfo" }});
}

void WSClient::sendFlashMCU()
{
    sendJsonData({{ "msg", "flash_mcu" }});
}

void WSClient::sendUploadBundle(QString bundleFilePath, QString password)
{
    QJsonObject o;
    o["file"] = bundleFilePath;
    o["password"] = password;
    sendJsonData({{ "msg", "upload_bundle" },
                  {"data", o}});
}

void WSClient::sendFetchData(QString fileName, Common::FetchType fetchType)
{
    QJsonObject o;
    o["type"] = static_cast<int>(fetchType);
    o["file"] = fileName;
    sendJsonData({{ "msg", "fetch_data" },
                  {"data", o}});
}

void WSClient::sendStopFetchData()
{
    sendJsonData({{ "msg", "stop_fetch_data" }});
}

void WSClient::sendGetUserCategories()
{
    sendJsonData({{ "msg", "get_user_categories" }});
}

void WSClient::sendSetUserCategories(const QString &cat1, const QString &cat2, const QString &cat3, const QString &cat4)
{
    QJsonObject o;
    o["category_1"] = cat1;
    o["category_2"] = cat2;
    o["category_3"] = cat3;
    o["category_4"] = cat4;
    sendJsonData({{ "msg", "set_user_categories" },
                  {"data", o}});
}

void WSClient::sendUserSettingsRequest()
{
    sendJsonData({{ "msg", "get_user_settings" }});
}

void WSClient::sendLoadParams()
{
    sendJsonData({{ "msg", "load_params" }});
}

void WSClient::sendBatteryRequest()
{
    sendJsonData({{ "msg", "get_battery" }});
}

void WSClient::sendNiMHReconditioning()
{
    sendJsonData({{ "msg", "nimh_reconditioning" }});
}

void WSClient::sendSecurityChallenge(QString str)
{
    sendJsonData({{ "msg", "request_security_challenge" },
                  { "data", QJsonObject{ {"key", str } } }
                 });
}

void WSClient::requestBleKeyboardLayout(bool onlyCheck)
{
    QJsonObject o;
    o["only_check"] = onlyCheck;
    sendJsonData({{ "msg", "request_keyboard_layout" }, {"data", o}});
}

void WSClient::sendLockDevice()
{
    sendJsonData({{ "msg", "lock_device" }});
}

void WSClient::sendInformLocked()
{
    sendJsonData({{ "msg", "inform_locked" }});
}

void WSClient::sendInformUnlocked()
{
    sendJsonData({{ "msg", "inform_unlocked" }});
}

SettingsGuiHelper* WSClient::settingsHelper()
{
    return m_settingsHelper;
}

void WSClient::queryRandomNumbers()
{
    sendJsonData({{ "msg", "get_random_numbers" }});
}

void WSClient::sendLoginJson(QString message, QString loginName)
{
    QJsonDocument jdoc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject rootobj = jdoc.object();
    QJsonObject o = rootobj["data"].toObject();
    rootobj["msg"] = "set_credential";
    if (loginName.isEmpty())
    {
        o["saveLoginConfirmed"] = "1";
    }
    else
    {
        o["login"] = loginName;
    }
    rootobj["data"] = o;
    sendJsonData(rootobj);
}

void WSClient::sendDomainJson(QString message, QString serviceName)
{
    QJsonDocument jdoc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject rootobj = jdoc.object();
    QJsonObject o = rootobj["data"].toObject();
    rootobj["msg"] = "set_credential";
    if (serviceName.isEmpty())
    {
        serviceName = o["domain"].toString();
    }

    o["service"] = serviceName;
    o["saveDomainConfirmed"] = "1";
    rootobj["data"] = o;
    sendJsonData(rootobj);
}
