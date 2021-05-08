#pragma once

#include <libopencm3/usb/usbd.h>

/**
 * Setup and start the USB peripheral
 */
usbd_device* usb_device_init(const usbd_driver* driver);

/**
 * External hook for getting an ASCII character to represent the switch position
 */
extern char read_switch_value(void);
