QT       += core network websockets gui widgets

TEMPLATE = app

TARGET = MoolticuteApp

CONFIG += c++11

mac {
    QMAKE_LFLAGS += -framework ApplicationServices
}

include(src/QtAwesome/QtAwesome/QtAwesome.pri)

SOURCES += src/main_gui.cpp \
    src/MainWindow.cpp \
    src/Common.cpp \
    src/WSClient.cpp \
    src/RotateSpinner.cpp \
    src/CredentialsModel.cpp \
    src/DialogEdit.cpp \
    src/AppGui.cpp \
    src/DaemonMenuAction.cpp \
    src/AutoStartup.cpp

HEADERS  += src/MainWindow.h \
    src/Common.h \
    src/QtHelper.h \
    src/WSClient.h \
    src/RotateSpinner.h \
    src/CredentialsModel.h \
    src/DialogEdit.h \
    src/version.h \
    src/AppGui.h \
    src/DaemonMenuAction.h \
    src/AutoStartup.h

mac {
    HEADERS += src/MacUtils.h
    SOURCES += src/MacUtils.mm
}

INCLUDEPATH += src

FORMS    += src/MainWindow.ui \
    src/DialogEdit.ui

RESOURCES += \
    img/images.qrc

win32 {
    RC_FILE = win/windows_res.rc
}

mac {
    ICON = img/AppIcon.icns
} else {
    ICON = img/AppIcon.svg
}
