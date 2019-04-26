#ifndef MPNODEBLEIMPL_H
#define MPNODEBLEIMPL_H

#include "MPNodeImpl.h"

class MPNodeBLEImpl : public MPNodeImpl
{
public:
    MPNodeBLEImpl(const QByteArray &d);
    bool isValid() override;
    bool isDataLengthValid() override;

    QByteArray getService() const override;
    void setService(QByteArray service) override;
    QByteArray getStartDataCtr() const override;
    QByteArray getCTR() const override;

    QByteArray getDescription() const override;
    void setDescription(const QByteArray& desc) override;
    QByteArray getLogin() const override;
    void setLogin(const QByteArray& login) override;
    QByteArray getPasswordEnc() const override;
    QByteArray getDateCreated() const override;
    QByteArray getDateLastUsed() const override;

private:

    static constexpr int PARENT_NODE_LENGTH = 264;
    static constexpr int CHILD_NODE_LENGTH = 528;
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

#endif // MPNODEBLEIMPL_H
