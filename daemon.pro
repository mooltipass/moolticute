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
    LIBS += -framework ApplicationServices -framework IOKit -framework CoreFoundation -framework Cocoa -framework Foundation
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
    src/http-parser/README.md \
    data/debug/app/scripts/main.js \
    data/debug/dist/scripts/main.js \
    data/debug/dist/scripts/plugins.js \
    data/debug/dist/scripts/vendor.js \
    data/debug/Gruntfile.js \
    data/debug/bower.json \
    data/debug/package.json \
    data/debug/dist/fonts/glyphicons-halflings-regular.eot \
    data/debug/dist/fonts/glyphicons-halflings-regular.woff \
    data/debug/dist/fonts/glyphicons-halflings-regular.ttf \
    data/debug/dist/fonts/glyphicons-halflings-regular.svg \
    data/debug/app/styles/main.css \
    data/debug/dist/styles/main.css \
    data/debug/dist/styles/vendor.css \
    data/debug/app/index.html \
    data/debug/dist/index.html \
    data/debug/dist/Makefile \
    data/debug/dist/Makefile.am \
    data/debug/dist/Makefile.in \
    data/debug/README.md

RESOURCES += \
    data/data_debug.qrc
