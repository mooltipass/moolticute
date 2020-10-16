#ifndef LOGINITEM_H
#define LOGINITEM_H

// Qt
#include <QDate>
#include <QJsonObject>

// Application
#include "TreeItem.h"
class ServiceItem;

class LoginItem : public TreeItem
{
public:
    LoginItem(const QString &sLoginName);
    virtual ~LoginItem();
    const QByteArray &address() const;
    void setAddress(const QByteArray &bAddress);
    qint8 favorite() const;
    void setFavorite(qint8 iFavorite);
    const QString &password() const;
    void setPassword(const QString &sPassword);
    void setPasswordOrig(const QString &setPassword);
    QJsonObject toJson() const;
    void setPasswordLocked(bool bVisible);
    bool passwordLocked() const;
    bool hasBlankPwdChanged() const;
    virtual TreeType treeType()  const Q_DECL_OVERRIDE;
    void setTOTPCredential(QString secretKey, int timeStep, int codeSize);
private:    
    QByteArray m_bAddress;
    qint8 m_iFavorite;
    QString m_sPassword;
    QString m_sPasswordOrig;
    bool m_bPasswordLocked;

    struct TOTPCredential
    {
        QString secretKey;
        int timeStep;
        int codeSize;
        bool valid = false;

        TOTPCredential(QString secretKey, int timeStep, int codeSize):
            secretKey{secretKey},
            timeStep{timeStep},
            codeSize{codeSize},
            valid{true}
        {}

        TOTPCredential() = default;

    };

    TOTPCredential m_totpCred;
};

#endif // LOGINITEM_H
