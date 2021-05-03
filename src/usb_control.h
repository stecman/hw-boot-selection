#pragma once

#include <libopencm3/usb/usbd.h>

#include <stdint.h>

/**
 * Setup and start the USB peripheral
 */
void usb_device_init(void);

extern void usb_set_config_callback(usbd_device *usbd_dev, uint16_t wValue);