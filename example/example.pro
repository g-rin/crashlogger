include($${PWD}/../config.pri)
include($${PWD}/../crashlogger.prf)
TEMPLATE = app
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QMAKE_CXXFLAGS += -std=c++11 -funwind-tables -rdynamic -g
SOURCES += *.cpp
HEADERS += *.h
INCLUDEPATH += $$PWD/../include

race {
    QMAKE_CXXFLAGS *= -fsanitize=address
    QMAKE_CFLAGS *= -fsanitize=address
    QMAKE_LFLAGS *= -fsanitize=address
    QMAKE_CXXFLAGS *= -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer
}
