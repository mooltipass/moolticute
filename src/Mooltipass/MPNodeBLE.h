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

    int getCategory() const;
    void setCategory(int category);

    int getKeyAfterLogin() const;
    void setKeyAfterLogin(int key);
    int getKeyAfterPwd() const;
    void setKeyAfterPwd(int key);
    int getPwdBlankFlag() const;
    void setPwdBlankFlag();

    int getTOTPTimeStep() const;
    int getTOTPCodeSize() const;
    void resetTOTPCredential();

    QByteArray getPointedToChildAddr() const;

    static constexpr int PARENT_NODE_LENGTH = 264;
    static constexpr int CHILD_NODE_LENGTH = 528;
    static constexpr int SERVICE_LENGTH = 252;
    static constexpr int LOGIN_LENGTH = 128;

protected:
    static constexpr int CTR_DATA_ADDR_START = 261;
    static constexpr int CTR_ADDR_START = 395;
    static constexpr int DESC_ADDR_START = 140;
    static constexpr int DESC_LENGTH = 48;
    static constexpr int LOGIN_ADDR_START = 12;
    static constexpr int PWD_ENC_ADDR_START = 266;
    static constexpr int PWD_ENC_LENGTH = 128;
    static constexpr int DATE_CREATED_ADDR_START = 8;
    static constexpr int DATE_LASTUSED_ADDR_START = 10;
    static constexpr int KEY_AFTER_LOGIN_ADDR_START = 260;
    static constexpr int KEY_AFTER_PWD_ADDR_START = 262;
    static constexpr int PWD_BLANK_FLAG = 266;
    static constexpr int TOTP_ADDR_START = 400;
    static constexpr int TOTP_LENGTH = 73;
    static constexpr int TOTP_TIME_STEP = 466;
    static constexpr int TOTP_CODE_SIZE = 468;
    static constexpr int KEY_AFTER_LENGTH = 2;
    static constexpr int POINTED_TO_CHILD_START = 6;
    static constexpr char BLANK_CHAR = 0x01;
};

#endif // MPNODEBLE_H
