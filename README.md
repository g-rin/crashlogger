# About crashloger

Simple library for logging call stack (backtrace) of the crashed thread (at the moment it's for UNIX only).

It uses libbacktace of Ian Lancetaylor: https://github.com/ianlancetaylor/libbacktrace

Initialy the source (logicaly) was competely copied from the: https://github.com/kmikolaj/backtrace-test.git

# Build (with Qt 4 or 5)

crashlogger.pro will compile build/<debug or release>/libcrashlogger.a static library

example/example.pro will compile build/<debug or release>/example application.
  
# Example

The example will produce the following output:

> QThread(0x16f2550, name = "emiter thread") eworker dowork:  "QtnBjsMqXD"<br/>
> QThread(0x15ba7a0, name = "gui thread") emiter handleresults:  "QtnBjsMqXD"<br/>
> QThread(0x176bb20, name = "runner thread #1") rworker process "QtnBjsMqXD"<br/>
>
> \*** process /home/grin/workspace/crashloger/build/release/example (3005) crashed ***
>
>  \> why: uncaught signal 11 (Segmentation fault)
>
>  \> backtrace:<br/>
>    0: RWorker::process(QString const&) [403ab7] in ../example/runner.cpp:41<br/>
>    1: QObject::event(QEvent*) [7f17b5ec9210]<br/>
>    2: QApplicationPrivate::notify_helper(QObject*, QEvent*) [7f17b649edcb]<br/>
>    3: QApplication::notify(QObject*, QEvent*) [7f17b64a6235]<br/>
>    4: QCoreApplication::notifyInternal2(QObject*, QEvent*) [7f17b5e9f457]<br/>
>    5: QCoreApplicationPrivate::sendPostedEvents(QObject*, int, QThreadData*) [7f17b5ea1a4a]<br/>
>    6: (null) [7f17b5eef8b2]<br/>
>    7: g_main_context_dispatch [7f17b15a4196]<br/>
>    8: (null) [7f17b15a43ef]<br/>
>    9: g_main_context_iteration [7f17b15a449b]<br/>
>   10: QEventDispatcherGlib::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) [7f17b5eefcda]<br/>
>   11: QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) [7f17b5e9d9c9]<br/>
>   12: QThread::exec() [7f17b5cd2bdb]<br/>
>   13: (null) [7f17b5cd7548]<br/>
>   14: start_thread [7f17b5a166b9]<br/>
>   14: clone [7f17b51b441c]<br/>
>   16: (null) [ffffffffffffffff]<br/>
> <br/>
>  \> the process will be aborted (core dump)
> 
> Aborted (core dumped)

As we can see the cause of the crash is on RWoker::process() function.
