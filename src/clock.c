#include "clock.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>

/**
 * These are based on clock configurations calculated in STM32CubeMX
 * A number of parts including the STM32F070 and STM32F103 require
 * an external oscillator for USB as the internal oscillator (HSI)
 * is not precise enough:
 *
 * > [USB] requires a precise 48 MHz clock which can be generated from the
 * > internal main PLL (the clock source must use an HSE crystal oscillator).
 */

#if defined(STM32F0)

    /**
     * Configure 12MHz HSE to give a 48MHz PLL clock required for the USB peripheral
     * (Tested on stm32f070)
     */
    void init_clocks(void)
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

#elif defined(STM32F1)

    /**
     * Configure 8MHz HSE to give a 48MHz PLL clock required for the USB peripheral
     *
     * You can alternatively use rcc_clock_setup_in_hsi_out_48mhz(), though running
     * the USB peripheral from the internal oscillator may not operate in spec for USB
     *
     * (Tested on stm32f103)
     */
    void init_clocks(void)
    {
        // Enable internal high-speed oscillator
        rcc_osc_on(RCC_HSI);
        rcc_wait_for_osc_ready(RCC_HSI);
        rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_HSICLK);

        // Enable external 8MHz oscillator
        rcc_osc_on(RCC_HSE);
        rcc_wait_for_osc_ready(RCC_HSE);
        rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_HSECLK);

        rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV4); // ADC Prescaler
        rcc_set_hpre(RCC_CFGR_HPRE_SYSCLK_NODIV);  // AHB Prescaler
        rcc_set_ppre1(RCC_CFGR_PPRE1_HCLK_DIV2); // APB1 Prescaler
        rcc_set_ppre2(RCC_CFGR_PPRE2_HCLK_NODIV); // APB2 Prescaler
        rcc_set_usbpre(RCC_CFGR_USBPRE_PLL_CLK_NODIV); // USB Prescaler

        // Sysclk runs with 48MHz -> 1 waitstates.
        // 0WS from 0-24MHz
        // 1WS from 24-48MHz
        // 2WS from 48-72MHz
        flash_prefetch_enable();
        flash_set_ws(FLASH_ACR_LATENCY_1WS);

        // PLL for USB: 8MHz * 6 = 48MHz
        rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_PLL_CLK_MUL6);
        rcc_set_pll_source(RCC_CFGR_PLLSRC_HSE_CLK);
        rcc_set_pllxtpre(RCC_CFGR_PLLXTPRE_HSE_CLK);

        rcc_osc_on(RCC_PLL);
        rcc_wait_for_osc_ready(RCC_PLL);
        rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_PLLCLK);

        // Set the peripheral clock frequencies used
        rcc_ahb_frequency = 48000000;
        rcc_apb1_frequency = 24000000;
        rcc_apb2_frequency = 48000000;
    }

#else
#    error "Clock configuration for the target device is not defined"
#endif