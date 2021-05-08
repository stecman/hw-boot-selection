# Device to build for
# See libopencm3/ld/devices.data for valid device name patterns
DEVICE ?= stm32f103c8

# Parameters for firmware size script
FLASH_SIZE_BYTES = 32768
SRAM_SIZE_BYTES = 6144

# Serial number for USB interface (must be 12 or more characters in hex)
SERIALNUM ?= D000000000E7
CDEFS += -DUSB_SERIALNUM="\"$(SERIALNUM)\""

# Base OpenCM3 build
PROJECT = boot-swtich
BUILD_DIR = build/$(DEVICE)

SHARED_DIR = ../common
CFILES = \
	clock.c \
	fat.c \
	main.c \
	usb.c \

CSTD = -std=c11
OPT = -Os

# Flag if specific devices that needs special pin mapping
ifeq ($(DEVICE), stm32f070f6)
	CDEFS += -DSTM32F070F6
endif

VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

# Custom tasks

OOCD_FILE = ../openocd-jlink.cfg

reset:
	openocd -f $(OOCD_FILE) -c 'reset ()'

flash: all
	BINARY=$(PROJECT).elf openocd -f $(OOCD_FILE) -c 'program_and_run ()'

flash_and_debug: all
	BINARY=$(PROJECT).elf openocd -f $(OOCD_FILE) -c 'program_and_attach ()'

debug_server:
	# This doesn't like being backgrounded from make for some reason
	# Run this as a background task, then run debug_gdb
	openocd -f $(OOCD_FILE) -c 'attach ()'

debug_gdb:
	$(GDB) $(PROJECT).elf -ex 'target remote :3333' \

disasm: $(PROJECT).elf
	$(OBJDUMP) -d $(PROJECT).elf

.PHONY: flash reset debug_gdb debug_server disasm