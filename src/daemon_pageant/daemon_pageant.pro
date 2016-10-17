TEMPLATE = app

QT += core network websockets gui widgets

CONFIG += c++11

TARGET = moolticute_pageant

LIBS += -ladvapi32

INCLUDEPATH += ..

SOURCES += main.cpp \
    PageantWin.cpp \
    ../Common.cpp \
    SshAgent.cpp

HEADERS += \
    PageantWin.h \
    ../Common.h \
    ressource.h \
    SshAgent.h

RC_FILE = ../../win/windows_res.rc
