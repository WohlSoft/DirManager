TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

CONFIG += c++11

include(src/dirman.pri)

SOURCES += \
    test/main.cpp
