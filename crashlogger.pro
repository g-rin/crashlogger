include($${PWD}/config.pri)
include($${PWD}/crashlogger.pri)
TEMPLATE = lib
QT = core
LIBS *= -lbacktrace
HEADERS += include/crashlogger.h
SOURCES += src/crashlogger.cpp
