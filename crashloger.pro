include($${PWD}/config.pri)
include($${PWD}/crashloger.pri)
TEMPLATE = lib
CONFIG += staticlib
QT = core
DEFINES *= _GNU_SOURCE

SOURCES += \
    src/crashloger.cpp \
    src/backtrace.c \
    src/simple.c \
    src/elf.c \
    src/dwarf.c \
    src/mmapio.c \
    src/mmap.c \
    src/atomic.c \
    src/fileline.c \
    src/posix.c \
    src/print.c \
    src/sort.c \
    src/state.c

