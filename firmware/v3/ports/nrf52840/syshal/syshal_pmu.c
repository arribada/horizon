/**
  ******************************************************************************
  * @file     syshal_pmu.c
  * @brief    System hardware abstraction layer for sleep states.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2019 Arribada</center></h2>
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <https://www.gnu.org/licenses/>.
  *
  ******************************************************************************
  */

#include "syshal_pmu.h"
#include "nrfx_wdt.h"
#include "nrfx_power.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "syshal_firmware.h"
#include "syshal_flash.h"
#include "syshal_uart.h"
#include "syshal_i2c.h"
#include "syshal_spi.h"
#include "syshal_time.h"
#include "syshal_timer.h"
#include "syshal_rtc.h"
#include "bsp.h"
#include "debug.h"

#define WATCHDOG_PERIOD_MS ((uint32_t) 24 * 60 * 60 * 1000)

static nrfx_wdt_channel_id wdt_channel_id;
static uint32_t reset_reason;

static void wdt_event_handler(void)
{
    // We have roughly 50-60us in this interrupt before the device is reset
    syshal_rtc_stash_time();
    while (1)
    {}
}

static void nrfx_power_usb_event_handler(nrfx_power_usb_evt_t event)
{

}

void syshal_pmu_assert_callback(uint16_t line_num, const uint8_t *file_name)
{
    syshal_rtc_stash_time();
#ifdef DONT_RESTART_ASSERT
    DEBUG_PR_ERROR("Assertion %s:%u", file_name, line_num);
    while (1)
    {}
#else
    NVIC_SystemReset();
#endif
}

void syshal_pmu_init(void)
{
    sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);

    nrf_pwr_mgmt_init();

    sd_power_reset_reason_get(&reset_reason);

    // The reset reasons are non-volatile so they must be explicitly cleared
    sd_power_reset_reason_clr(0xFFFFFFFF);

    const nrfx_wdt_config_t config =
    {
        .behaviour          = NRF_WDT_BEHAVIOUR_RUN_SLEEP_HALT,
        .reload_value       = WATCHDOG_PERIOD_MS,
        .interrupt_priority = INTERRUPT_PRIORITY_WATCHDOG
    };

    nrfx_wdt_init(&config, wdt_event_handler);
    nrfx_wdt_channel_alloc(&wdt_channel_id);
    nrfx_wdt_enable();

    // Enable interrupts for USB detected/power_ready/removed events
    // This is mainly just to wake the device when these occur
#ifdef SOFTDEVICE_PRESENT
    if (nrf_sdh_is_enabled())
    {
        sd_power_usbdetected_enable(true);
        sd_power_usbpwrrdy_enable(true);
        sd_power_usbremoved_enable(true);
    }
    else
#endif
    {
        nrfx_power_usbevt_config_t nrfx_power_usbevt_config;
        nrfx_power_usbevt_config.handler = nrfx_power_usb_event_handler;

        nrfx_power_usbevt_init(&nrfx_power_usbevt_config);
        nrfx_power_usbevt_enable();
    }
}

/**
 * @brief      Sleep the microcontroller
 */
void syshal_pmu_sleep(syshal_pmu_sleep_mode_t mode)
{
    bool soft_wdt_running;
    int ret;

    for (uint32_t i = 0; i < 47; ++i)
        if (NVIC_GetPendingIRQ(i))
            DEBUG_PR_TRACE("Can't sleep as IRQ %lu pending", i);

    syshal_timer_recalculate_next_alarm(); // Determine when we need to wakeup next to service something

    switch (mode)
    {
        case SLEEP_DEEP:
            // We don't want our soft watchdog to run in deep sleep so disable
            ret = syshal_rtc_soft_watchdog_running(&soft_wdt_running);
            if (ret)
                soft_wdt_running = false;

            if (soft_wdt_running)
            {
                nrfx_rtc_disable(&RTC_Inits[RTC_SOFT_WATCHDOG].rtc);
                // Wait for the maximum amount of time it takes to disable the RTC
                // unfortunately there is no register we can poll to determine this
                syshal_time_delay_us(46);
            }

            syshal_flash_sleep(FS_DEVICE);

            // Disable all of our peripherals to reduce their current consumption
            for (uint32_t i = 0; i < SPI_TOTAL_NUMBER; ++i)
                syshal_spi_term(i);

            for (uint32_t i = 0; i < I2C_TOTAL_NUMBER; ++i)
                syshal_i2c_term(i);

            for (uint32_t i = 0; i < UART_TOTAL_NUMBER; ++i)
                syshal_uart_term(i);

            // We must set our cellular interface low to avoid power consumption
            nrf_gpio_cfg_input(UART_Inits[UART_CELLULAR].uarte_config.pseltxd, NRF_GPIO_PIN_PULLDOWN);
            nrf_gpio_cfg_input(UART_Inits[UART_CELLULAR].uarte_config.pselrxd, NRF_GPIO_PIN_PULLDOWN);
            nrf_gpio_cfg_input(UART_Inits[UART_CELLULAR].uarte_config.pselcts, NRF_GPIO_PIN_PULLDOWN);
            nrf_gpio_cfg_input(UART_Inits[UART_CELLULAR].uarte_config.pselrts, NRF_GPIO_PIN_PULLDOWN);

            // We must set our satellite interface low to avoid power consumption
            nrf_gpio_cfg_input(SPI_Inits[SPI_SATALLITE].spim_config.mosi_pin, NRF_GPIO_PIN_PULLDOWN);
            nrf_gpio_cfg_input(SPI_Inits[SPI_SATALLITE].spim_config.miso_pin, NRF_GPIO_PIN_PULLDOWN);
            nrf_gpio_cfg_input(SPI_Inits[SPI_SATALLITE].spim_config.sck_pin, NRF_GPIO_PIN_PULLDOWN);
            nrf_gpio_cfg_input(SPI_Inits[SPI_SATALLITE].spim_config.ss_pin, NRF_GPIO_PIN_PULLDOWN);

            syshal_time_term();

            // Sleep the device
            nrf_pwr_mgmt_run();

            syshal_time_init();

            for (uint32_t i = 0; i < UART_TOTAL_NUMBER; ++i)
                syshal_uart_init(i);

            for (uint32_t i = 0; i < SPI_TOTAL_NUMBER; ++i)
                syshal_spi_init(i);

            for (uint32_t i = 0; i < I2C_TOTAL_NUMBER; ++i)
                syshal_i2c_init(i);

            syshal_flash_wakeup(FS_DEVICE);

            if (soft_wdt_running)
                nrfx_rtc_enable(&RTC_Inits[RTC_SOFT_WATCHDOG].rtc);

            break;

        case SLEEP_LIGHT:
        default:
            nrf_pwr_mgmt_run();
            break;
    }
}

/**
 * @brief      Causes a software reset of the MCU
 */
__RAMFUNC void syshal_pmu_reset(void)
{
    syshal_rtc_stash_time();
    NVIC_SystemReset();
}

uint32_t syshal_pmu_get_startup_status(void)
{
    return reset_reason;
}

__RAMFUNC void syshal_pmu_kick_watchdog(void)
{
    nrf_wdt_reload_request_set(wdt_channel_id);
}
