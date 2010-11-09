#
# parameters
#
TARGET:=sensor
FORMAT:=ihex
ASOURCES:=
CSOURCES:=main.c
ifndef NDEBUG
ASOURCES+=uart.S
CSOURCES+=picofmt.c
endif
TESTS=picofmt
DEFINES = F_CPU=12000000

#
# (CKDIV=0) start with clk/8
# (CKOUT=1) disable clock out
# (CKSEL=2) internal rc osc
# (SUT=0) start up time - 14CK
# (BODLEVEL=6) set BOD to 1v8
# (WDTON=1) do not use wdt
AVREAL_FUSES=RSTDISBL=1,DWEN=1,WDTON=1,EESAVE=1,BODLEVEL=7,CKDIV8=1,CKOUT=1,SUT=1,CKSEL=0xf

#
# compiler settings
#
CC:=avr-gcc
AS:=avr-gcc
LD:=avr-ld
OBJCOPY:=avr-objcopy
AVRLIB:=/usr/lib/avr

#COMMON_FLAGS = -Xlinker -Tdata -Xlinker 0x800100
COMMON_FLAGS += -Os -Wall -mmcu=attiny25 -D__AVR_ATtiny25__
COMMON_FLAGS += $(addprefix -D,$(DEFINES))
ifdef NDEBUG
COMMON_FLAGS += -DNDEBUG=1
endif

ASFLAGS = $(COMMON_FLAGS)
ASFLAGS += -I$(AVRLIB)/include
ASFLAGS += -D__SFR_OFFSET=0

CFLAGS = $(COMMON_FLAGS)
CFLAGS += -I$(AVRLIB)/include
CFLAGS += -std=c99

LDFLAGS = $(COMMON_FLAGS)
LDFLAGS += -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -B$(AVRLIB)/lib

#
# build mechanics
#
AOBJECTS:=$(ASOURCES:.S=.o)
COBJECTS:=$(CSOURCES:.c=.o)
OBJECTS:=$(AOBJECTS) $(COBJECTS)

all: build
	avr-size -A -d $(TARGET).elf

build: $(TARGET).flash.hex $(TARGET).eeprom.hex

release:
	$(MAKE) NDEBUG=1 build

flash:
	/opt/bin/avreal -aft2232 +tiny45 -o250khz -e -w -c $(TARGET).flash.hex -v

flash-fuses:
	/opt/bin/avreal -aft2232 +tiny45 -o250khz -w -f$(AVREAL_FUSES) -v

flash-eeprom:
	/opt/bin/avreal -aft2232 +tiny45 -o250khz -w -d $(TARGET).eeprom.hex -v

$(TARGET).flash.hex: $(TARGET).elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse $< $@

$(TARGET).eeprom.hex: $(TARGET).elf
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O $(FORMAT) $< $@ || exit 0

$(TARGET).elf: $(OBJECTS)
	$(CC) $^ --output $@ $(LDFLAGS)

$(AOBJECTS):
%.o: %.S
	$(AS) -c $(ASFLAGS) $< -o $@

$(COBJECTS):
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

#
# host-based tests
#

TEST_EXECUTABLES=$(addprefix test_, $(TESTS))
HOSTLD=ld
HOSTCC=gcc
HOSTCFLAGS=-g -O0 -Wall -I./hostcompat -std=c99
HOSTLDFLAGS=

test: build_test

build_test: .tobj $(TEST_EXECUTABLES)

$(TEST_EXECUTABLES):
test_%: .tobj/test_%.o .tobj/%.o
	$(HOSTCC) -o $@ $(HOSTLDFLAGS) $^

.tobj/%.o: %.c
	$(HOSTCC) -c -o $@ $(HOSTCFLAGS) $^

.tobj:
	mkdir -p .tobj

#
# utility
#
.PHONY: clean build release all flash

clean:
	-rm -f $(TARGET).flash.hex $(TARGET).eeprom.hex $(TARGET).elf
	-rm -f $(OBJECTS)
