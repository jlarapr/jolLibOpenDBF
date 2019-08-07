# Original Source Code
# https://github.com/bcsanches/dbf2txt

TEMPLATE = lib
# CONFIG += dll
CONFIG += staticlib
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++17

VERSION=1.0.0.1
QMAKE_TARGET_COMPANY = libOpenDBF
QMAKE_TARGET_PRODUCT = libOpenDBF
QMAKE_TARGET_DESCRIPTION = Lib Open DBF
QMAKE_TARGET_COPYRIGHT = libOpenDBF
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

nx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


isEmpty(TARGET_EXT) {
    win32 {
        #TARGET_CUSTOM_EXT = .exe
    }
    macx {
        TARGET_CUSTOM_EXT = .app
    }
} else {
    TARGET_CUSTOM_EXT = $${TARGET_EXT}
}


CONFIG(release, debug|release):{
     # release
     release: DESTDIR = "release"
     #DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/.executable/$${TARGET}$${TARGET_CUSTOM_EXT}))
     DEPLOY_TARGET = $$shell_quote($$shell_path(/.executable/$${TARGET}$${TARGET_CUSTOM_EXT}))
    MyMakefile = Makefile.Release
 }else:CONFIG(debug, debug|release):{
     # debug
     debug: DESTDIR = "debug"
     #DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/.executable/$${TARGET}$${TARGET_CUSTOM_EXT}))
     DEPLOY_TARGET = $$shell_quote($$shell_path(/.executable/$${TARGET}$${TARGET_CUSTOM_EXT}))
     #DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/$${TARGET}$${TARGET_CUSTOM_EXT}))
     MyMakefile = Makefile.Debug
 }

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui
QMAKE_MAKEFILE = $$DESTDIR/Makefile
MAKEFILE = $$DESTDIR/$$MyMakefile
TARGET = $$DEPLOY_TARGET

SOURCES += \
        src/dbffile.cpp

HEADERS += \
    src/dbffile.h

DISTFILES += \
    README.md
