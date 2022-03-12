# Device to build for
# See libopencm3/ld/devices.data for valid device name patterns
DEVICE ?= stm32f103c4

# Programmer to use. This can be either:
#
# - The name of an interface config file in OpenOCD like "jlink" or "stlink"
#   (see the interfaces/*.cfg files installed by openocd for other options);
#
# OR
#
# - The special string "st-flash" to flash using stlink tools instead of OpenOCD
#   (Available at github.com/stlink-org/stlink)
#
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

# Configure OpenOCD if we're using that
ifneq ($(PROGRAMMER), st-flash)
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
endif


# Generate the actual build targets based on this what's configured above
OPENCM3_DIR=libopencm3
include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

# Extract parameters for size script from generated device flags
genlink_defs = $(shell $(OPENCM3_DIR)/scripts/genlink.py $(DEVICES_DATA) $(DEVICE) DEFS)
FLASH_SIZE_BYTES = $(shell python ./scripts/parse_defs.py _ROM $(genlink_defs))
SRAM_SIZE_BYTES = $(shell python ./scripts/parse_defs.py _RAM $(genlink_defs))


flash: all
ifeq ($(PROGRAMMER), st-flash)
	st-flash --reset write $(OUTPUT_BIN) 0x8000000
else
	BINARY=$(OUTPUT_ELF) $(OOCD) $(OOCD_FLAGS) -c 'program_and_run ()'
endif

# Some utility targets are only supported when using OpenOCD
ifneq ($(PROGRAMMER), st-flash)
flash_and_debug: all
	# Flash and halt, waiting for the debugger to attach
	BINARY=$(OUTPUT_ELF) $(OOCD) $(OOCD_FLAGS) -c 'program_and_attach ()'

reset:
	# Reset the target board
	$(OOCD) -f $(OOCD_FILE) -c 'reset ()'
endif

# Start a GDB debug server. Once started, used 'make debug_gdb' to connect in
# a separate terminal (openocd breaks if backgrounded within the Makefile)
debug_server:
ifeq ($(PROGRAMMER), st-flash)
	st-util --listen_port=3333
else
	$(OOCD) $(OOCD_FLAGS) -c 'attach ()'
endif

# Connect to a GDB server started with 'make debug_server'
debug_gdb:
	$(GDB) $(OUTPUT_ELF) -ex 'target remote :3333' \

disasm: $(OUTPUT_ELF)
	$(OBJDUMP) -d $(OUTPUT_ELF)

.PHONY: flash reset debug_gdb debug_server disasm
