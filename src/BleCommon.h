#ifndef BLECOMMON_H
#define BLECOMMON_H

#include <QString>
#include <QMap>

class BleCredential
{
public:
    enum class CredAttr
    {
        SERVICE,
        LOGIN,
        DESCRIPTION,
        THIRD,
        PASSWORD
    };

    BleCredential(QString service, QString login = "", QString pwd = "", QString desc = "", QString third = "")
    {
        m_attributes = { {CredAttr::SERVICE, service},
                         {CredAttr::LOGIN, login},
                         {CredAttr::DESCRIPTION, desc},
                         {CredAttr::THIRD, third},
                         {CredAttr::PASSWORD, pwd}
                       };
    }

    QMap<CredAttr, QString> getAttributes() const { return m_attributes; }
    void set(CredAttr field, QString value) { m_attributes[field] = value; }
    QString get(CredAttr field) const { return m_attributes[field]; }

private:
    QMap<CredAttr, QString> m_attributes;
};

#endif // BLECOMMON_H
