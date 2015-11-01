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
#include "AsyncJobs.h"
#include "MPDevice.h"

void MPCommandJob::start(const QByteArray &previous_data)
{
    if (!beforeFunc(previous_data))
    {
        emit error();
        return;
    }

    device->sendData(cmd, data, [=](bool success, const QByteArray &resdata)
    {
        if (!success)
            emit error();
        else
        {
            if (!afterFunc(resdata))
                emit error();
            else
                emit done(resdata);
        }
    });
}

AsyncJobs::AsyncJobs(QObject *parent):
    QObject(parent)
{
}

AsyncJobs::~AsyncJobs()
{
    qDebug() << "~AsyncJobs()";
}

void AsyncJobs::append(AsyncJob *j)
{
    //reparent object to handle jobs memory from AsyncJobs
    j->setParent(this);
    jobs.enqueue(j);
}

void AsyncJobs::start()
{
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
    emit failed(currentJob);
    deleteLater();
}

void AsyncJobs::jobDone(const QByteArray &data)
{
    dequeueStartJob(data);
}
