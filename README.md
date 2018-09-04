# About crashloger

Simple library for logging call stack (backtrace) of the crashed thread (at the moment it's for UNIX only).

It uses libbacktace of Ian Lancetaylor: https://github.com/ianlancetaylor/libbacktrace

Initialy the source (logicaly) was competely copied from the: https://github.com/kmikolaj/backtrace-test.git

# Build (with Qt 4 or 5)

crashloger.pro will compile build/<debug or release>/libcrashloger.a static library

example/example.pro will compile build/<debug or release>/example application.
  
# Example

The example will produce the following output:

> QThread(0x16f2550, name = "emiter thread") eworker dowork:  "QtnBjsMqXD"
> QThread(0x15ba7a0, name = "gui thread") emiter handleresults:  "QtnBjsMqXD"
> QThread(0x176bb20, name = "runner thread #1") rworker process "QtnBjsMqXD"
> 
> \*** process /home/grin/workspace/crashloger/build/release/example (3005) crashed ***
> 
>  \> why: uncaught signal 11 (Segmentation fault)
> 
>  \> backtrace:
>    0: crashLoger [404844] in ../crashloger/src/crashloger.cpp:249
>    1: operator() [404a57] in ../crashloger/src/crashloger.cpp:47
>    1: _FUN [404a57] in ../crashloger/src/crashloger.cpp:48
>    2: (null) [7f17b50e24af]
>    3: RWorker::process(QString const&) [403ab7] in ../example/runner.cpp:41
>    4: QObject::event(QEvent*) [7f17b5ec9210]
>    5: QApplicationPrivate::notify_helper(QObject*, QEvent*) [7f17b649edcb]
>    6: QApplication::notify(QObject*, QEvent*) [7f17b64a6235]
>    7: QCoreApplication::notifyInternal2(QObject*, QEvent*) [7f17b5e9f457]
>    8: QCoreApplicationPrivate::sendPostedEvents(QObject*, int, QThreadData*) [7f17b5ea1a4a]
>    9: (null) [7f17b5eef8b2]
>   10: g_main_context_dispatch [7f17b15a4196]
>   11: (null) [7f17b15a43ef]
>   12: g_main_context_iteration [7f17b15a449b]
>   13: QEventDispatcherGlib::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) [7f17b5eefcda]
>   14: QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) [7f17b5e9d9c9]
>   15: QThread::exec() [7f17b5cd2bdb]
>   16: (null) [7f17b5cd7548]
>   17: start_thread [7f17b5a166b9]
>   18: clone [7f17b51b441c]
>   19: (null) [ffffffffffffffff]
> 
>  \> the process will be aborted (core dump)
> 
> Aborted (core dumped)

As we can see the first four lines of the backtrace refer to crashloger's code.
So we skip them. The cause of the crash is on RWoker::process() function (the line started with '3:').
