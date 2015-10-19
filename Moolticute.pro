#-------------------------------------------------
#
# Project created by QtCreator 2015-10-09T15:28:31
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Moolticute
TEMPLATE = app

CONFIG += c++11

#Add "CONFIG+=DEV" in command line
win32 {
    DEV {
        LIBS += -LC:/libusb/MinGW32/dll -lusb-1.0
        INCLUDEPATH += C:/libusb/include/libusb-1.0
    } else {
        LIBS += -L/opt/windows_32/bin -lusb-1.0
        INCLUDEPATH += /opt/windows_32/include/libusb-1.0
    }
} else:unix {
    QT_CONFIG -= no-pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += libusb-1.0
}

SOURCES += src/main.cpp\
        src/MainWindow.cpp \
    src/MPDevice.cpp \
    src/MPManager.cpp \
    src/Common.cpp

win32 {
    SOURCES += src/UsbMonitor_win.cpp
    HEADERS += src/UsbMonitor_win.h
} else:unix {
    SOURCES += src/UsbMonitor_linux.cpp
    HEADERS += src/UsbMonitor_linux.h
}

HEADERS  += src/MainWindow.h \
    src/Common.h \
    src/MPDevice.h \
    src/MPManager.h

FORMS    += src/MainWindow.ui
