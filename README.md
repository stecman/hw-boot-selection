# GRUB boot selector switch

Firmware for the [Hardware Boot Selection Switch](https://hackaday.io/project/179539-hardware-boot-selection-switch) project. This acts as a USB mass storage device containing a dynamic GRUB script, which sets a variable to indicate the position of a physical switch.


## Building

On Linux, you'll need `gcc-arm-none-eabi` and `python` installed to build and `openocd` to flash.

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

This project supports building and flashing multiple different parts. The following parts have been tested:

- `stm32f103c4` (default): supports all variants of the F103 chip found on the "blue pill" and "maple" STM32 dev boards.
- `stm32f103c6`
- `stm32f103c8`
- `stm32f070f6`

To build for your specific part, pass the `DEVICE` parameter to `make` on the command line (or export `DEVICE` as an environment variable): 

```sh
make DEVICE=stm32f070f6
make flash DEVICE=stm32f070f6
```

Valid patterns for the `DEVICE` parameter can be found in `libopencm3/ld/devices.data`. If you are adding custom functionality, specify your exact variant to have access to the full RAM and flash; the default is the lowest spec F103 part.

## Wiring

The switch position is read from pin A6, which is pulled high internally. Connect a toggle switch between this pin and GND to control the reported state.


## GRUB config

You'll need to modify your system's GRUB configuration to look for and read from this device during boot.

On Debian and Arch based systems at least, this can be done non-destructively by creating an additional script in `/etc/grub.d/`:

1. Create `/etc/grub.d/01_bootswitch` with the following contents:
  
  ```sh
  #! /bin/sh

  cat << 'EOF'
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
    fi

  fi
  EOF
  ```

2. Make it executable: `chmod +x /etc/grub.d/01_bootswitch`

3. Run `update-grub` to generate and validate the `/boot/grub/grub.cfg` file that's used when booting.


This overrides the `default` variable set in `00_header`. If the switch is not found, your existing default selection will be used.

You may need to change the values on the `set default=` lines if you have a different arrangement of choices in your GRUB menu.

## Troubleshooting

### The device is not accessible in GRUB

Ensure your BIOS is configured to enumerate USB storage devices. On some motherboards, this requires changing from "fast boot" to "normal boot". If you can boot from a flash drive, you should be able to access this device from GRUB.

### Reading from an operating system

The dynamic files presented by this device cannot be reliably used when mounted in Windows or Linux. Operating systems (quite rightly) assume that the files on an underlying device won't change without their knowledge, so they cache reads heavily.

If you want to access the switch state from an operating system, you'll need to implement a second USB interface on the device, or work with the unmounted block device.
