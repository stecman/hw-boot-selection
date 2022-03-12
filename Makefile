# Device to build for
# See libopencm3/ld/devices.data for valid device name patterns
DEVICE ?= stm32f103c4

# Programmer to use (name of interface config in OpenOCD)
# For ST-Link v1 or v2 programmers, use "stlink"
# See the interfaces/*.cfg files installed by openocd for options
PROGRAMMER ?= jlink

# Serial number for USB interface (must be 12 or more characters in hex)
SERIALNUM ?= D000000000E7
CDEFS += -DUSB_SERIALNUM="\"$(SERIALNUM)\""

# Base OpenCM3 build
PROJECT = boot-switch

CFILES = \
	src/clock.c \
	src/fat.c \
	src/main.c \
	src/usb.c \

CSTD = -std=c11
OPT = -Os

# Flag specific devices that needs special pin mapping
ifeq ($(DEVICE), stm32f070f6)
	CDEFS += -DSTM32F070F6
endif


# Pick OpenOCD target config based on DEVICE parameter if not set externally
OOCD_TARGET ?= $(shell python ./scripts/openocd_target.py $(DEVICE))

# Pick appropriate transport for programmer
ifeq ($(PROGRAMMER), stlink)
	TRANSPORT ?= hla_swd
else
	TRANSPORT ?= swd
endif

OOCD_FLAGS = -f interface/$(PROGRAMMER).cfg \
             -c "transport select $(TRANSPORT)" \
             -f target/$(OOCD_TARGET).cfg \
             -f openocd.cfg


# Generate the actual build targets based on this what's configured above
OPENCM3_DIR=libopencm3
include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

# Extract parameters for size script from generated device flags
genlink_defs = $(shell $(OPENCM3_DIR)/scripts/genlink.py $(DEVICES_DATA) $(DEVICE) DEFS)
FLASH_SIZE_BYTES = $(shell python ./scripts/parse_defs.py _ROM $(genlink_defs))
SRAM_SIZE_BYTES = $(shell python ./scripts/parse_defs.py _RAM $(genlink_defs))


reset:
	$(OOCD) -f $(OOCD_FILE) -c 'reset ()'

flash: all
	BINARY=$(OUTPUT_ELF) $(OOCD) $(OOCD_FLAGS) -c 'program_and_run ()'

flash_and_debug: all
	BINARY=$(OUTPUT_ELF) $(OOCD) $(OOCD_FLAGS) -c 'program_and_attach ()'

debug_server:
	# This doesn't like being backgrounded from make for some reason
	# Run this as a background task, then run debug_gdb
	$(OOCD) $(OOCD_FLAGS) -c 'attach ()'

debug_gdb:
	$(GDB) $(OUTPUT_ELF) -ex 'target remote :3333' \

disasm: $(OUTPUT_ELF)
	$(OBJDUMP) -d $(OUTPUT_ELF)

.PHONY: flash reset debug_gdb debug_server disasm
