QT       += core gui network webenginewidgets webchannel serialport svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

lessThan(QT_MAJOR_VERSION, 6): win32: QMAKE_CXXFLAGS += -execution-charset:utf-8

# make the application have authority of running on Ubuntu, and I do not know why it works
unix: QMAKE_LFLAGS += -no-pie

# Qt 5.14 VERSION can only contains numbers, no any others type of character please
# please do not put 0 before any version number, because this will cause a warnning on Qt 5.14
win32:  VERSION = 22.09.19.1017                # major.minor.patch.build
else:   VERSION = 22.09.19                    # major.minor.patch

QMAKE_TARGET_COPYRIGHT = Copyright 2020-2022 Snailfish DeepSea Tech

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# 禁用 QDebug 输出调试i信息
#DEFINES += QT_NO_DEBUG_OUTPUT

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TARGET = Auxilium

DESTDIR = ../bin

SOURCES += \
    Dialog/dialogsetting.cpp \
    Packer/linuxpacker.cpp \
    Packer/windowspacker.cpp \
    Public/appconfig.cpp \
    Public/appsignal.cpp \
    Widget/buttondesigned.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Dialog/dialogsetting.h \
    Packer/linuxpacker.h \
    Packer/windowspacker.h \
    Public/appconfig.h \
    Public/appsignal.h \
    Widget/buttondesigned.h \
    mainwindow.h

FORMS += \
    Dialog/dialogsetting.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RC_ICONS = ./Resource/icon/icon.ico

RESOURCES += \
    res.qrc

#0x0800代表和系统当前语言一致
RC_LANG = 0x0800

win32
{
    LIBS += -lUser32 -lKernel32
}
