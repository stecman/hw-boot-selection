# GRUB boot selector switch

Firmware for the [Hardware Boot Selection Switch](https://hackaday.io/project/179539-hardware-boot-selection-switch) project: This acts as a USB mass storage device containing a dynamic GRUB script, which sets a varible to indicate the position of the physical switch.

## Building

```sh
# Pull in libopencm3
git submodule init
git submodule update

# Build
cd src
make

# Flash
make flash
```

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