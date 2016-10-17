
INCLUDEPATH += $$PWD/..

win32 {
LIBS += -ladvapi32
SOURCES += $$PWD/PageantWin.cpp
HEADERS += $$PWD/PageantWin.h \
        $$PWD/ressource.h
}

SOURCES += $$PWD/SshAgent.cpp
HEADERS += $$PWD/SshAgent.h
