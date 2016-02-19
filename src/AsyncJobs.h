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
#ifndef ASYNCJOBS_H
#define ASYNCJOBS_H

#include <QObject>
#include <QVariant>
#include <QByteArray>
#include <QQueue>
#include <functional>

/*
 * Classes for running jobs (aka a command sent to the device and a result coming from it)
 * It handles chaining the jobs to start the next one after one has completed successfuly.
 * It also handles failure: when a job fails, the job queue is stopped.
 *
 * The AsyncJobs manages the job queue and it autodeletes itself when finished for convenience.
 *
 * In a MPCommandJob you can have two callbacks for controlling the running queue, beforeFunc
 * is called in a job before starting it and with passing the data from the job called before it.
 * afterFunc is called after the data has been received from device and before starting the next job.
 * In those callbacks, use can return false to stop the running queue, false otherwise.
 *
 * AsyncJobs queue support adding more jobs to the queue dynamically (even from the callback from
 * one running job). This is usefull to add jobs to the queue that are different based on the result
 * of the data received from the device.
 */

typedef std::function<bool(const QByteArray &data)> AsyncFunc;
typedef std::function<bool(const QByteArray &data, bool &done)> AsyncFuncDone;

class AsyncJob: public QObject
{
    Q_OBJECT
public:
    AsyncJob(QObject *parent = nullptr):
        QObject(parent)
    {}

public slots:
    virtual void start(const QByteArray &data) = 0;

signals:
    void done(const QByteArray &data);
    void error();
};

class MPDevice;

class MPCommandJob: public AsyncJob
{
    Q_OBJECT
public:
    MPCommandJob(MPDevice *dev, quint8 c, const QByteArray &d,
                 AsyncFunc beforefn,
                 AsyncFuncDone afterfn):
        AsyncJob(),
        device(dev),
        cmd(c),
        data(d),
        beforeFunc(std::move(beforefn)),
        afterFunc(std::move(afterfn))
    {}
    MPCommandJob(MPDevice *dev, quint8 c, const QByteArray &d = QByteArray(),
                 AsyncFuncDone afterfn = [](const QByteArray &, bool &) -> bool { return true; }):
        AsyncJob(),
        device(dev),
        cmd(c),
        data(d),
        afterFunc(std::move(afterfn))
    {}
    MPCommandJob(MPDevice *dev, quint8 c,
                 AsyncFunc beforefn,
                 AsyncFuncDone afterfn):
        AsyncJob(),
        device(dev),
        cmd(c),
        beforeFunc(std::move(beforefn)),
        afterFunc(std::move(afterfn))
    {}
    MPCommandJob(MPDevice *dev, quint8 c,
                 AsyncFuncDone afterfn = [](const QByteArray &, bool &) -> bool { return true; }):
        AsyncJob(),
        device(dev),
        cmd(c),
        afterFunc(std::move(afterfn))
    {}

public slots:
    virtual void start(const QByteArray &previous_data);

private:
    MPDevice *device;
    quint8 cmd;
    QByteArray data;

    //callback with data from previous Job
    AsyncFunc beforeFunc = [](const QByteArray &) -> bool { return true; };

    //calback with data from this job before it finishes
    AsyncFuncDone afterFunc = [](const QByteArray &, bool &) -> bool { return true; };
};

class AsyncJobs: public QObject
{
    Q_OBJECT
public:
    AsyncJobs(QObject *parent = nullptr);
    virtual ~AsyncJobs();

    void append(AsyncJob *j);

    //user data attached to this job queue
    QVariant user_data;

public slots:
    void start();

signals:
    void finished(const QByteArray &data);
    void failed(AsyncJob *job);

private slots:
    void dequeueStartJob(const QByteArray &data);
    void jobDone(const QByteArray &data);
    void jobFailed();

private:
    QQueue<AsyncJob *> jobs;
    bool running = false;
    AsyncJob *currentJob = nullptr;
};

#endif // ASYNCJOBS_H
