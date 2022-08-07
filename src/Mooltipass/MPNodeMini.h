#ifndef MPNODEMINI_H
#define MPNODEMINI_H

#include <QObject>
#include <MPNode.h>

class MPNodeMini : public MPNode
{
public:
    MPNodeMini(const QByteArray &d, QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNodeMini(QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNodeMini(QByteArray &&d, QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNodeMini(QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);


    bool isDataLengthValid() const override;
    bool isValid() const override;

    QByteArray getLoginNodeData() const override;
    void setLoginNodeData(const QByteArray &flags, const QByteArray &d) override;
    QByteArray getLoginChildNodeData() const override;
    int getLoginChildNodeDataAddrStart() override { return LOGIN_CHILD_NODE_DATA_ADDR_START; }

    QString getService() const override;
    void setService(const QString& service) override;
    QByteArray getStartDataCtr() const override;
    QByteArray getCTR() const override;
    QString getDescription() const override;
    void setDescription(const QString& newDescription) override;
    QString getLogin() const override;
    void setLogin(const QString& newLogin) override;
    QByteArray getPasswordEnc() const override;
    QDate getDateCreated() const override;
    QDate getDateLastUsed() const override;

protected:
    static constexpr int CTR_DATA_ADDR_START = 129;
    static constexpr int CTR_ADDR_START = 34;
    static constexpr int DESC_ADDR_START = 6;
    static constexpr int DESC_LENGTH = 24;
    static constexpr int LOGIN_ADDR_START = 37;
    static constexpr int LOGIN_LENGTH = 63;
    static constexpr int PWD_ENC_ADDR_START = 100;
    static constexpr int PWD_ENC_LENGTH = 32;
    static constexpr int DATE_CREATED_ADDR_START = 30;
    static constexpr int DATE_LASTUSED_ADDR_START = 32;
    static constexpr int LOGIN_CHILD_NODE_DATA_ADDR_START = 6;
};

#endif // MPNODEMINI_H
