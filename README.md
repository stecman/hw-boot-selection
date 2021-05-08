# GRUB boot selector switch

Firmware for the [Hardware Boot Selection Switch](https://hackaday.io/project/179539-hardware-boot-selection-switch) project: This acts as a USB mass storage device containing a dynamic GRUB script, which sets a varible to indicate the position of the physical switch.


## Building

On Linux, you'll need `gcc-arm-none-eabi` installed to build and `openocd` to flash.

```sh
# Pull in libopencm3
git submodule init
git submodule update

# Build libopencm3 (only needed once)
make -C libopencm3 -j8

# Build the firmware
make

# Flash with a J-Link programmer connected by SWD
make flash

# Flash with an ST-Link programmer connected by SWD
make flash PROGRAMMER=stlink
```

This project supports building and flashing multiple different parts. These parts have been tested:

- `stm32f103c8` (default): found on the "blue pill" and "maple" STM32 dev boards
- `stm32f070f6`

To build for your specific part, pass the `DEVICE` parameter to `make` on the command line (or export `DEVICE` as an environment variable):

```sh
make DEVICE=stm32f070f6
make flash DEVICE=stm32f070f6
```

Valid patterns for the `DEVICE` parameter can be found in `libopencm3/ld/devices.data`.


## GRUB config

You'll need to modify your system's GRUB config to look for and read from this device. Here's an example of how this can be done in GRUB script:

```sh
# Look for hardware switch device by its hard-coded filesystem ID
search --no-floppy --fs-uuid --set hdswitch 55AA-6922

# If found, read dynamic config file and select appropriate entry for each position
if [ "${hdswitch}" ] ; then
  source ($hdswitch)/switch_position_grub.cfg

  if [ "${os_hw_switch}" == 0 ] ; then
    # Boot Linux
    set default="0"
  elif [ "${os_hw_switch}" == 1 ] ; then
    # Boot Windows
    set default="2"
  else
    # Fallback to default
    set default="${GRUB_DEFAULT}"
  fi

else
  set default="${GRUB_DEFAULT}"
fi
```