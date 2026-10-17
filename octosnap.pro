QT += core gui widgets
CONFIG += c++17

TARGET = octosnap
TEMPLATE = app

DEFINES += OCTOSNAP_VERSION=\\\"0.2.1\\\"

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/Theme.cpp \
    src/FssBackend.cpp \
    src/RestoreDialog.cpp

HEADERS += \
    src/MainWindow.h \
    src/Theme.h \
    src/FssBackend.h \
    src/RestoreDialog.h

INCLUDEPATH += src

TRANSLATIONS += \
    translations/octosnap_pt_BR.ts \
    translations/octosnap_en.ts

target.path = /usr/local/bin
desktop.path = /usr/local/share/applications
desktop.files = octosnap.desktop

INSTALLS += target desktop
