/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#include "AsyncJobs.h"
#include "MPDevice.h"

AsyncFuncDone MPCommandJob::defaultCheckRet = [](const QByteArray &data, bool &) -> bool
{
    return (quint8)data.at(2) == 0x01;
};

void MPCommandJob::start(const QByteArray &previous_data)
{
    if (!beforeFunc(previous_data, data))
    {
        emit error();
        return;
    }

    device->sendData((MPCmd::Command)cmd, data, timeout, [=](bool success, const QByteArray &resdata, bool &done_recv)
    {
        if (!success)
            emit error();
        else
        {
            bool ret = afterFunc(resdata, done_recv);
            if (!done_recv)
            {
                //qDebug() << "Waiting for other packet...";
                return; //all data are not received yet. keep waiting
            }
            if (!ret)
                emit error();
            else
                emit done(resdata);
        }
    },
    checkReturn);
}

AsyncJobs::AsyncJobs(QString _log, QObject *parent):
    QObject(parent),
    jobsid(Common::createUid("job-")),
    log(_log)
{
}

AsyncJobs::AsyncJobs(QString _log, QString jid, QObject *parent):
    QObject(parent),
    jobsid(jid),
    log(_log)
{
}

AsyncJobs::~AsyncJobs()
{
    Common::releaseUid(jobsid);
}

void AsyncJobs::append(AsyncJob *j)
{
    //reparent object to handle jobs memory from AsyncJobs
    j->setParent(this);
    jobs.enqueue(j);
}

void AsyncJobs::prepend(AsyncJob *j)
{
    //reparent object to handle jobs memory from AsyncJobs
    j->setParent(this);
    jobs.prepend(j);
}

void AsyncJobs::insertAfter(AsyncJob *j, int pos)
{
    //reparent object to handle jobs memory from AsyncJobs
    j->setParent(this);
    jobs.insert(pos + 1, j);
}

void AsyncJobs::start()
{
    qInfo() << log;

    if (running) return;
    running = true;

    //start running or queued jobs if any
    dequeueStartJob(QByteArray());
}

void AsyncJobs::dequeueStartJob(const QByteArray &data)
{
    if (jobs.isEmpty())
    {
        //end of job queue, emit finished signal
        //and delete job runner
        emit finished(data);
        deleteLater();
        return;
    }

    currentJob = jobs.dequeue();
    connect(currentJob, SIGNAL(done(QByteArray)), this, SLOT(jobDone(QByteArray)));
    connect(currentJob, SIGNAL(error()), this, SLOT(jobFailed()));
    currentJob->start(data);
}

void AsyncJobs::jobFailed()
{
    disconnect(currentJob, SIGNAL(done(QByteArray)), this, SLOT(jobDone(QByteArray)));
    disconnect(currentJob, SIGNAL(error()), this, SLOT(jobFailed()));
    emit failed(currentJob);
    deleteLater();
}

void AsyncJobs::jobDone(const QByteArray &data)
{
    disconnect(currentJob, SIGNAL(done(QByteArray)), this, SLOT(jobDone(QByteArray)));
    disconnect(currentJob, SIGNAL(error()), this, SLOT(jobFailed()));
    dequeueStartJob(data);
}

void AsyncJobs::setCurrentJobError(QString err)
{
    if (currentJob)
        currentJob->setErrorStr(err);
}
