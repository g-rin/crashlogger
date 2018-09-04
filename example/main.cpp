#include "emiter.h"
#include "runner.h"
#include <crashloger.h>
#include <QApplication>
#include <QMainWindow>

int main( int argc, char**argv ) {
    initCrashloger();

    QApplication app(argc, argv);

    QMainWindow gui;

    gui.thread()->setObjectName("gui thread");
    auto*const emiter = new Emiter( &gui );
    auto*const runner = new Runner( &gui );
    gui.connect(emiter, SIGNAL(pass(QString)), runner, SIGNAL(execute(QString)));
    emiter->run();

    gui.show();

    return app.exec();
}
