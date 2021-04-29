QT += core gui widgets
CONFIG += c++11
TARGET = NeuroPlayMinApp
TEMPLATE = app

include(../DestDirBuilder.pri)

INCLUDEPATH += $$PWD/../lib
CONFIG(debug, debug|release): SUFFIX = d
LIBS += -L$${DESTDIR}/ -lNeuroPlayMin$${SUFFIX}

HEADERS += \
    MainWindow.h

SOURCES += \
    main.cpp \
    MainWindow.cpp


