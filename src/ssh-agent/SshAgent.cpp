#include "SshAgent.h"

SshAgent::SshAgent()
{
}

QByteArray SshAgent::processRequest(const void *request)
{
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
