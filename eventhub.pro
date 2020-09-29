TEMPLATE = app
CONFIG += c++11 #console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += DEBUG

QMAKE_CFLAGS += -fPIC -g -rdynamic
QMAKE_CXXFLAGS += -fPIC -g -rdynamic

LIBS += -lpthread

#INCLUDEPATH += $$PWD

HEADERS += \ 
    hl_type.h \
    libunixsocket.h \
    EventHubInner.h \
    EventHub.h \
    debug.h \
    dump.h \
    threadpool.h

SOURCES += \ 
    libunixsocket.c \
    EventHubInner.cpp \
    EventHub.cpp \
    demo/main.c \
    dump.c \
    threadpool.c

