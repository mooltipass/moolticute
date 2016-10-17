#ifndef SSHAGENT_H
#define SSHAGENT_H

#include <QtCore>

#define SSH_AGENT_FAILURE           5
#define SSH_AGENT_SUCCESS           6

class SshAgent
{
public:
    SshAgent();

    QByteArray processRequest(const void *request);

private:
    QByteArray failedAnswer();
};

#endif // SSHAGENT_H
