#-------------------------------------------------
#
# Project created by QtCreator 2017-08-01T23:36:26
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

QMAKE_CXXFLAGS += -std=c++0x

TARGET = tests
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include (../src/QSimpleUpdater/QSimpleUpdater.pri)

SOURCES += \
    ../src/SimpleCrypt/SimpleCrypt.cpp \
    ../src/FilesCache.cpp \
    ../src/DbBackupsTracker.cpp \
    ../src/TreeItem.cpp \
    ../src/RootItem.cpp \
    ../src/LoginItem.cpp \
    ../src/ServiceItem.cpp \
    ../src/CredentialModel.cpp \
    ../src/CredentialModelFilter.cpp \
    ../src/DbExportsRegistry.cpp \
    ../src/DbBackupChangeNumbersComparator.cpp \
    main.cpp \
    FilesCacheTests.cpp \
    UpdaterTests.cpp \
    DbBackupsTrackerTests.cpp \
    TestTreeItem.cpp \
    TestCredentialModel.cpp \
    TestCredentialModelFilter.cpp \
    TestDbExportsRegistry.cpp

HEADERS += \
    ../src/SimpleCrypt/SimpleCrypt.h \
    ../src/FilesCache.h \
    ../src/DbBackupsTracker.h\
    ../src/TreeItem.h \
    ../src/RootItem.h \
    ../src/LoginItem.h \
    ../src/ServiceItem.h \
    ../src/CredentialModel.h \
    ../src/CredentialModelFilter.h \
    ../src/DbExportsRegistry.h \
    ../src/DbBackupChangeNumbersComparator.h \
    UpdaterTests.h \
    FilesCacheTests.h \
    DbBackupsTrackerTests.h \
    TestTreeItem.h \
    TestCredentialModel.h \
    TestCredentialModelFilter.h \
    TestDbExportsRegistry.h

DEFINES += SRCDIR=\\\"$$PWD/\\\"
