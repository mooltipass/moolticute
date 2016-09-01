#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>


#include "HttpClient.h"

class HttpServer: QObject
{
    Q_OBJECT
public:
    HttpServer(QObject *parent = 0);
    virtual ~HttpServer();

    bool start(quint16 port);

private:
    QTcpServer *m_tcpServer;
    QList<HttpClient *>m_clients;
    void parseRequest(QTcpSocket *socket);
    QString m_cacheDirectory;

private slots:
};

#endif // HTTPSERVER_H
