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
#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QTcpSocket>

#include "http-parser/http_parser.h"

#define HTTP_400 "HTTP/1.0 400 Bad Request"
#define HTTP_404 "HTTP/1.0 404 Not Found"
#define HTTP_301 "HTTP/1.1 301 Moved Permanently"
#define HTTP_500 "HTTP/1.0 500 Internal Server Error"
#define HTTP_200 "HTTP/1.0 200 OK"

#define HTTP_400_BODY "<html><head>" \
    "<title>400 Bad Request</title>" \
    "</head>" \
    "<body>" \
    "<h1>Moolticute Server - Bad Request</h1>" \
    "<p>The server received a request it could not understand.</p>" \
    "</body>" \
    "</html>"

#define HTTP_404_BODY "<html><head>" \
    "<title>404 Not Found</title>" \
    "</head>" \
    "<body>" \
    "<h1>Moolticute Server - Page not found</h1>" \
    "<p>Document or file requested by the client was not found.</p>" \
    "</body>" \
    "</html>"

#define HTTP_500_BODY "<html><head>" \
    "<title>500 Internal Server Error</title>" \
    "</head>" \
    "<body>" \
    "<h1>Moolticute Server - Internal Server Error</h1>" \
    "<p>The server encountered an unexpected condition which prevented it from fulfilling the request.</p>" \
    "</body>" \
    "</html>"


class HttpClient : public QObject
{
    Q_OBJECT
public:
    explicit HttpClient(QTcpSocket *socket, QObject *parent);
    ~HttpClient();

private:
    QTcpSocket *m_socket;
    http_parser *m_httpParser;
    http_parser_settings *m_httpParserSettings;
    QByteArray m_parseUrl;
    QByteArray m_bodyMessage;
    bool m_hasField;
    bool m_hasValue;
    QByteArray m_hField;
    QByteArray  m_hValue;
    bool m_parsingDone;
    unsigned char m_requestMethod;
    QHash<QString, QString> m_requestHeaders;

    void parseRequest();
    void CloseConnection();
    QByteArray buildHttpResponse(QString code, QHash<QString, QString> &headers, const QByteArray &body);

    friend int onMessageBeginCb(http_parser *parser);
    friend int onUrlCb(http_parser *parser, const char *at, size_t length);
    friend int onStatusCompleteCb(http_parser *parser);
    friend int onHeaderFieldCb(http_parser *parser, const char *at, size_t length);
    friend int onHeaderValueCb(http_parser *parser, const char *at, size_t length);
    friend int onHeadersCompleteCb(http_parser *parser);
    friend int onBodyCb(http_parser *parser, const char *at, size_t length);
    friend int onMessageCompleteCb(http_parser *parser);

signals:

public slots:
};

#endif // HTTPCLIENT_H
