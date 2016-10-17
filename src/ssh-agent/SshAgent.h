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
#ifndef SSHAGENT_H
#define SSHAGENT_H

#include <QtCore>

/* Implement an ssh-agent
 * Actually only supports SSH2 keys, no SSH protocol 1 keys
 * All command to add/delete keys are not supported from the
 * agent protocol. It must be done within moolticute App.
 * Lock/Unlock command (aka ssh-add -x/-X) are supported but without
 * password. It just ask moolticuted to get all keys from the device
 * or to clear them from memory.
 * The request of identities is supported and only for SSHv2
 */

//agent response
#define SSH_AGENT_FAILURE                   5
#define SSH_AGENT_SUCCESS                   6
#define SSH2_AGENT_IDENTITIES_ANSWER		12
#define SSH2_AGENT_SIGN_RESPONSE			14

//client queries
#define SSH2_AGENTC_REQUEST_IDENTITIES  	11
#define SSH2_AGENTC_SIGN_REQUEST			13
#define SSH_AGENTC_LOCK                     22
#define SSH_AGENTC_UNLOCK                   23

class SshAgent
{
public:
    SshAgent();

    QByteArray processRequest(const quint8 *request, quint32 msgSize);

private:
    QByteArray failedAnswer();

    QByteArray requestSsh2Identities(const quint8 *request, quint32 msgSize);
    QByteArray signSsh2Request(const quint8 *request, quint32 msgSize);
    QByteArray agentLock(const quint8 *request, quint32 msgSize);
    QByteArray agentUnlock(const quint8 *request, quint32 msgSize);
};

#endif // SSHAGENT_H
