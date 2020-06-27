/* syshal_switch.c - HAL for saltwater switch
 *
 * Copyright (C) 2018 Arribada
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
#include "syshal_switch.h"
#include "syshal_time.h"
#include "syshal_gpio.h"
#include "debug.h"

/**
 * @brief      Interrupt handler for when switch has been triggered
 */
static void syshal_switch_interrupt_priv(syshal_gpio_event_t event)
{
    // Generate a callback event
    syshal_switch_event_id_t switch_event;
//    static uint32_t last_event = 0;
    static uint8_t last_state = 2;

//    if (syshal_time_get_ticks_ms() > (last_event + SYSHAL_SWITCH_DEBOUNCE_TIME_MS))
//    {
    bool state = syshal_switch_get();
    if (last_state != state)
    {
        if (state)
        {
            switch_event = SYSHAL_SWITCH_EVENT_CLOSED;
            DEBUG_PR_SYS("Saltwater Switch Closed");
        }
        else
        {
            switch_event = SYSHAL_SWITCH_EVENT_OPEN;
            DEBUG_PR_SYS("Saltwater Switch Open");
        }

        syshal_switch_callback(switch_event);

//        last_event = syshal_time_get_ticks_ms();
        last_state = state;
    }
//    }
}

int syshal_switch_init(void)
{
    syshal_gpio_init(GPIO_SWS);
    syshal_gpio_enable_interrupt(GPIO_SWS, syshal_switch_interrupt_priv);

    return SYSHAL_SWITCH_NO_ERROR;
}

inline bool syshal_switch_get(void)
{
    return !syshal_gpio_get_input(GPIO_SWS);
}

/**
 * @brief      Saltwater Switch callback stub, should be overriden by the user
 *             application
 *
 * @param[in]  event  The event that occured
 */
__attribute__((weak)) void syshal_switch_callback(syshal_switch_event_id_t event)
{
    DEBUG_PR_WARN("%s() Not implemented", __FUNCTION__);
}