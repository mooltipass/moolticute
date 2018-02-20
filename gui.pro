QT       += core network websockets gui widgets

TEMPLATE = app

TARGET = moolticute

CONFIG += c++11

mac {
    LIBS += -framework ApplicationServices -framework IOKit -framework CoreFoundation -framework Cocoa -framework Foundation
}
win32 {
    LIBS += -luser32
}

include(src/QtAwesome/QtAwesome/QtAwesome.pri)
include (src/QSimpleUpdater/QSimpleUpdater.pri)

SOURCES += src/main_gui.cpp \
    src/MainWindow.cpp \
    src/Common.cpp \
    src/WSClient.cpp \
    src/RotateSpinner.cpp \
    src/AppGui.cpp \
    src/DaemonMenuAction.cpp \
    src/AutoStartup.cpp \
    src/WindowLog.cpp \
    src/OutputLog.cpp \
    src/AnsiEscapeCodeHandler.cpp \
    src/PasswordLineEdit.cpp \
    src/CredentialsManagement.cpp \
    src/zxcvbn-c/zxcvbn.c \
    src/FilesManagement.cpp \
    src/SSHManagement.cpp \
    src/CredentialView.cpp \
    src/CredentialModel.cpp \
    src/CredentialModelFilter.cpp \
    src/ItemDelegate.cpp \
    src/LoginItem.cpp \
    src/TreeItem.cpp \
    src/ServiceItem.cpp \
    src/RootItem.cpp \
    src/AnimatedColorButton.cpp \
    src/PasswordProfilesModel.cpp \
    src/PassGenerationProfilesDialog.cpp \
    src/DbBackupsTracker.cpp \
    src/DbBackupsTrackerController.cpp \
    src/PromptWidget.cpp \
    src/DbExportsRegistry.cpp \
    src/DbExportsRegistryController.cpp \
    src/DbBackupChangeNumbersComparator.cpp

HEADERS  += src/MainWindow.h \
    src/Common.h \
    src/QtHelper.h \
    src/WSClient.h \
    src/RotateSpinner.h \
    src/version.h \
    src/AppGui.h \
    src/DaemonMenuAction.h \
    src/AutoStartup.h \
    src/WindowLog.h \
    src/OutputLog.h \
    src/AnsiEscapeCodeHandler.h \
    src/PasswordLineEdit.h \
    src/CredentialsManagement.h \
    src/zxcvbn-c/dict-src.h \
    src/zxcvbn-c/zxcvbn.h \
    src/FilesManagement.h \
    src/SSHManagement.h \
    src/CredentialView.h \
    src/ServiceItem.h \
    src/TreeItem.h \
    src/RootItem.h \
    src/LoginItem.h \
    src/ItemDelegate.h \
    src/CredentialModel.h \
    src/CredentialModelFilter.h \
    src/AnimatedColorButton.h \
    src/PasswordProfilesModel.h \
    src/PassGenerationProfilesDialog.h \
    src/DbBackupsTracker.h \
    src/DbBackupsTrackerController.h \
    src/PromptWidget.h \
    src/DbExportsRegistry.h \
    src/DbExportsRegistryController.h \
    src/DbBackupChangeNumbersComparator.h

mac {
    HEADERS += src/MacUtils.h
    OBJECTIVE_SOURCES += src/MacUtils.mm
}

INCLUDEPATH += src\
    src/zxcvbn-c

FORMS    += src/MainWindow.ui \
    src/WindowLog.ui \
    src/CredentialsManagement.ui \
    src/FilesManagement.ui \
    src/SSHManagement.ui \
    src/PassGenerationProfilesDialog.ui

RESOURCES += \
    img/images.qrc \
    lang.qrc

win32 {
    RC_FILE = win/windows_res.rc
}

mac {
    ICON = img/AppIcon.icns
} else {
    ICON = img/AppIcon.svg
}

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

    # install the desktop files
    xdgdesktop.path = $$PREFIX/share/applications
    xdgdesktop.files += $$PWD/data/moolticute.desktop
    INSTALLS += xdgdesktop

    # install icons
    iconScalable.path = $$PREFIX/share/icons/hicolor/scalable/apps
    iconScalable.extra = cp -f $$PWD/img/AppIcon.svg $(INSTALL_ROOT)$$iconScalable.path/moolticute.svg
    icon32.path = $$PREFIX/share/icons/hicolor/32x32/apps
    icon32.extra = cp $$PWD/img/AppIcon_32.png $(INSTALL_ROOT)$$icon32.path/moolticute.png
    icon128.path = $$PREFIX/share/icons/hicolor/128x128/apps
    icon128.extra = cp $$PWD/img/AppIcon_128.png $(INSTALL_ROOT)$$icon128.path/moolticute.png
    INSTALLS += iconScalable icon32 icon128
}

TRANSLATIONS = \
    lang/mc_fr.ts \
    lang/mc_de.ts \
    lang/mc_ru.ts \
    lang/mc_nl.ts \
    lang/mc_ja.ts \
    lang/mc_pt.ts \
    lang/mc_br.ts \
    lang/mc_tr.ts \
    lang/mc_sv.ts

#Build *.qm translation files automatically

isEmpty(QMAKE_LRELEASE) {
    QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}

updateqm.input = TRANSLATIONS
updateqm.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm
PRE_TARGETDEPS += compiler_updateqm_make_all
