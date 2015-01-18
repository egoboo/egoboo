#-------------------------------------------------
#
# Project created by QtCreator 2014-12-28T01:07:36
#
#-------------------------------------------------

QT        += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

lessThan(QT_MAJOR_VERSION, 5) | if(equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 4)) {
    macx: QMAKE_MAC_SDK = macosx10.10
    QT    += opengl
}

TARGET     = EOV-qt
TEMPLATE   = app
VERSION    = 0.1

SOURCES   += main.cpp \
           controllerwindow.cpp \
           modelwidget.cpp \
           md2/Md2Model.cpp \
           md2/id_normals.cpp

HEADERS   += controllerwindow.h \
           modelwidget.h \
           md2/id_md2.h \
           md2/Md2Model.h

FORMS     += controllerwindow.ui

DISTFILES += COPYING

macx: QMAKE_INFO_PLIST = EOV-qt-Info.plist

