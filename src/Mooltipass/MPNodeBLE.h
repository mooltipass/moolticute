#ifndef MPNODEBLE_H
#define MPNODEBLE_H

#include <QObject>
#include "MPNode.h"

class MPNodeBLE : public MPNode
{
public:
    MPNodeBLE(const QByteArray &d, QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNodeBLE(QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNodeBLE(QByteArray &&d, QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);
    MPNodeBLE(QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0);

    bool isDataLengthValid() const override;
    bool isValid() const override;

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

    static constexpr int PARENT_NODE_LENGTH = 264;
    static constexpr int CHILD_NODE_LENGTH = 528;

protected:
    static constexpr int SERVICE_LENGTH = 252;
    static constexpr int CTR_DATA_ADDR_START = 261;
    static constexpr int CTR_ADDR_START = 395;
    static constexpr int DESC_ADDR_START = 140;
    static constexpr int DESC_LENGTH = 48;
    static constexpr int LOGIN_ADDR_START = 12;
    static constexpr int LOGIN_LENGTH = 128;
    static constexpr int PWD_ENC_ADDR_START = 266;
    static constexpr int PWD_ENC_LENGTH = 128;
    static constexpr int DATE_CREATED_ADDR_START = 8;
    static constexpr int DATE_LASTUSED_ADDR_START = 10;
};

#endif // MPNODEBLE_H
