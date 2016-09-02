QT       += core network websockets widgets
QT       -= gui

#Wee need that for qwinoverlappedionotifier class which is private
win32: QT += core-private

TEMPLATE = app

TARGET = moolticuted
CONFIG += console
CONFIG -= app_bundle

CONFIG += c++11

win32 {
    LIBS += -lsetupapi
} else:linux {
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += libusb-1.0
} else:mac {
    QMAKE_LFLAGS += -framework ApplicationServices -framework IOKit -framework CoreFoundation
}

win32 {
    SOURCES += src/UsbMonitor_win.cpp \
               src/MPDevice_win.cpp \
               src/HIDLoader.cpp
    HEADERS += src/UsbMonitor_win.h \
               src/MPDevice_win.h \
               src/HIDLoader.h \
               src/hid_dll.h
}
linux {
    SOURCES += src/UsbMonitor_linux.cpp \
               src/MPDevice_linux.cpp
    HEADERS += src/UsbMonitor_linux.h \
               src/MPDevice_linux.h
}
mac {
    SOURCES += src/UsbMonitor_mac.cpp \
               src/MPDevice_mac.cpp
    HEADERS += src/UsbMonitor_mac.h \
               src/MPDevice_mac.h

    HEADERS += src/MacUtils.h
    SOURCES += src/MacUtils.mm
}

SOURCES += src/main_daemon.cpp \
    src/MPDevice.cpp \
    src/MPManager.cpp \
    src/Common.cpp \
    src/WSServer.cpp \
    src/AppDaemon.cpp \
    src/AsyncJobs.cpp \
    src/MPNode.cpp \
    src/WSServerCon.cpp \
    src/MPDevice_emul.cpp \
    src/http-parser/http_parser.c \
    src/HttpClient.cpp \
    src/HttpServer.cpp

HEADERS  += \
    src/Common.h \
    src/MPDevice.h \
    src/MPManager.h \
    src/MooltipassCmds.h \
    src/QtHelper.h \
    src/WSServer.h \
    src/AppDaemon.h \
    src/AsyncJobs.h \
    src/MPNode.h \
    src/version.h \
    src/WSServerCon.h \
    src/MPDevice_emul.h \
    src/http-parser/http_parser.h \
    src/HttpClient.h \
    src/HttpServer.h

DISTFILES += \
    src/http-parser/CONTRIBUTIONS \
    src/http-parser/http_parser.gyp \
    src/http-parser/LICENSE-MIT \
    src/http-parser/AUTHORS \
    src/http-parser/README.md
