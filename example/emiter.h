#ifndef EMITER_H
#define EMITER_H

#include <QObject>
#include <QThread>

class EWorker : public QObject
{
    Q_OBJECT

    // do not need
    ~EWorker();

public slots:
    void doWork(const QString &parameter);

signals:
    void resultReady(const QString &result);
};

class Emiter : public QObject
{
    Q_OBJECT
    QThread objectThread;

public:
    explicit Emiter(QObject *parent = nullptr);
    ~Emiter();

    void run();
private slots:
    void onTimeout();
public slots:
    void handleResults(const QString &result);
signals:
    void operate(const QString &);
    void pass(const QString &);

private:
    QString randomString() const;
};

#endif  // EMITER_H
