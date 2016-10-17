/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Calaos is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Calaos is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#include "SshAgent.h"

SshAgent::SshAgent()
{
}

QByteArray SshAgent::processRequest(const quint8 *request, quint32 msgSize)
{
    if (msgSize <= 0)
        return failedAnswer();

    quint8 cmd = request[0];

    switch (cmd)
    {
    case SSH2_AGENTC_REQUEST_IDENTITIES:
        return requestSsh2Identities(request, msgSize);
        break;
    case SSH2_AGENTC_SIGN_REQUEST:
        return signSsh2Request(request, msgSize);
        break;
    case SSH_AGENTC_LOCK:
        return agentLock(request, msgSize);
        break;
    case SSH_AGENTC_UNLOCK:
        return agentUnlock(request, msgSize);
        break;
    default: break;
    }

    return failedAnswer();
}

QByteArray SshAgent::failedAnswer()
{
    QByteArray d;
    d.resize(4);
    qToBigEndian((quint32)1, d.data());
    d.append(SSH_AGENT_FAILURE);
    return d;
}

QByteArray SshAgent::requestSsh2Identities(const quint8 *request, quint32 msgSize)
{
    Q_UNUSED(request);
    Q_UNUSED(msgSize);

    quint32 numkeys = 0;

    QByteArray d;
    d.resize(4); //size
    d.append(SSH2_AGENT_IDENTITIES_ANSWER);

    d.append(4, 0); //number of keys
    qToBigEndian(numkeys, d.data() + 5);

    //set size
    qToBigEndian((quint32)d.size() - 4, d.data());

    return d;
}

QByteArray SshAgent::signSsh2Request(const quint8 *request, quint32 msgSize)
{
    return failedAnswer();
}

QByteArray SshAgent::agentLock(const quint8 *request, quint32 msgSize)
{
    return failedAnswer();
}

QByteArray SshAgent::agentUnlock(const quint8 *request, quint32 msgSize)
{
    return failedAnswer();
}
