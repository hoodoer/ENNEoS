TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt


QMAKE_CXXFLAGS_WARN_ON = -Wall -Wextra -Wno-enum-compare
LIBS += -lpthread

SOURCES += main.cpp \
    Cga.cpp \
    CInnovation.cpp \
    CSpecies.cpp \
    genotype.cpp \
    phenotype.cpp \
    sharedmemory.cpp \
    shellybot.cpp

SUBDIRS += \
    enneosEncoder.pro

DISTFILES += \
    enneosEncoder.pro.user

HEADERS += \
    Cga.h \
    CInnovation.h \
    CSpecies.h \
    genes.h \
    genotype.h \
    parameters.h \
    phenotype.h \
    timer.h \
    utils.h \
    encodedOutput.h \
    sharedmemory.h \
    sharedmemory.h \
    shellybot.h
