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
#include "WSServerCon.h"
#include "WSServer.h"
#include "version.h"
#include "ParseDomain.h"
#include "MPDeviceBleImpl.h"
#include "HaveIBeenPwned.h"

#include <QCryptographicHash>

WSServerCon::WSServerCon(QWebSocket *conn):
    wsClient(conn),
    clientUid(Common::createUid(QStringLiteral("ws-"))),
    hibp(new HaveIBeenPwned(this))
{
    connect(wsClient, &QWebSocket::textMessageReceived, this, &WSServerCon::processMessage);
    connect(hibp, &HaveIBeenPwned::sendPwnedMessage, this, &WSServerCon::sendHibpNotification);
}

WSServerCon::~WSServerCon()
{
    delete wsClient;
}

void WSServerCon::sendJsonMessage(const QJsonObject &data)
{
    QJsonDocument jdoc(data);
    wsClient->sendTextMessage(jdoc.toJson(QJsonDocument::JsonFormat::Compact));
    // wsClient->flush();
}

void WSServerCon::sendJsonMessageString(const QString &data)
{
    wsClient->sendTextMessage(data);
}

void WSServerCon::processMessage(const QString &message)
{
    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(message.toUtf8(), &err);

    if (message.startsWith("{\"ping"))
    {
        return;
    }
    qDebug().noquote() << "JSON API recv:" << Common::maskLog(message);

    if (err.error != QJsonParseError::NoError)
    {
        qWarning() << "JSON parse error " << err.errorString();
        return;
    }

    QJsonObject root = jdoc.object();

    /* API that does not require device */
    if (root["msg"] == "show_app")
    {
        //broadcast the message to all clients
        emit notifyAllClients(root);
        return;
    }
    else if (root["msg"] == "get_application_id")
    {
        QJsonObject ores;
        QJsonObject oroot = root;
        ores["application_name"] = "moolticute";
        ores["application_version"] = QStringLiteral(APP_VERSION);
        oroot["data"] = ores;
        oroot["msg"] = "get_application_id";
        sendJsonMessage(oroot);
        return;
    }
    else if (root["msg"] == "show_status_notification_warning")
    {
        QJsonDocument showWarningDoc(root);
        bool isGuiRunning = false;
        emit sendMessageToGUI(showWarningDoc.toJson(), isGuiRunning);
        if (!isGuiRunning)
        {
            qDebug() << "Cannot show status notification warning, because Moolticute is not running";
        }
        return;
    }

    //Strip the data for the progress lambda,
    //uneeded data should not be passed around
    QJsonObject rootStripped = root;
    rootStripped.remove("data");

    //Default progress callback handling
    auto defaultProgressCb = [=](QVariantMap progressData)
    {
        if (!WSServer::Instance()->checkClientExists(this))
            return;

        int total = progressData.value("total", QVariant(-1)).toInt();
        int current = progressData.value("current", QVariant(-1)).toInt();

        if (current > total)
            current = total;

        QJsonObject ores;
        QJsonObject oroot = rootStripped;
        ores["progress_total"] = total;
        ores["progress_current"] = current;

        oroot["msg"] = "progress"; //change msg to avoid breaking of client waiting of the response
        sendJsonMessage(oroot);

        // New progress message
        if (progressData.contains("msg")) {
            ores["progress_message"] = progressData["msg"].toString();
            if (progressData.contains("msg_args"))
            {
                auto message_args = progressData["msg_args"].toList();
                auto message_args_json = QJsonValue::fromVariant(message_args);
                ores["progress_message_args"] = message_args_json;
            }
        }
        oroot["data"] = ores;
        oroot["msg"] = "progress_detailed"; //change msg to avoid breaking of client waiting of the response
        sendJsonMessage(oroot);
    };

    if (!mpdevice)
    {
        sendFailedJson(root, "No device connected");
        return;
    }

    if (checkMemModeEnabled(root))
        return;

    if (root["msg"] == "get_random_numbers")
    {
        mpdevice->getRandomNumber([=](bool success, QString errstr, const QByteArray &rndNums)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject oroot = root;
            QJsonArray arr;
            for (auto num : rndNums)
            {
                arr.append(static_cast<quint8>(num));
            }
            oroot["data"] = arr;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "start_memorymgmt")
    {
        QJsonObject o = root["data"].toObject();

        WSServer::Instance()->setMemLockedClient(clientUid);

        //send command to start MMM
        mpdevice->startMemMgmtMode(o["want_data"].toBool(),
                defaultProgressCb,
                [=](bool success, int errCode, QString errMsg)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                QJsonObject oroot = root;
                oroot["msg"] = "failed_memorymgmt";
                sendFailedJson(oroot, errMsg, errCode);
            }
        });
    }
    else if (root["msg"] == "exit_memorymgmt")
    {
        //send command to exit MMM
        mpdevice->exitMemMgmtMode();
    }
    else if (root["msg"] == "set_credentials")
    {
        if (!mpdevice->get_memMgmtMode())
        {
            sendFailedJson(root, "Not in memory management mode");
            return;
        }

        mpdevice->setMMCredentials(
                    root["data"].toArray(),
                    false,
                    defaultProgressCb,
                    [=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = "true";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "cancel_request")
    {
        QJsonObject o = root["data"].toObject();
        QString reqid;
        if (o.contains("request_id"))
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));

        mpdevice->cancelUserRequest(reqid);
    }
    else if (root["msg"] == "reset_card")
    {
        mpdevice->resetSmartCard([=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = "true";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        }
        );
    }
    else if (root["msg"] == "lock_device")
    {
        mpdevice->lockDevice([this, root](bool success, QString errstr)
        {
            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = "true";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "get_available_users")
    {
        mpdevice->getAvailableUsers([this, root](bool success, QString result)
        {
            if (!success)
            {
                sendFailedJson(root, result);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = "true";
            ores["num"] = result;
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "param_set")
    {
        processParametersSet(root["data"].toObject());
    }
    else if (root["msg"] == "export_database")
    {
        QString encryptionMethod  = "none";
        if (root.contains("data"))
        {
            QJsonObject o = root["data"].toObject();
            encryptionMethod = o.value("encryption").toString();
        }

        mpdevice->exportDatabase(encryptionMethod,
                                 [=](bool success, QString errstr, QByteArray fileData)
        {
            qDebug() << "send exported DB on WS: success:" << success
                     << ", fileData size:" << fileData.size()
                     << ", errstr:" << errstr;

            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["file_data"] = QString(fileData.toBase64());
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        },
        defaultProgressCb);
    }
    else if (root["msg"] == "import_database")
    {
        QJsonObject o = root["data"].toObject();

        QByteArray data = QByteArray::fromBase64(o["file_data"].toString().toLocal8Bit());
        if (data.isEmpty())
        {
            sendFailedJson(root, "file_data is empty");
            return;
        }

        mpdevice->importDatabase(data, o["no_delete"].toBool(),
                    [=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = "true";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        },
        defaultProgressCb);
    }
    else if (root["msg"] == "load_params")
    {
        mpdevice->loadParams();
    }
    else if (root["msg"] == "import_csv")
    {
        mpdevice->importFromCSV(
                    root["data"].toArray(),
                    defaultProgressCb,
                    [=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = "true";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "set_data_node")
    {
        QJsonObject o = root["data"].toObject();
        QString service = o["service"].toString();
        QByteArray data = QByteArray::fromBase64(o["node_data"].toString().toLocal8Bit());
        if (data.isEmpty())
        {
            sendFailedJson(root, "node_data is empty");
            return;
        }

        int maxSize = MP_MAX_FILE_SIZE;
        if (service.toLower() == MC_SSH_SERVICE)
            maxSize = MP_MAX_SSH_SIZE;
        if (data.size() > maxSize)
        {
            sendFailedJson(root, "data is too big to be stored in device");
            return;
        }

        mpdevice->setDataNode(service, data,
                [=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            ores["service"] = service;
            QJsonObject oroot = root;
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        },
        defaultProgressCb);
    }
    else if (root["msg"] == "get_data_node")
    {
        QJsonObject o = root["data"].toObject();
        QString reqid;
        if (o.contains("request_id"))
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));

        mpdevice->getDataNode(o["service"].toString(), o["fallback_service"].toString(),
                reqid,
                [=](bool success, QString errstr, const QString &service, const QByteArray &dataNode)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["service"] = service;
            ores["node_data"] = QString(dataNode.toBase64());
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        },
        defaultProgressCb);
    }
    else if (root["msg"] == "delete_data_nodes")
    {
        QJsonObject o = root["data"].toObject();

        if (!mpdevice->get_memMgmtMode())
        {
            sendFailedJson(root, "Not in memory management mode");
            return;
        }

        QJsonArray jarr = o["services"].toArray();
        QStringList services;
        for (int i = 0;i < jarr.size();i++)
            services.append(jarr[i].toString());

        mpdevice->deleteDataNodesAndLeave(services,
                [=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject oroot = root;
            oroot["data"] = QJsonObject({{ "success", true }});
            sendJsonMessage(oroot);
        },
        defaultProgressCb);
    }
    else if (root["msg"] == "refresh_files_cache")
    {
        mpdevice->updateFilesCache();
    }
    else if (root["msg"] == "list_files_cache")
    {
        sendFilesCache();
    }
    else if (mpdevice->isBLE())
    {
        processMessageBLE(root, defaultProgressCb);
    }
    else
    {
        processMessageMini(root, defaultProgressCb);
    }
}

void WSServerCon::sendFailedJson(QJsonObject obj, QString errstr, int errCode)
{
    QJsonObject odata;
    odata["failed"] = true;
    if (!errstr.isEmpty())
        odata["error_message"] = errstr;
    if (errCode != -999)
        odata["error_code"] = errCode;
    obj["data"] = odata;
    sendJsonMessage(obj);
}

void WSServerCon::resetDevice(MPDevice *dev)
{
    mpdevice = dev;

    if (!mpdevice)
    {
        sendJsonMessage({{ "msg", "mp_disconnected" }});
        return;
    }

    sendVersion();
    sendJsonMessage({{ "msg", "mp_connected" }});

    //Whenever mp status changes, send state update to client
    connect(mpdevice, &MPDevice::statusChanged, this, &WSServerCon::statusChanged);

    mpdevice->settings()->connectSendParams(this);

    connect(mpdevice, SIGNAL(memMgmtModeChanged(bool)), this, SLOT(sendMemMgmtMode()));
    connect(mpdevice, SIGNAL(uidChanged(qint64)), this, SLOT(sendDeviceUID()));
    connect(mpdevice, SIGNAL(hwVersionChanged(QString)), this, SLOT(sendVersion()));
    connect(mpdevice, SIGNAL(serialNumberChanged(quint32)), this, SLOT(sendVersion()));
    connect(mpdevice, SIGNAL(flashMbSizeChanged(int)), this, SLOT(sendVersion()));

    connect(mpdevice, &MPDevice::filesCacheChanged, this, &WSServerCon::sendFilesCache);

    connect(mpdevice, &MPDevice::dbChangeNumbersChanged, this, &WSServerCon::sendCardDbMetadata);

    if (nullptr != mpdevice->ble())
    {
        const auto* mpBle = mpdevice->ble();
        connect(mpBle, &MPDeviceBleImpl::userSettingsChanged, this, &WSServerCon::sendUserSettings);
        connect(mpBle, &MPDeviceBleImpl::bleDeviceLanguage, this, &WSServerCon::sendDeviceLanguage);
        connect(mpBle, &MPDeviceBleImpl::bleKeyboardLayout, this, &WSServerCon::sendKeyboardLayout);
        connect(mpBle, &MPDeviceBleImpl::batteryPercentChanged, this, &WSServerCon::sendBatteryPercent);
        connect(mpBle, &MPDeviceBleImpl::userCategoriesFetched, this, &WSServerCon::sendUserCategories);
        connect(mpBle, &MPDeviceBleImpl::bundleVersionChanged, this, &WSServerCon::sendVersion);
    }
}

void WSServerCon::statusChanged()
{
    qDebug() << "Update client status changed: " << this;
    if (!mpdevice)
        return;
    sendJsonMessage({{ "msg", "status_changed" },
                     { "data", Common::MPStatusString[mpdevice->get_status()] }});
}

void WSServerCon::sendParams(int value, int param)
{
    DeviceSettings *settings = mpdevice->settings();
    if (!settings)
        return;
    QJsonObject data = {{ "parameter", settings->getParamName(MPParams::Param(param)) },
                        { "value", value }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendParams(bool value, int param)
{
    DeviceSettings *settings = mpdevice->settings();
    if (!settings)
        return;
    QJsonObject data = {{ "parameter", settings->getParamName(MPParams::Param(param)) },
                        { "value", value }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendInitialStatus()
{
    //Sends initial status to any new connected client
    //is any mp connected? and if true send mp state too

    if (!mpdevice)
        sendJsonMessage({{ "msg", "mp_disconnected" }});
    else
    {
        sendJsonMessage({{ "msg", "mp_connected" }});
        sendJsonMessage({{ "msg", "status_changed" },
                         { "data", Common::MPStatusString[mpdevice->get_status()] }});
        mpdevice->settings()->sendEveryParameter();
        sendVersion();
        sendMemMgmtMode();
        sendCardDbMetadata();
    }
}

void WSServerCon::sendMemMgmtMode()
{
    if (!mpdevice)
        return;
    sendJsonMessage({{ "msg", "memorymgmt_changed" },
                     { "data", mpdevice->get_memMgmtMode() }});

    QJsonArray logins;
    foreach (MPNode *n, mpdevice->getLoginNodes())
    {
        logins.append(n->toJson());
    }

    QJsonArray datas;
    foreach (MPNode *n, mpdevice->getDataNodes())
    {
        datas.append(n->toJson());
    }

    QJsonObject jdata;
    jdata["login_nodes"] = logins;
    jdata["data_nodes"] = datas;

    sendJsonMessage({{ "msg", "memorymgmt_data" },
                     { "data", jdata }});
}

void WSServerCon::sendVersion()
{
    DeviceSettings *settings = mpdevice->settings();
    if (!settings)
        return;
    QJsonObject data = {{ "hw_version", mpdevice->get_hwVersion() },
                        { "flash_size", mpdevice->get_flashMbSize() }};
    data["hw_serial"] = static_cast<qint64>(mpdevice->get_serialNumber());
    if (mpdevice->isBLE())
    {
        data["hw_version"] = "ble";
        if (auto bleImpl = mpdevice->ble())
        {
            data["aux_mcu_version"] = bleImpl->get_auxMCUVersion();
            data["main_mcu_version"] = bleImpl->get_mainMCUVersion();
            data["bundle_version"] = bleImpl->get_bundleVersion();
        }
    }
    sendJsonMessage({{ "msg", "version_changed" }, { "data", data }});
}

void WSServerCon::sendDeviceUID()
{
    if (!mpdevice)
        return;
    sendJsonMessage({{ "msg", "device_uid" },
                     { "data", QJsonObject{ {"uid", mpdevice->get_uid()} } }
                    });
}

void WSServerCon::sendFilesCache()
{
    auto deviceStatus = mpdevice->get_status();
    if (deviceStatus != Common::Unlocked && deviceStatus != Common::MMMMode)
    {
        qDebug() << "It's an unknown smartcard or it's locked, no need to search for files cache";
        return;
    }

    qDebug() << "Sending files cache";
    QJsonObject oroot = { {"msg", "files_cache_list"} };
    QJsonArray array;
    oroot["sync"] = mpdevice->isFilesCacheInSync();

    if (mpdevice->hasFilesCache())
    {
        for (QVariantMap item : mpdevice->getFilesCache())
            array.append(QJsonDocument::fromVariant(item).object());
    }
    else
    {
        qDebug() << "There is no files cache to send";
        oroot["sync"] = false;
    }

    oroot["data"] = array;
    sendJsonMessage(oroot);
}

void WSServerCon::sendDeviceLanguage(const QJsonObject &langs)
{
    if (!mpdevice || !mpdevice->ble())
    {
        return;
    }
    sendJsonMessage({{ "msg", "device_languages" },
                     { "data", langs }
                    });
}

void WSServerCon::sendKeyboardLayout(const QJsonObject &layouts)
{
    if (!mpdevice || !mpdevice->ble())
    {
        return;
    }
    sendJsonMessage({{ "msg", "keyboard_layouts" },
                     { "data", layouts }
                    });
}

void WSServerCon::sendCardDbMetadata()
{
    qDebug() << "Send card db metadata";
    QByteArray cpz = mpdevice->get_cardCPZ();
    int credentialsCn = mpdevice->get_credentialsDbChangeNumber();
    int dataCn = mpdevice->get_dataDbChangeNumber();
    if (cpz.isEmpty())
    {
        qDebug() << "There is no card data to be send.";
        return;
    }
    else
    {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData("mooltipass");
        hash.addData(cpz);
        QString cardId = hash.result().toHex();

        QJsonObject oroot = { {"msg", "card_db_metadata"} };
        QJsonObject data;
        data.insert("cardId", cardId);
        data.insert("credentialsDbChangeNumber", credentialsCn);
        data.insert("dataDbChangeNumber", dataCn);
        oroot["data"] = data;

        sendJsonMessage(oroot);
        qDebug() << "Sended card db metadata";
    }
}

void WSServerCon::sendHibpNotification(QString credInfo, QString pwnedNum)
{
    QJsonObject oroot = { {"msg", "send_hibp"} };
    QJsonObject data;
    data.insert("credinfo", credInfo);
    data.insert("pwnednum", pwnedNum);
    oroot["data"] = data;

    bool isGuiRunning;
    emit sendMessageToGUI(QJsonDocument(oroot).toJson(QJsonDocument::JsonFormat::Compact), isGuiRunning);

    if (!isGuiRunning)
    {
        qDebug() << "Cannot send pwned notification to GUI: " << credInfo << ": " << pwnedNum;
    }
}

void WSServerCon::sendUserSettings(QJsonObject settings)
{
    QJsonObject oroot = { {"msg", "send_user_settings"} };
    oroot["data"] = settings;
    sendJsonMessage(oroot);
}

void WSServerCon::sendUserCategories(QJsonObject categories)
{
    QJsonObject oroot = { {"msg", "get_user_categories"} };
    oroot["data"] = categories;
    sendJsonMessage(oroot);
}

void WSServerCon::sendBatteryPercent(int batteryPct)
{
    QJsonObject oroot = { {"msg", "send_battery"} };
    QJsonObject data;
    data.insert("battery", batteryPct);
    oroot["data"] = data;
    sendJsonMessage(oroot);
}

void WSServerCon::processParametersSet(const QJsonObject &data)
{
    DeviceSettings *settings = mpdevice->settings();
    if (!settings)
        return;

    for (auto it = data.begin(); it != data.end(); ++it)
    {
        const auto paramId = settings->getParamId(it.key());
        const auto value = it.value();
        if (value.isBool())
        {
            settings->updateParam(paramId, value.toBool());
        }
        else
        {
            settings->updateParam(paramId, value.toInt());
        }
    }
    emit parameterProcessFinished();

    //reload parameters from device after changed all params, this will trigger
    //websocket update of clients too
    settings->loadParameters();
}

QString WSServerCon::getRequestId(const QJsonValue &v)
{
    if (v.isDouble())
        return QString::number(v.toInt());
    return v.toString();
}

void WSServerCon::checkHaveIBeenPwned(const QString &service, const QString &login, const QString &password)
{
    QSettings s;
    if (s.value("settings/enable_hibp_check").toBool())
    {
        QString credInfo = service + ": " + login + ": ";
        hibp->isPasswordPwned(password, credInfo);
    }
}

void WSServerCon::processMessageMini(QJsonObject root, const MPDeviceProgressCb &cbProgress)
{
    if (root["msg"] == "start_memcheck")
    {
        //start integrity check
        mpdevice->startIntegrityCheck(
                    [=](bool success, int freeBlocks, int totalBlocks, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            QJsonObject oroot = root;
            oroot["msg"] = "memcheck";

            if (!success)
            {
                sendFailedJson(oroot, errstr);
                return;
            }

            QJsonObject ores;
            ores["memcheck_status"] = "done"; //TODO: add return info here about the result of memcheck?
            ores["free_blocks"] = freeBlocks;
            ores["total_blocks"] = totalBlocks;
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        },
        cbProgress);
    }
    else if (root["msg"] == "ask_password" ||
             root["msg"] == "get_credential")
    {
        QJsonObject o = root["data"].toObject();

        QString reqid;
        if (o.contains("request_id"))
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));

        mpdevice->getCredential(o["service"].toString(), o["login"].toString(), o["fallback_service"].toString(),
                reqid,
                [=](bool success, QString errstr, const QString &service, const QString &login, const QString &pass, const QString &desc)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            checkHaveIBeenPwned(service, login, pass);
            QJsonObject ores;
            QJsonObject oroot = root;
            ores["service"] = service;
            ores["login"] = login;
            ores["password"] = pass;
            if (mpdevice && mpdevice->isFw12()) //only add description for fw > 1.2
                ores["description"] = desc;
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "set_credential")
    {
        QJsonObject o = root["data"].toObject();
        if (!processSetCredential(root, o))
        {
            return;
        }
        mpdevice->setCredential(o["service"].toString(), o["login"].toString(),
                o["password"].toString(), o["description"].toString(), o.contains("description"),
                [=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores = o;
            QJsonObject oroot = root;
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "del_credential")
    {
        QJsonObject o = root["data"].toObject();
        mpdevice->delCredentialAndLeave(o["service"].toString(), o["login"].toString(),
                cbProgress,
                [=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject oroot = root;
            oroot["data"] = QJsonObject({{ "success", true }});
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "request_device_uid")
    {
        QJsonObject o = root["data"].toObject();
        const QByteArray key = o.value("key").toString().toUtf8().simplified();
        mpdevice->getUID(key);
    }
    else if (root["msg"] == "credential_exists")
    {
        QJsonObject o = root["data"].toObject();

        QString reqid;
        if (o.contains("request_id"))
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));

        mpdevice->serviceExists(false, o["service"].toString(),
                reqid,
                [=](bool success, QString errstr, const QString &service, bool exists)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["service"] = service;
            ores["exists"] = exists;
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "data_node_exists")
    {
        QJsonObject o = root["data"].toObject();

        QString reqid;
        if (o.contains("request_id"))
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));

        mpdevice->serviceExists(true, o["service"].toString(),
                reqid,
                [=](bool success, QString errstr, const QString &service, bool exists)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["service"] = service;
            ores["exists"] = exists;
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
}

void WSServerCon::processMessageBLE(QJsonObject root, const MPDeviceProgressCb &cbProgress)
{
    //Ble related commands
    MPDeviceBleImpl *bleImpl = mpdevice->ble();
    if (nullptr == bleImpl)
    {
        return;
    }

    if (root["msg"] == "get_debug_platinfo")
    {
        bleImpl->getDebugPlatInfo([this, root, bleImpl](bool success, QString errstr, QByteArray data)
        {
            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            auto platInfo = bleImpl->calcDebugPlatInfo(data);
            QJsonObject ores;
            QJsonObject oroot = root;
            ores["aux_major"] = platInfo[0];
            ores["aux_minor"] = platInfo[1];
            ores["main_major"] = platInfo[2];
            ores["main_minor"] = platInfo[3];
            ores["success"] = "true";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "flash_mcu")
    {
        bleImpl->flashMCU([this, root](bool success, QString errstr)
        {
            if (!success)
            {
                qCritical() << errstr;
                sendFailedJson(root, errstr);
                return;
            }
        });
    }
    else if (root["msg"] == "upload_bundle")
    {
        QJsonObject o = root["data"].toObject();
        bleImpl->uploadBundle(o["file"].toString(), o["password"].toString(),
                [this, root](bool success, QString errstr)
        {
            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = success;
            if (!success)
            {
                qCritical() << errstr;
            }
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        }, cbProgress);
    }
    else if (root["msg"] == "fetch_data")
    {
        QJsonObject o = root["data"].toObject();
        auto type = static_cast<Common::FetchType>(o["type"].toInt());
        const auto cmd = Common::FetchType::ACCELEROMETER == type ?
                    MPCmd::CMD_DBG_GET_ACC_32_SAMPLES : MPCmd::GET_RANDOM_NUMBER;
        bleImpl->fetchData(o["file"].toString(), cmd);
    }
    else if (root["msg"] == "stop_fetch_data")
    {
        bleImpl->stopFetchData();
    }
    else if (root["msg"] == "ask_password" ||
             root["msg"] == "get_credential")
    {
        QJsonObject o = root["data"].toObject();
        QString service = o["service"].toString();
        QString login = o["login"].toString();
        QString reqid;
        if (o.contains("request_id"))
        {
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));
        }
        bleImpl->getCredential(service, login, reqid, o["fallback_service"].toString(),
                [this, root, bleImpl, service, login](bool success, QString errstr, QByteArray data)
                {
                    if (!WSServer::Instance()->checkClientExists(this))
                        return;

                    if (!success)
                    {
                        sendFailedJson(root, errstr);
                        return;
                    }

                    auto cred = bleImpl->retrieveCredentialFromResponse(data, service, login);

                    checkHaveIBeenPwned(service, cred.get(BleCredential::CredAttr::LOGIN), cred.get(BleCredential::CredAttr::PASSWORD));
                    QJsonObject ores;
                    QJsonObject oroot = root;
                    ores["service"] = service;
                    ores["login"] = cred.get(BleCredential::CredAttr::LOGIN);
                    ores["desc"] = cred.get(BleCredential::CredAttr::DESCRIPTION);
                    ores["third"] = cred.get(BleCredential::CredAttr::THIRD);
                    ores["password"] = cred.get(BleCredential::CredAttr::PASSWORD);
                    oroot["data"] = ores;
                    sendJsonMessage(oroot);
                });
    }
    else if (root["msg"] == "set_credential")
    {
        QJsonObject o = root["data"].toObject();
        if (!processSetCredential(root, o))
        {
            return;
        }
        bleImpl->checkAndStoreCredential(BleCredential{o["service"].toString(), o["login"].toString(),
                                               o["description"].toString(), "", o["password"].toString()},
                                 [=](bool success, QString errstr)
                                 {
                                     if (!WSServer::Instance()->checkClientExists(this))
                                         return;

                                     if (!success)
                                     {
                                         sendFailedJson(root, errstr);
                                         return;
                                     }

                                     QJsonObject ores = o;
                                     QJsonObject oroot = root;
                                     oroot["data"] = ores;
                                     sendJsonMessage(oroot);
                                 });
    }
    else if (root["msg"] == "get_user_categories")
    {
         QJsonObject o = root["data"].toObject();
         bleImpl->getUserCategories([this, root, bleImpl](bool success, QString errstr, QByteArray data)
                 {
                     if (!WSServer::Instance()->checkClientExists(this))
                         return;

                     if (!success)
                     {
                         sendFailedJson(root, errstr);
                         return;
                     }

                     QJsonObject ores;
                     QJsonObject oroot = root;
                     bleImpl->fillGetCategory(data, ores);
                     oroot["data"] = ores;
                     sendJsonMessage(oroot);
                 });
    }
    else if (root["msg"] == "set_user_categories")
    {
         QJsonObject o = root["data"].toObject();
         bleImpl->setUserCategories(o, [this, root](bool success, QString errstr, QByteArray)
                 {
                     if (!WSServer::Instance()->checkClientExists(this))
                         return;

                     if (!success)
                     {
                         sendFailedJson(root, errstr);
                         return;
                     }

                     QJsonObject ores;
                     QJsonObject oroot = root;
                     ores["success"] = "true";
                     oroot["data"] = ores;
                     sendJsonMessage(oroot);
                 });
    }
    else if (root["msg"] == "get_user_settings")
    {
        bleImpl->sendUserSettings();
    }
    else if (root["msg"] == "request_keyboard_layout")
    {
        QJsonObject o = root["data"].toObject();
        bleImpl->readLanguages(o["only_check"].toBool());
    }
    else if (root["msg"] == "inform_locked")
    {
        mpdevice->informLocked([this, root](bool success, QString errstr)
        {
            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = "true";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "inform_unlocked")
    {
        mpdevice->informUnlocked([this, root](bool success, QString errstr)
        {
            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = "true";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "get_battery")
    {
        mpdevice->getBattery();
    }
    else if (root["msg"] == "nimh_reconditioning")
    {
        bleImpl->nihmReconditioning();
    }
    else if (root["msg"] == "request_security_challenge")
    {
        QJsonObject o = root["data"].toObject();
        const auto key = o.value("key").toString();
        bleImpl->getSecurityChallenge(key, [this, root](bool success, QString res)
        {
            if (!success)
            {
                sendFailedJson(root, res);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["success"] = "true";
            ores["challenge_response"] = res;
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else
    {
        qDebug() << root["msg"] << " message have not implemented yet for BLE";
    }
}

bool WSServerCon::checkMemModeEnabled(const QJsonObject &root)
{
    if (WSServer::Instance()->isMemModeLocked(clientUid))
    {
        sendFailedJson(root, "Device is in memory management mode");
        return true;
    }

    return false;
}

bool WSServerCon::processSetCredential(QJsonObject &root, QJsonObject &o)
{
    QString loginName = o["login"].toString();
    bool isMsgContainsExtInfo = o.contains("extension_version") || o.contains("mc_cli_version");
    bool isGuiRunning = false;
    if (loginName.isEmpty() && isMsgContainsExtInfo && !o.contains("saveLoginConfirmed"))
    {
        root["msg"] = "request_login";
        QJsonDocument requestLoginDoc(root);
        emit sendMessageToGUI(requestLoginDoc.toJson(), isGuiRunning);
        if (isGuiRunning)
        {
            return false;
        }
        qDebug() << "GUI is not running, saving credential with empty login";
    }

    QString originalService = o["service"].toString();
    ParseDomain url(originalService);
    QSettings s;
    bool isSubdomainSelectionEnabled = s.value("settings/enable_subdomain_selection").toBool() && url.isWebsite();
    bool isManualCredential = o.contains("saveManualCredential");
    if (!url.subdomain().isEmpty() && isMsgContainsExtInfo && isSubdomainSelectionEnabled && !isManualCredential && !o.contains("saveDomainConfirmed"))
    {
        root["msg"] = "request_domain";
        o["domain"] = url.getFullDomain();
        o["subdomain"] = url.getFullSubdomain();
        root["data"] = o;
        QJsonDocument requestLoginDoc(root);
        emit sendMessageToGUI(requestLoginDoc.toJson(), isGuiRunning);
        if (isGuiRunning)
        {
            return false;
        }
        qDebug() << "GUI is not running, saving credential with subdomain";
    }

    if (!o.contains("saveDomainConfirmed") && url.isWebsite())
    {
        o["service"] = url.getFullDomain();
    }

    if (isManualCredential)
    {
        o["service"] = url.getManuallyEnteredDomainName(originalService);
    }

    const QJsonDocument credDetectedDoc(QJsonObject{{ "msg", "credential_detected" }});
    emit sendMessageToGUI(credDetectedDoc.toJson(QJsonDocument::JsonFormat::Compact), isGuiRunning);

    checkHaveIBeenPwned(o["service"].toString(), loginName, o["password"].toString());
    return true;
}
