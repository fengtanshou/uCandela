
TARGET=hidtool
CSOURCES=hidtool.c

CFLAGS+=-std=c99 -Wall -Werror -D_XOPEN_SOURCE -D_BSD_SOURCE -g
CC=gcc
CXX=g++

AOBJECTS:=$(ASOURCES:.S=.o)
COBJECTS:=$(CSOURCES:.c=.o)
CXXOBJECTS:=$(CXXSOURCES:.cpp=.o)
OBJECTS:=$(AOBJECTS) $(COBJECTS) $(CXXOBJECTS)

all: build

build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(AOBJECTS):
%.o: %.S
	$(AS) -c $(ASFLAGS) $< -o $@

$(COBJECTS):
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

#
# utility
#
.PHONY: clean build release all

clean:
	-rm -f $(TARGET)
	-rm -f $(OBJECTS)
