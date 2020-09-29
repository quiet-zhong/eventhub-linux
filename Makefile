gCROSS_COMPILE=

VERSION = 1.0
LIB_NAME = libeventhub
LIB_SO_NAME = $(LIB_NAME).so

LIB_SO	= $(LIB_SO_NAME).$(VERSION)
LIB_A	= $(LIB_NAME).a


CFLAGS	+= -fPIC -Wall -O2
#CFLAGS	+= -g -rdynamic
CFLAGS	+= -DDEBUG

CXXFLAGS += -std=c++11

INCLUDE += -I./ -I./include
DEPLIBS += -lpthread -lstdc++

CC	= $(CROSS_COMPILE)gcc
CXX	= $(CROSS_COMPILE)g++
AR	= $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip

COBJS	+= $(patsubst %.c, %.o, $(wildcard *.c))
CXXOBJS	+= $(patsubst %.cpp, %.o, $(wildcard *.cpp))


all:lib demo

lib:$(LIB_SO) $(LIB_A)

demo:$(COBJS) $(CXXOBJS)
	$(CC) $(CFLAGS) $(wildcard demo/*.c) $^ -o $@.bin $(INCLUDE) $(DEPLIBS)
	$(STRIP) --strip-all $@.bin

$(LIB_SO):$(COBJS) $(CXXOBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(INCLUDE)
	$(STRIP) --strip-all $@

$(LIB_A):$(COBJS) $(CXXOBJS)
	$(AR) -rcs $@ $^ $(LIBS)

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^ $(INCLUDE)

$(CXXOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -o $@ -c $^ $(INCLUDE)


.PHONY:clean

clean:
	rm -f $(COBJS) $(CXXOBJS) $(LIB_SO) $(LIB_A) demo.bin
