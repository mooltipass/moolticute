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
#include <QFile>
#include "HttpClient.h"
#include <QDir>

int onMessageBeginCb(http_parser *parser)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    client->m_parseUrl.clear();
    client->m_bodyMessage.clear();
    client->m_parsingDone = false;
    client->m_hasField = false;
    client->m_hasValue = false;
    client->m_hField.clear();
    client->m_hValue.clear();
    return 0;
}

int onUrlCb(http_parser *parser, const char *at, size_t length)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);
    client->m_parseUrl.append(at, length);

    return 0;
}

int onStatusCompleteCb(http_parser *parser)
{
    Q_UNUSED(parser);
    return 0;
}

int onHeaderFieldCb(http_parser *parser, const char *at, size_t length)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    if (client->m_hasField && client->m_hasValue)
    {
        client->m_requestHeaders[client->m_hField.toLower()] = client->m_hValue;
        client->m_hasField = false;
        client->m_hasValue = false;
        client->m_hField.clear();
        client->m_hValue.clear();
    }

    if (!client->m_hasField)
        client->m_hasField = true;

    client->m_hField.append(at, length);

    return 0;
}

int onHeaderValueCb(http_parser *parser, const char *at, size_t length)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    if (!client->m_hasValue)
        client->m_hasValue = true;

    client->m_hValue.append(at, length);

    return 0;
}

int onHeadersCompleteCb(http_parser *parser)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    if (client->m_hasField && client->m_hasValue)
    {
        client->m_requestHeaders[client->m_hField.toLower()] = client->m_hValue;
        client->m_hasField = false;
        client->m_hasValue = false;
        client->m_hField.clear();
        client->m_hValue.clear();
    }
    return 0;
}

int onBodyCb(http_parser *parser, const char *at, size_t length)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    client->m_bodyMessage.append(at, length);


    return 0;
}

int onMessageCompleteCb(http_parser *parser)
{
    HttpClient *client = reinterpret_cast<HttpClient *>(parser->data);

    client->m_parsingDone = true;
    client->m_requestMethod = parser->method;

    return 0;
}

HttpClient::HttpClient(QTcpSocket *socket, QObject *parent) :
    QObject(parent),
    m_socket(socket)
{
    m_httpParser = (http_parser*) calloc(1, sizeof(http_parser));
    http_parser_init(m_httpParser, HTTP_REQUEST);
    m_httpParser->data = this;

    m_httpParserSettings = (http_parser_settings*) calloc(1, sizeof(http_parser_settings));
    m_httpParserSettings->on_message_begin = onMessageBeginCb;
    m_httpParserSettings->on_url = onUrlCb;
    m_httpParserSettings->on_status_complete = onStatusCompleteCb;
    m_httpParserSettings->on_header_field = onHeaderFieldCb;
    m_httpParserSettings->on_header_value = onHeaderValueCb;
    m_httpParserSettings->on_headers_complete = onHeadersCompleteCb;
    m_httpParserSettings->on_body = onBodyCb;
    m_httpParserSettings->on_message_complete = onMessageCompleteCb;

    connect(socket, &QTcpSocket::readyRead, [=]
    {
        parseRequest();

        qDebug() << "Processing HTTP: " << m_parseUrl;

        if (m_parseUrl == "/")
            m_parseUrl = "/index.html";

        QHash<QString, QString> headers;
        headers["Connection"] = "Close";

        QFile fp(QString(":/debug/dist%1").arg(QString(m_parseUrl)));

        if (fp.exists() && fp.open(QIODevice::ReadOnly))
        {
            QString extension = QFileInfo(fp.fileName()).suffix().toLower();

            if (extension == "js")
                headers["Content-Type"] = "applicatiom/javascript";
            else if (extension == "html")
                headers["Content-Type"] = "text/html; charset=utf-8";
            else if (extension == "css")
                headers["Content-Type"] = "text/css; charset=utf-8";

            if (m_socket->write(buildHttpResponse(HTTP_200, headers, fp.readAll())) == -1)
                qCritical() << "HttpClient: writing error";
            fp.close();
        }
        else
        {
            headers["Content-Type"] = "text/html; charset=utf-8";
            if (m_socket->write(buildHttpResponse(HTTP_404, headers, HTTP_404_BODY)) == -1)
                qCritical() << "HttpClient: writing error";
        }

        socket->flush();
        CloseConnection();
    });
}

HttpClient::~HttpClient()
{

}

void HttpClient::CloseConnection()
{
    deleteLater();
}

void HttpClient::parseRequest()
{
    while (m_socket->bytesAvailable())
    {
        QByteArray data = m_socket->readAll();
        int n = http_parser_execute(m_httpParser, m_httpParserSettings, data.constData(), data.size());
        if (n != data.size())
        {
            // Errro close connection
            CloseConnection();
        }
    }
}

QByteArray HttpClient::buildHttpResponse(QString code, QHash<QString,QString> &headers, const QByteArray &body)
{
    QByteArray res;
    res = code.toLatin1() + "\r\n";

    if (!headers.contains("Content-Length"))
        headers["Content-Length"] = QString("%1").arg(body.length());

    foreach(QString key, headers.keys())
    {
        res +=  key + ": " + headers[key] + "\r\n";
    }
    res += "\r\n";
    res += body;
    return res;
}
