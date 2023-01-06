QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtFramelessWindow
TEMPLATE = lib
CONFIG += staticlib

HEADERS += \
    include/QtFramelessWindow.h

win32{
	SOURCES += \
		QtFramelessWindow.cpp
}
