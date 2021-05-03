#include "usb_control.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/syscfg.h>

/**
 * Configure the 12MHz HSE to give a 48MHz PLL clock for the USB peripheral to function
 *
 * This is based on a clock configuration calculated in STM32 Cube. Note that
 * the STM32F070 requires an external oscillator for USB as its HSI is not
 * precise enough per the datasheet:
 *
 * > [USB] requires a precise 48 MHz clock which can be generated from the
 * > internal main PLL (the clock source must use an HSE crystal oscillator).
 */
static void clock_setup_12mhz_hse_out_48mhz(void)
{
    rcc_osc_on(RCC_HSE);
    rcc_wait_for_osc_ready(RCC_HSE);
    rcc_set_sysclk_source(RCC_HSE);

    rcc_set_hpre(RCC_CFGR_HPRE_NODIV);  // AHB Prescaler
    rcc_set_ppre(RCC_CFGR_PPRE_NODIV); // APB1 Prescaler

    flash_prefetch_enable();
    flash_set_ws(FLASH_ACR_LATENCY_024_048MHZ);

    // PLL for USB: (12MHz * 8) / 2 = 48MHz
    rcc_set_prediv(RCC_CFGR2_PREDIV_DIV2);
    rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_MUL8);
    rcc_set_pll_source(RCC_CFGR_PLLSRC_HSE_CLK);
    rcc_set_pllxtpre(RCC_CFGR_PLLXTPRE_HSE_CLK_DIV2);

    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);
    rcc_set_sysclk_source(RCC_PLL);

    rcc_set_usbclk_source(RCC_PLL);

    rcc_apb1_frequency = 48000000;
    rcc_ahb_frequency = 48000000;
}

int main(void)
{
    // Turn on the SYSCFG module and switch out PA9/PA10 for PA11/PA12
    // The USB peripheral is not phsyically connected without this and won't be enumerated
    RCC_APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;
    SYSCFG_CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;

    // Clock setup
    clock_setup_12mhz_hse_out_48mhz();
    rcc_periph_clock_enable(RCC_GPIOA);

    // Configure SysTick at 1ms intervals
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload((rcc_ahb_frequency/1000) - 1); // (48MHz/1000)
    systick_interrupt_enable();
    systick_counter_enable();

    // Configure switch input
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO6);

    usb_device_init();

    while (1) {
        // Behaviour is defined in usb_control.c
    }
}

/**
 * SysTick interrupt handler
 */
void sys_tick_handler(void)
{

}