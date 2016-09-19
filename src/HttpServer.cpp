#include <QSettings>
#include <QCoreApplication>
#include <QStandardPaths>
#include "HttpServer.h"


HttpServer::HttpServer(QObject *parent) :
    QObject(parent),
    m_tcpServer(0)
{

}

HttpServer::~HttpServer()
{

}

bool HttpServer::start(quint16 port)
{
    bool ret = false;
    m_tcpServer = new QTcpServer(this);

    ret = m_tcpServer->listen(QHostAddress(QHostAddress::AnyIPv4), port);
    if (!ret)
    {
        delete m_tcpServer;
        return ret;
    }

    qDebug() << "Http server listening on port " << port;
    connect(m_tcpServer, &QTcpServer::newConnection, [=]()
    {
        while (m_tcpServer->hasPendingConnections())
        {
            QTcpSocket *socket = m_tcpServer->nextPendingConnection();
            HttpClient *client = new HttpClient(socket, m_tcpServer);
            m_clients << client;
        }
    });

    return ret;

}

