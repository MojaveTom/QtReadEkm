#-------------------------------------------------
#
# Project created by QtCreator 2015-08-15T16:35:11
#
#-------------------------------------------------

QT       += core sql serialport

QT       -= gui

TARGET = ReadEKM
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    ../SupportRoutines/supportfunctions.cpp \
    messages.cpp \
    EkmCRC.cpp

HEADERS += \
    ../SupportRoutines/supportfunctions.h \
    messages.h

DISTFILES += \
    DoLink.sh \
    GetArchiveTag.sh \
    .gitignore \
    Notes.txt \
    RefreshReadEKM.sh \
    README.md

DEFINES += QT_MESSAGELOGCONTEXT
