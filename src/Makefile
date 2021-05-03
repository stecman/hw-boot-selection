# Serial number for USB interface (mustt be 12 or more characters in hex)
SERIALNUM ?= DEFEA109FE7A
CDEFS = -DUSB_SERIALNUM="\"$(SERIALNUM)\""

# Parameters for firmware size script
FLASH_SIZE_BYTES = 32768
SRAM_SIZE_BYTES = 6144

# Base OpenCM3 build
PROJECT = boot-swtich
BUILD_DIR = build

SHARED_DIR = ../common
CFILES = \
	fat.c \
	main.c \
	usb_control.c \

SERIALNUM ?= DEVELOPMENT

CSTD = -std=c11
OPT = -Os

DEVICE = stm32f070f6

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
	arm-none-eabi-gdb $(PROJECT).elf -ex 'target remote :3333'

disasm: $(PROJECT).elf
	$(OBJDUMP) -d $(PROJECT).elf

.PHONY: flash reset debug_gdb debug_server disasm