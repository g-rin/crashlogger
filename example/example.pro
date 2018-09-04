include($${PWD}/../config.pri)
TEMPLATE = app
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QMAKE_CXXFLAGS += -std=c++11 -funwind-tables -rdynamic
SOURCES += *.cpp
HEADERS += *.h
INCLUDEPATH += $$PWD/../include
LIBS += -lcrashloger