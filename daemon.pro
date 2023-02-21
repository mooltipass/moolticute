QT       += core network websockets widgets
QT       -= gui

#We need that for qwinoverlappedionotifier class which is private
win32: QT += core-private

TEMPLATE = app

TARGET = moolticuted
CONFIG -= app_bundle

CONFIG += c++11

INCLUDEPATH += $$PWD/src $$PWD/src/MessageProtocol $$PWD/src/Mooltipass $$PWD/src/Settings $$PWD/src/CyoEncode $$PWD/src/SimpleCrypt

win32 {
    LIBS += -lsetupapi -luser32
} else:linux {
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += libudev
    QT -= widgets
} else:mac {
    LIBS += -framework ApplicationServices -framework IOKit -framework CoreFoundation -framework Cocoa -framework Foundation
}

win32 {
    #Private header qwinoverlappedionotifier_p.h removed from Qt5.10 SDK
    #so need to include files explicitly.
    if (equals(QT_MAJOR_VERSION, 5):greaterThan(QT_MINOR_VERSION, 9)|greaterThan(QT_MAJOR_VERSION, 5)){
        INCLUDEPATH += $$PWD/src/qwinoverlappedionotifier
        SOURCES += src/qwinoverlappedionotifier/qwinoverlappedionotifier.cpp
        HEADERS += src/qwinoverlappedionotifier/qwinoverlappedionotifier.h
    }

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
    src/CyoEncode/Base32.cpp \
    src/CyoEncode/CyoDecode.c \
    src/CyoEncode/CyoEncode.c \
    src/MPDevice.cpp \
    src/MPDevice_localSocket.cpp \
    src/MPManager.cpp \
    src/Common.cpp \
    src/Mooltipass/MPMiniToBleNodeConverter.cpp \
    src/WSServer.cpp \
    src/AppDaemon.cpp \
    src/AsyncJobs.cpp \
    src/Mooltipass/MPNode.cpp \
    src/WSServerCon.cpp \
    src/MPDevice_emul.cpp \
    src/http-parser/http_parser.c \
    src/HttpClient.cpp \
    src/HttpServer.cpp \
    src/MooltipassCmds.cpp \
    src/FilesCache.cpp \
    src/SimpleCrypt/SimpleCrypt.cpp \
    src/ParseDomain.cpp \
    src/MessageProtocol/MessageProtocolMini.cpp \
    src/MessageProtocol/MessageProtocolBLE.cpp \
    src/MPDeviceBleImpl.cpp \
    src/HaveIBeenPwned.cpp \
    src/Mooltipass/MPNodeMini.cpp \
    src/Mooltipass/MPNodeBLE.cpp \
    src/Mooltipass/MPSettingsMini.cpp \
    src/Mooltipass/MPSettingsBLE.cpp \
    src/Settings/DeviceSettings.cpp \
    src/Settings/DeviceSettingsMini.cpp \
    src/Settings/DeviceSettingsBLE.cpp \
    src/Mooltipass/MPBLEFreeAddressProvider.cpp

HEADERS  += \
    src/Common.h \
    src/CyoEncode/Base32.h \
    src/CyoEncode/CyoDecode.h \
    src/CyoEncode/CyoDecode.hpp \
    src/CyoEncode/CyoEncode.h \
    src/CyoEncode/CyoEncode.hpp \
    src/MPDevice.h \
    src/MPDevice_localSocket.h \
    src/MPManager.h \
    src/Mooltipass/MPMiniToBleNodeConverter.h \
    src/MooltipassCmds.h \
    src/QtHelper.h \
    src/WSServer.h \
    src/AppDaemon.h \
    src/AsyncJobs.h \
    src/Mooltipass/MPNode.h \
    src/version.h \
    src/WSServerCon.h \
    src/MPDevice_emul.h \
    src/http-parser/http_parser.h \
    src/HttpClient.h \
    src/HttpServer.h \
    src/FilesCache.h \
    src/SimpleCrypt/SimpleCrypt.h \
    src/ParseDomain.h \
    src/MessageProtocol/IMessageProtocol.h \
    src/MessageProtocol/MessageProtocolMini.h \
    src/MessageProtocol/MessageProtocolBLE.h \
    src/MPDeviceBleImpl.h \
    src/HaveIBeenPwned.h \
    src/BleCommon.h \
    src/Mooltipass/MPNodeMini.h \
    src/Mooltipass/MPNodeBLE.h \
    src/Mooltipass/MPSettingsMini.h \
    src/Mooltipass/MPSettingsBLE.h \
    src/Settings/DeviceSettings.h \
    src/Settings/DeviceSettingsMini.h \
    src/Settings/DeviceSettingsBLE.h \
    src/Mooltipass/MPBLEFreeAddressProvider.h \
    src/utils/qurltlds_p.h

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

unix {
    # INSTALL RULES
    #
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }
    DEFINES += MC_INSTALL_PREFIX=\\\"$$PREFIX\\\"

    # install the binary
    target.path = $$PREFIX/bin
    INSTALLS += target

    # systemd service files
    systemd_user.path = $$PREFIX/lib/systemd/system/
    systemd_user.files += $$PWD/systemd/moolticuted.service
    INSTALLS += systemd_user
}
