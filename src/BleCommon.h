#ifndef BLECOMMON_H
#define BLECOMMON_H

#include <QString>
#include <QMap>

static constexpr quint16 MSG_FAILED = 0x00;
static constexpr quint16 MSG_SUCCESS = 0x01;

static constexpr quint16 BT_REPORT_ID = 0x04;


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

    BleCredential(QString service, QString login = "", QString desc = "", QString third = "", QString pwd = "")
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

using CredMap = QMap<BleCredential::CredAttr, QString>;

#endif // BLECOMMON_H
