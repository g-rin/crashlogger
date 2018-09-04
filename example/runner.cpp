#include "runner.h"
#include "debug.h"

quint8 Runner::count = 0;

Runner::Runner(QObject *parent) : QObject(parent)
{
    objectThread.setObjectName(QString("runner thread #") + QString::number(++count));

    RWorker *worker = new RWorker;
    worker->moveToThread(&objectThread);

    connect(&objectThread, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(this, SIGNAL(execute(QString)), worker, SLOT(process(QString)));
    connect(worker, SIGNAL(ready()), this, SLOT(onReady()));

    objectThread.start();
}

Runner::~Runner()
{
    DEBUG("destroy runner");
    objectThread.quit();
    objectThread.wait();
}

void Runner::onReady()
{
    DEBUG("runner onReady");
}

RWorker::~RWorker()
{
    DEBUG("destroy rworker");
}

void RWorker::process(const QString &data)
{
    DEBUG("rworker process" << data);
    // SEGFAULT
    *((int *)1) = 1;
    emit ready();
}
