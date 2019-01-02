TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += ../enneosEncoder

QMAKE_CXXFLAGS +=-fno-stack-protector -z execstack

SOURCES += main.cpp \
    ../enneosEncoder/genotype.cpp \
    ../enneosEncoder/phenotype.cpp \
    ../enneosEncoder/shellybot.cpp \
    ../enneosEncoder/CInnovation.cpp


