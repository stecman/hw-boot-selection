#include "clock.h"
#include "usb.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#if defined(STM32F0)
#include <libopencm3/stm32/syscfg.h>
#endif

static usbd_device* init_usb(void)
{
    #if defined(STM32F070F6)
        // Turn on the SYSCFG module and switch out PA9/PA10 for PA11/PA12
        // The USB peripheral is not phsyically connected without this
        // and won't be enumerated
        RCC_APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;
        SYSCFG_CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;
    #endif

    #if defined(STM32F0)
        return usb_device_init(&st_usbfs_v2_usb_driver);
    #elif defined(STM32F1)
        return usb_device_init(&st_usbfs_v1_usb_driver);
    #else
        #error "USB driver not set for the current target"
    #endif
}

static void init_gpio(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);

    #if defined(STM32F1)
        // STM32 F1
        gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO6);
        gpio_set(GPIOA, GPIO6); // Enable internal pull-up
    #else
        // STM32 F0, F2, F3, F4
        gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO6);
    #endif
}

// (Called from usb.c)
char read_switch_value(void)
{
    return gpio_get(GPIOA, GPIO6) ? '1' : '0';
}

int main(void)
{
    init_clocks();
    init_gpio();

    usbd_device* usbd_dev = init_usb();

    while (1) {
        // Keep the USB device hardware fed
        // This can be done with interrupts, but the implementation differs
        // across devices so we're keeping this simple here.
        usbd_poll(usbd_dev);
    }
}