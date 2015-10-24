#-------------------------------------------------
#
# Project created by QtCreator 2015-10-09T15:28:31
#
#-------------------------------------------------

QT       += core gui

#Wee need that for qwinoverlappedionotifier class which is private
win32: QT += core-private

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Moolticute
TEMPLATE = app

CONFIG += c++11

win32 {
    LIBS += -lSetupApi
} else:linux {
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += libusb-1.0
} else:mac {
    QMAKE_LFLAGS += -framework IOKit -framework CoreFoundation
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
}

SOURCES += src/main.cpp\
        src/MainWindow.cpp \
    src/MPDevice.cpp \
    src/MPManager.cpp \
    src/Common.cpp

HEADERS  += src/MainWindow.h \
    src/Common.h \
    src/MPDevice.h \
    src/MPManager.h \
    src/MooltipassCmds.h \
    src/QtHelper.h

FORMS    += src/MainWindow.ui
