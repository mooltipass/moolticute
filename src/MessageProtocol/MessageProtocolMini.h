#ifndef MESSAGEPROTOCOLMINI_H
#define MESSAGEPROTOCOLMINI_H

#include "IMessageProtocol.h"

class MessageProtocolMini : public IMessageProtocol
{
public:
    MessageProtocolMini();

    // IMessageProtocol interface
    virtual QVector<QByteArray> createPackets(const QByteArray &data, MPCmd::Command c) override;
    virtual Common::MPStatus getStatus(const QByteArray &data) override;
    virtual quint16 getMessageSize(const QByteArray &data) override;
    virtual MPCmd::Command getCommand(const QByteArray &data) override;

    virtual quint8 getFirstPayloadByte(const QByteArray &data) override;
    virtual quint8 getPayloadByteAt(const QByteArray &data, int at) override;
    virtual QByteArray getFullPayload(const QByteArray &data) override;
    virtual QByteArray getPayloadBytes(const QByteArray &data, int fromPayload, int to) override;

    virtual quint32 getSerialNumber(const QByteArray &data) override;
    virtual bool getChangeNumber(const QByteArray &data, quint32& credDbNum, quint32& dataDbNum) override;
    virtual bool isCPZInvalid(const QByteArray &data) override;

    virtual QVector<QByteArray> createWriteNodePackets(const QByteArray& data, const QByteArray& address) override;
    //This default func only checks if return value from device is ok or not
    virtual AsyncFuncDone getDefaultFuncDone() override;
    virtual QString getDeviceName() const override;

    virtual QByteArray toByteArray(const QString &input) override;
    virtual QString toQString(const QByteArray &data) override;

    QByteArray convertDate(const QDateTime& dateTime) override;

    virtual MPNode* createMPNode(const QByteArray &d, QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0) override;
    virtual MPNode* createMPNode(QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0) override;
    MPNode* createMPNode(QByteArray &&d, QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0) override;
    MPNode* createMPNode(QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0) override;

    int getParentNodeSize() const override { return MP_NODE_SIZE; }
    int getChildNodeSize() const override { return MP_NODE_SIZE; }

    virtual void fillCommandMapping() override;
};

#endif // MESSAGEPROTOCOLMINI_H
