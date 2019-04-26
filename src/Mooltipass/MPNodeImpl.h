#ifndef MPNODEIMPL_H
#define MPNODEIMPL_H

#include <QByteArray>

class MPNodeImpl
{
public:
    MPNodeImpl(const QByteArray &d);
    virtual bool isValid() = 0;
    virtual bool isDataLengthValid() = 0;
    int getType() const;
    void setType(quint8 type);
    QByteArray getData() const;
    void appendData(const QByteArray &d);
    QByteArray getNodeFlags();

    QByteArray getPreviousAddress() const;
    void setPreviousAddress(const QByteArray &d);
    QByteArray getNextAddress() const;
    void setNextAddress(const QByteArray &d);
    QByteArray getStartAddress() const;
    void setStartAddress(const QByteArray &d);

    QByteArray getNextChildDataAddress() const;
    void setNextChildDataAddress(const QByteArray &d);
    QByteArray getNextDataAddress() const;

    void setLoginNodeData(const QByteArray &flags, const QByteArray &d);
    QByteArray getLoginNodeData() const;
    void setLoginChildNodeData(const QByteArray &flags, const QByteArray &d);
    QByteArray getLoginChildNodeData() const;
    void setDataNodeData(const QByteArray &flags, const QByteArray &d);
    QByteArray getDataNodeData() const;
    void setDataChildNodeData(const QByteArray &flags, const QByteArray &d);
    QByteArray getDataChildNodeData() const;


    virtual QByteArray getService() const = 0;
    virtual void setService(QByteArray) = 0;

    virtual QByteArray getStartDataCtr() const = 0;
    virtual QByteArray getCTR() const = 0;
    virtual QByteArray getDescription() const = 0;
    virtual void setDescription(const QByteArray& desc) = 0;
    virtual QByteArray getLogin() const = 0;
    virtual void setLogin(const QByteArray& login) = 0;
    virtual QByteArray getPasswordEnc() const = 0;
    virtual QByteArray getDateCreated() const = 0;
    virtual QByteArray getDateLastUsed() const = 0;

protected:
    QByteArray data;

    static constexpr int ADDRESS_LENGTH = 2;
    static constexpr int CTR_DATA_LENGTH = 2;
    static constexpr int NODE_FLAG_ADDR_START = 0;

    static constexpr int PREVIOUS_PARENT_ADDR_START = 2;
    static constexpr int NEXT_PARENT_ADDR_START = 4;
    static constexpr int START_CHILD_ADDR_START = 6;

    static constexpr int NEXT_DATA_ADDR_START = 2;
    static constexpr int DATA_ADDR_START = 8;
    static constexpr int LOGIN_CHILD_NODE_DATA_ADDR_START = 6;
    static constexpr int DATA_CHILD_DATA_ADDR_START = 4;

    static constexpr int SERVICE_ADDR_START = 8;
};

#endif // MPNODEIMPL_H
