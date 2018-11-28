include($${PWD}/config.pri)
include($${PWD}/crashlogger.pri)
TEMPLATE = lib
QT = core
HEADERS += include/crashlogger.h
SOURCES += src/crashlogger.cpp
shared: LIBS *= -lbacktrace
