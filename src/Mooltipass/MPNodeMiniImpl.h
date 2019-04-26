#ifndef MPNODEMINIIMPL_H
#define MPNODEMINIIMPL_H

#include "MPNodeImpl.h"

class MPNodeMiniImpl : public MPNodeImpl
{
public:
    MPNodeMiniImpl(const QByteArray &d);
    bool isValid() override;
    bool isDataLengthValid() override;

    QByteArray getService() const override;
    void setService(QByteArray) override;
    QByteArray getStartDataCtr() const override;
    QByteArray getCTR() const override;
    QByteArray getDescription() const override;
    void setDescription(const QByteArray& desc) override;
    QByteArray getLogin() const override;
    void setLogin(const QByteArray& login) override;
    QByteArray getPasswordEnc() const override;
    QByteArray getDateCreated() const override;
    QByteArray getDateLastUsed() const override;

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
};

#endif // MPNODEMINIIMPL_H
