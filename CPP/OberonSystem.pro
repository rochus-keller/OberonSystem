#/*
#* Copyright 2020 Rochus Keller <mailto:me@rochus-keller.ch>
#*
#* This file is part of the Oberon parser library.
#*
#* The following is the license that applies to this copy of the
#* library. For a license to use the library under conditions
#* other than those described here, please email to me@rochus-keller.ch.
#*
#* GNU General Public License Usage
#* This file may be used under the terms of the GNU General Public
#* License (GPL) versions 2.0 or 3.0 as published by the Free Software
#* Foundation and appearing in the file LICENSE.GPL included in
#* the packaging of this file. Please review the following information
#* to ensure GNU General Public Licensing requirements will be met:
#* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
#* http://www.gnu.org/copyleft/gpl.html.
#*/

QT       = core gui widgets

TARGET = OberonSystem
TEMPLATE = app

INCLUDEPATH +=  ..

SOURCES += \ 
    ObDisplay.cpp \
    ObEdit.cpp \
    ObFileDir.cpp \
    ObFiles.cpp \
    ObFonts.cpp \
    ObInput.cpp \
    ObKernel.cpp \
    ObMenuViewers.cpp \
    ObModules.cpp \
    ObOberon.cpp \
    ObSystem.cpp \
    ObTextFrames.cpp \
    ObTexts.cpp \
    ObViewers.cpp \
    Ob_Global.cpp \
    main.cpp \
    Ob_SYSTEM.cpp \
    QtDisplay.cpp

CONFIG(debug, debug|release) {
        DEFINES += _DEBUG
}

!win32 {
QMAKE_CXXFLAGS += -Wno-reorder -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
}

HEADERS += \ 
    ObDisplay.h \
    ObEdit.h \
    ObFileDir.h \
    ObFiles.h \
    ObFonts.h \
    ObInput.h \
    ObKernel.h \
    ObMenuViewers.h \
    ObModules.h \
    ObOberon.h \
    ObSystem.h \
    ObTextFrames.h \
    ObTexts.h \
    ObViewers.h \
    Ob_Global.h \
    Ob_SYSTEM.h \
    QtDisplay.h

DISTFILES +=



