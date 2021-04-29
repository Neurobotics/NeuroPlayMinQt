QT += core bluetooth
CONFIG += c++11
DEFINES += CORE_LIBRARY
TARGET = NeuroPlayMin
TEMPLATE = lib

include(../DestDirBuilder.pri)

SOURCES += \
    Core.cpp \
    Data/BleDevice.cpp \
    Data/BleDiscovery.cpp \
    Data/BleDiscoveryController.cpp \
    Data/BleService.cpp \
    Data/NeuroplayDataGrabber.cpp \
    Data/NeuroplayDevice.cpp \
    helpers/ConnectionHelper.cpp

HEADERS += \
    Core.h \
    Core_global.h \
    Data/BleDevice.h \
    Data/BleDiscovery.h \
    Data/BleDiscoveryController.h \
    Data/BleService.h \
    Data/NeuroplayDataGrabber.h \
    Data/NeuroplayDevice.h
