#ifndef RUNNER_H
#define RUNNER_H

#include <QObject>
#include <QThread>

class RWorker : public QObject
{
    Q_OBJECT

    // do not need
    ~RWorker();

public slots:
    void process(const QString &data);

signals:
    void ready();
};

class Runner : public QObject
{
    Q_OBJECT
    QThread objectThread;
    static quint8 count;

public:
    explicit Runner(QObject *parent = 0);
    ~Runner();

signals:
    void execute(const QString &data);

private slots:
    void onReady();
};

#endif  // RUNNER_H
