TEMPLATE = app

QT += core network websockets gui widgets

CONFIG += c++11

TARGET = moolticute_pageant

INCLUDEPATH += ..

SOURCES += main.cpp \
    PageantWin.cpp \
    ../Common.cpp

HEADERS += \
    PageantWin.h \
    ../Common.h \
    ressource.h

RC_FILE = ../../win/windows_res.rc
