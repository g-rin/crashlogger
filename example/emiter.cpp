#include "emiter.h"
#include "debug.h"
#include <QTimer>
#include <QTime>

Emiter::Emiter(QObject *parent) : QObject(parent)
{
    objectThread.setObjectName("emiter thread");

    EWorker *worker = new EWorker;
    worker->moveToThread(&objectThread);

    connect(&objectThread, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(this, SIGNAL(operate(QString)), worker, SLOT(doWork(QString)));
    connect(worker, SIGNAL(resultReady(QString)), this, SLOT(handleResults(QString)));

    qsrand(uint(QTime::currentTime().msec()));

    objectThread.start();
}

Emiter::~Emiter()
{
    DEBUG("destroy emiter");
    objectThread.quit();
    objectThread.wait();
}

void Emiter::run()
{
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer->start(3000);
}

void Emiter::onTimeout()
{
    emit operate(randomString());
}

void Emiter::handleResults(const QString &result)
{
    DEBUG("emiter handleresults: " << result);
    emit pass(result);
}

QString Emiter::randomString() const
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    const int randomStringLength = 10;

    QString str;
    for (int i = 0; i < randomStringLength; ++i)
    {
        int index = qrand() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        str.append(nextChar);
    }
    return str;
}

EWorker::~EWorker()
{
    DEBUG("destroy eworker");
}

void EWorker::doWork(const QString &parameter)
{
    DEBUG("eworker dowork: " << parameter);
    emit resultReady(parameter);
}
