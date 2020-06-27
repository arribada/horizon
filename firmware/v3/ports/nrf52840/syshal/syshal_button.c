/* syshal_button.c - HAL for implementing buttons
 *
 * Copyright (C) 2019 Arribada
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
 */

#include "bsp.h"
#include "syshal_button.h"
#include "syshal_gpio.h"
#include "nrfx_timer.h"
#include "app_util_platform.h" // CRITICAL_REGION def
#include "debug.h"

#define BUTTON_POLLING_PERIOD_US  (10000)
#define BUTTON_DEBOUNCE_PERIODS   (4)

static uint32_t button_pin;
static volatile uint32_t periods_elapsed;
static volatile bool button_press_confirmed;

#define GPIO_LOW  (false)
#define GPIO_HIGH (true)

static void syshal_button_interrupt_priv(syshal_gpio_event_t event)
{
    if (button_press_confirmed)
        return;


    periods_elapsed = 0;
    button_press_confirmed = false;

    nrfx_timer_clear(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer);
    if (!nrfx_timer_is_enabled(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer))
        nrfx_timer_enable(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer);
}

static void timer_evt_handler(nrf_timer_event_t event_type, void * p_context)
{
    periods_elapsed++;

    if (periods_elapsed > BUTTON_DEBOUNCE_PERIODS)
    {
        if ((syshal_gpio_get_input(button_pin) == GPIO_LOW) && !button_press_confirmed)
        {
            DEBUG_PR_TRACE("SYSHAL_BUTTON_PRESSED");
            periods_elapsed = 0;
            button_press_confirmed = true;

            syshal_button_event_t event;
            event.id = SYSHAL_BUTTON_PRESSED;
            syshal_button_callback(event);
        }
        else if ((syshal_gpio_get_input(button_pin) == GPIO_HIGH) && button_press_confirmed)
        {
            nrfx_timer_disable(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer);

            button_press_confirmed = false;
            syshal_button_event_t event;
            event.id = SYSHAL_BUTTON_RELEASED;
            event.released.duration_ms = periods_elapsed * BUTTON_POLLING_PERIOD_US / 1000;
            DEBUG_PR_TRACE("SYSHAL_BUTTON_RELEASED, held for %lu", event.released.duration_ms);
            syshal_button_callback(event);
        }
        else if ((syshal_gpio_get_input(button_pin) == GPIO_HIGH) && !button_press_confirmed)
        {
            nrfx_timer_disable(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer);
        }
    }
}

int syshal_button_init(uint32_t pin)
{
    button_pin = pin;
    nrfx_timer_config_t timer_config =
    {
        .frequency          = NRF_TIMER_FREQ_1MHz,
        .mode               = NRF_TIMER_MODE_TIMER,
        .bit_width          = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = TIMER_Inits[TIMER_BUTTON_DEBOUNCE].irq_priority,
        .p_context          = NULL
    };

    button_press_confirmed = false;
    // Setup a timer to fire every BUTTON_POLLING_PERIOD_US
    nrfx_timer_extended_compare(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer, NRF_TIMER_CC_CHANNEL0, BUTTON_POLLING_PERIOD_US, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrfx_timer_init(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer, &timer_config, timer_evt_handler);

    syshal_gpio_init(button_pin);
    syshal_gpio_enable_interrupt(button_pin, syshal_button_interrupt_priv);

    return SYSHAL_BUTTON_NO_ERROR;
}

int syshal_button_term(uint32_t pin)
{

    if (pin != button_pin)
        return SYSHAL_BUTTON_ERROR_NOT_INITIALISED;

    // NOTE: We want to be careful that either interrupt does not fire when either the timer or gpio is in an unknown state
    CRITICAL_REGION_ENTER();
    syshal_gpio_term(button_pin);
    nrfx_timer_uninit(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer);
    CRITICAL_REGION_EXIT();

    button_pin = 0;

    return SYSHAL_BUTTON_NO_ERROR;
}

__attribute__((weak)) void syshal_button_callback(syshal_button_event_t event)
{
    DEBUG_PR_WARN("%s() Not implemented", __FUNCTION__);
}