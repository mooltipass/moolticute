#ifndef IMESSAGEPROTOCOL_H
#define IMESSAGEPROTOCOL_H

#include "../MooltipassCmds.h"
#include "../Common.h"
#include "../AsyncJobs.h"

#include <QByteArray>

class MPNode;
class IMessageProtocol
{
public:
    IMessageProtocol() = default;
    virtual ~IMessageProtocol() = default;

    //Message information related function
    /*!
     * \brief createPackets
     * \param data
     * \param c
     * Creates the packets according to the specialized protocol.
     * \return a QByteArray vector of the created packets from the pure \a data and \a c Command
     */
    virtual QVector<QByteArray> createPackets(const QByteArray &data, MPCmd::Command c) = 0;
    virtual Common::MPStatus getStatus(const QByteArray &data) = 0;
    virtual quint16 getMessageSize(const QByteArray &data) = 0;
    virtual MPCmd::Command getCommand(const QByteArray &data) = 0;

    // Payload related functions
    /*!
     * \brief getFirstPayloadByte
     * \param data
     * \return first byte of the \a data payload as quint8
     */
    virtual quint8 getFirstPayloadByte(const QByteArray &data) = 0;
    /*!
     * \brief getFullPayload
     * \param data
     * \return the entire packet payload as QByteArray from \a data
     */
    virtual QByteArray getFullPayload(const QByteArray &data) = 0;
    /*!
     * \brief getPayloadByteAt
     * \param data
     * \param at
     * \return A byte as quint8 \a at place of \a data payload
     */
    virtual quint8 getPayloadByteAt(const QByteArray &data, int at) = 0;
    /*!
     * \brief getPayloadBytes
     * \param data
     * \param fromPayload
     * \param to
     * \return A QByteArray of the following range bytes: \a fromPayload byte until \a to byte.
     */
    virtual QByteArray getPayloadBytes(const QByteArray &data, int fromPayload, int to) = 0;


    //Command related functions
    virtual quint32 getSerialNumber(const QByteArray &data) = 0;
    virtual bool getChangeNumber(const QByteArray &data, quint32& credDbNum, quint32& dataDbNum) = 0;
    virtual bool isCPZInvalid(const QByteArray &data) = 0;

    /*!
     * \brief createWriteNodePackets
     * \param data
     * \param address
     * Protocol specified version of WRITE_FLASH_NODE command's data creation
     * \return created packets from MPNode's \a data and \a address
     */
    virtual QVector<QByteArray> createWriteNodePackets(const QByteArray& data, const QByteArray& address) = 0;


    //This default func only checks if return value from device is ok or not
    virtual AsyncFuncDone getDefaultFuncDone() = 0;

    /**
     * @brief getDeviceName
     * @return the used device name (BLE or Mini)
     */
    virtual QString getDeviceName() const = 0;

    /**
     * @brief isBLE
     * @return true if the current device is a BLE
     */
    bool isBLE() const
    {
        return "BLE" == getDeviceName();
    }

    /**
     * @brief fillCommandMapping
     * Fill commandMap according to the given device
     */
    virtual void fillCommandMapping() = 0;

    virtual QByteArray toByteArray(const QString &input) = 0;
    virtual QString toQString(const QByteArray &data) = 0;

    /**
     * @brief getDeviceMappedCommandId
     * @param cmd: general command id
     * @return mapped commandId according to the given device
     */
    quint16 getDeviceMappedCommandId(const MPCmd::Command &cmd)
    {
        const auto commandIter = m_commandMapping.find(cmd);
        if (commandIter == m_commandMapping.end())
        {
            qCritical() << MPCmd::printCmd(cmd) << " is not implemented for " << getDeviceName();
            return m_commandMapping[MPCmd::PING];
        }
        return commandIter.value();
    }

    /**
     * @brief getGeneralCommandId
     * @param cmd: mapped commandId according to the given device
     * @return cmd: general command id
     */
    MPCmd::Command getGeneralCommandId(const quint16 cmd)
    {
        return MPCmd::Command(m_commandMapping.key(cmd, MPCmd::PING));
    }

    QString printCmd(const MPCmd::Command &cmd)
    {
        const auto commandId = m_commandMapping[cmd];
        QMetaEnum m = QMetaEnum::fromType<MPCmd::Command>();
        return QString("%1 (%2)")
                .arg(m.valueToKey(cmd))
                .arg(MPCmd::toHexString(commandId));
    }

    QString printCmd(const QByteArray &data)
    {
        MPCmd::Command cmd = getCommand(data);
        return printCmd(cmd);
    }

    quint16 toIntFromLittleEndian(quint8 lowerByte, quint8 upperByte)
    {
        return (lowerByte|(upperByte<<8));
    }

    QByteArray toLittleEndianFromInt(quint16 num)
    {
        QByteArray littleEndian;
        littleEndian.append(static_cast<char>(num&0xFF));
        littleEndian.append(static_cast<char>((num&0xFF00)>>8));
        return littleEndian;
    }

    QByteArray toLittleEndianFromInt32(quint32 num)
    {
        QByteArray littleEndian;
        littleEndian.append(static_cast<char>(num&0x0FF));
        littleEndian.append(static_cast<char>((num&0x0FF00)>>8));
        littleEndian.append(static_cast<char>((num&0x0FF0000)>>16));
        littleEndian.append(static_cast<char>((num&0x0FF000000)>>24));
        return littleEndian;
    }

    /**
     * @brief convertDate
     * @param dateTime: current dateTime
     * @return dateTime converted to QByteArray
     * for the given device
     */
    virtual QByteArray convertDate(const QDateTime& dateTime) = 0;

    /**
     * @brief createMPNode
     * @return MPNodeBLE or MPNodeMini
     * Factory method for MPNode return a new
     * MPNode for the corresponding device
     */
    virtual MPNode* createMPNode(const QByteArray &d, QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0) = 0;
    virtual MPNode* createMPNode(QObject *parent = nullptr, const QByteArray &nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0) = 0;
    virtual MPNode* createMPNode(QByteArray &&d, QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0) = 0;
    virtual MPNode* createMPNode(QObject *parent = nullptr, QByteArray &&nodeAddress = QByteArray(2, 0), const quint32 virt_addr = 0) = 0;

    virtual int getParentNodeSize() const = 0;
    virtual int getChildNodeSize() const = 0;
    virtual uint getMaxFavorite() const = 0;
    virtual uint getDataNodeEncSize() const = 0;

    virtual int getCredentialPackageSize() const = 0;

    virtual int getLoginMaxLength() const = 0;
    virtual int getPwdMaxLength() const = 0;

    virtual QByteArray getCpzValue(const QByteArray &cpzCtr) const = 0;


    QMap<quint16,quint16> m_commandMapping;

    static constexpr int CPZ_LENGTH = 8;
};


#endif // IMESSAGEPROTOCOL_H
