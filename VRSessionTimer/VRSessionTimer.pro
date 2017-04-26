#-------------------------------------------------
#
# Project created by QtCreator 2015-06-10T16:57:45
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VRSessionTimer
TEMPLATE = app


SOURCES += main.cpp\
    overlaywidget.cpp \
    openvroverlaycontroller.cpp \
    vrnotification.cpp \
    mainwindow.cpp

HEADERS  += overlaywidget.h \
    openvroverlaycontroller.h \
    vrnotification.h \
    mainwindow.h

FORMS    += overlaywidget.ui \
    vrnotification.ui \
    mainwindow.ui

#INCLUDEPATH += ../../openvr/headers

#LIBS += -L../../openvr/lib/win64 -lopenvr_api
LIBS += -L$$PWD/../openvr/lib/win64/ -lopenvr_api

INCLUDEPATH += $$PWD/../openvr/headers
DEPENDPATH += $$PWD/../openvr/headers

Release:DESTDIR = $$PWD/../build/release/bin/win64
Release:MOC_DIR = $$PWD/../build/release/.moc
Release:RCC_DIR = $$PWD/../build/release/.rcc

Debug:DESTDIR = $$PWD/../build/debug/bin/win64
Debug:MOC_DIR = $$PWD/../build/debug/.moc
Debug:RCC_DIR = $$PWD/../build/debug/.rcc
Debug:OBJECTS_DIR = $$PWD/../build/MSVC2013_64bit/debug

#QMAKE_LFLAGS += -static-libgcc -static-libstdc++

