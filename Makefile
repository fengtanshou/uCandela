#
# parameters
#
TARGET:=sensor
FORMAT:=ihex
CPU=attiny45
DEFINES = F_CPU=12000000
FEAT_WITH_USB ?= yes
FEAT_WITH_SERIAL ?= no
FEAT_USB_DRIVER ?= vusb
include Makefile.features

#
# settings
#

#
# (CKDIV=0) start with clk/8
# (CKOUT=1) disable clock out
# (CKSEL=2) internal rc osc
# (SUT=0) start up time - 14CK
# (BODLEVEL=6) set BOD to 1v8
# (WDTON=1) do not use wdt
AVREAL_FUSES=RSTDISBL=1,DWEN=1,WDTON=1,EESAVE=1,BODLEVEL=7,CKDIV8=1,CKOUT=1,SUT=1,CKSEL=0xf
CC:=avr-gcc
AS:=avr-gcc
LD:=avr-ld
OBJCOPY:=avr-objcopy
AVRLIB:=/usr/lib/avr
AVREAL=/opt/bin/avreal
AVREAL_OPTS = -aft2232:enable=~acbus1 -o100khz +$(CPU)

#COMMON_FLAGS = -Xlinker -Tdata -Xlinker 0x800100
COMMON_FLAGS += -Os -Wall -mmcu=$(CPU)
COMMON_FLAGS += $(addprefix -D,$(DEFINES))
ifdef NDEBUG
COMMON_FLAGS += -DNDEBUG=1
else
COMMON_FLAGS += -g
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

hex: $(TARGET).flash.hex $(TARGET).eeprom.hex

build: $(TARGET).elf
	avr-size -A -d $^

release:
	$(MAKE) NDEBUG=1 build

flash: AVREAL_COMMAND=-e -w -c $(TARGET).flash.hex -v
flash: $(TARGET).flash.hex __avreal

flash-fuses: AVREAL_COMMAND=-w -f$(AVREAL_FUSES) -v
flash-fuses: __avreal

flash-eeprom: AVREAL_COMMAND=-w -d $(TARGET).eeprom.hex -v
flash-eeprom: $(TARGET).eeprom.hex __avreal

eeread: AVREAL_COMMAND=-r -d _read.eeprom.hex
eeread: __avreal
	$(OBJCOPY) -I ihex -O binary _read.eeprom.hex _read.eeprom.bin && hexdump -C _read.eeprom.bin

__avreal:
	$(AVREAL) $(AVREAL_OPTS) $(AVREAL_COMMAND)

$(TARGET).flash.hex: $(TARGET).elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse -R .noinit $< $@

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
# utility
#
.PHONY: clean build release all flash

clean:
	-rm -f $(TARGET).flash.hex $(TARGET).eeprom.hex $(TARGET).elf
	-rm -f $(OBJECTS)
