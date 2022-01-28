TEMPLATE = app
TARGET = genavb-gui-controller

QT = core gui
greaterThan(QT_MAJOR_VERSION, 4): QT+=widgets

QMAKE_CC=$(CC)
QMAKE_CXX=$(CXX)
QMAKE_LINK=$(CXX)
QMAKE_AR=$(AR)

SOURCES += \
    main.cpp \
    window.cpp \
    avbcontroller.cpp \
    avbinputeventlistener.cpp \
    avb.c \
    ../../common/common.c \
    ../../common/time.c \
    ../../common/stats.c \
    ../../common/aecp.c

HEADERS += \
    window.h \
    avbcontroller.h \
    avbinputeventlistener.h \
    avb.h \
    ../../common/common.h \
    ../../common/time.h \
    ../../common/stats.h \
    ../../common/aecp.h

FORMS += \
    mainform.ui

RESOURCES += \
    gui_elements.qrc

