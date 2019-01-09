#ifndef IMESSAGEPROTOCOL_H
#define IMESSAGEPROTOCOL_H

#include "../MooltipassCmds.h"
#include "../Common.h"
#include "../AsyncJobs.h"

#include <QByteArray>

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
    virtual quint8 getMessageSize(const QByteArray &data) = 0;
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
    virtual quint32 getSerialNumber(const QByteArray &) = 0;
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
};


#endif // IMESSAGEPROTOCOL_H
