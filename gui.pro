QT       += core network websockets gui widgets

TEMPLATE = app

TARGET = MoolticuteApp

CONFIG += c++11

include(src/QtAwesome/QtAwesome/QtAwesome.pri)

SOURCES += src/main_gui.cpp \
    src/MainWindow.cpp \
    src/Common.cpp \
    src/WSClient.cpp \
    src/RotateSpinner.cpp \
    src/CredentialsModel.cpp \
    src/DialogEdit.cpp

HEADERS  += src/MainWindow.h \
    src/Common.h \
    src/QtHelper.h \
    src/WSClient.h \
    src/RotateSpinner.h \
    src/CredentialsModel.h \
    src/DialogEdit.h \
    src/version.h

INCLUDEPATH += src

FORMS    += src/MainWindow.ui \
    src/DialogEdit.ui

RESOURCES += \
    img/images.qrc

win32 {
    RC_FILE = win/windows_res.rc
}
