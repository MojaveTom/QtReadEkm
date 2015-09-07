#-------------------------------------------------
#
# Project created by QtCreator 2015-08-15T16:35:11
#
#    ReadEKM project description file.
#    Copyright (C) 2015  Thomas A. DeMay
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
    README.md \
    LICENSE.txt \
    Doxyfile

DEFINES += QT_MESSAGELOGCONTEXT
DEFINES += SOURCE_DIR=\'\"$$_PRO_FILE_PWD_\"\'
