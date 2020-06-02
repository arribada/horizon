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
#include "nrfx_timer.h"

#define HIGH  (true)
#define LOW   (false)

static volatile uint64_t m_pin_state;
static volatile uint64_t m_pin_transition;

static bool timer_init = false;

static struct
{
    bool current_state;
    bool is_init;
    void (*callback)(const syshal_switch_state_t*);
} switch_state[SWITCH_TOTAL_NUMBER];

static inline uint32_t switch_pin_number(uint32_t inst)
{
    return GPIO_Inits[Switch_Inits[inst].gpio_inst].pin_number;
}

static syshal_switch_state_t apply_active_low_opt(uint32_t instance, bool state) {
    if (Switch_Inits[instance].active_low)
        state = !state;

    if (state)
        return SYSHAL_SWITCH_EVENT_CLOSED;
    else
        return SYSHAL_SWITCH_EVENT_OPEN;
}

static void timer_evt_handler(nrf_timer_event_t event_type, void * p_context)
{
    // Change detected, execute switch handler(s)
    for (uint32_t i = 0; i < SWITCH_TOTAL_NUMBER; i++)
    {
        if (switch_state[i].is_init)
        {
            uint64_t switch_mask = 1ULL << switch_pin_number(i);
            if (switch_mask & m_pin_transition)
            {
                m_pin_transition &= ~switch_mask;
                bool pin_is_set = syshal_gpio_get_input(Switch_Inits[i].gpio_inst);
                if ((m_pin_state & (1ULL << switch_pin_number(i))) == (((uint64_t)pin_is_set) << switch_pin_number(i)))
                {
                    syshal_switch_state_t state = apply_active_low_opt(i, pin_is_set);

                    if (switch_state[i].callback)
                        switch_state[i].callback(&state);
                }
            }
        }
    }
}

static uint32_t get_switch_instance(uint32_t pin)
{
    for (uint32_t i = 0; i < SWITCH_TOTAL_NUMBER; ++i)
        if (switch_pin_number(i) == pin)
            return i;
    
    DEBUG_PR_ERROR("Switch instance not found");
    return 0;
}

/**
 * @brief      Interrupt handler for when a switch has been triggered
 */
static void syshal_switch_interrupt_priv(const syshal_gpio_event_t *event)
{
    uint64_t pin_mask = 1ULL << event->pin_number;

    // Stop the detection timer to prevent us being interrupted
    nrfx_timer_disable(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer);

    const Switch_InitTypeDefAndInst_t *init = &Switch_Inits[get_switch_instance(event->pin_number)];

//    if (!(m_pin_transition & pin_mask))
//    {
        if (syshal_gpio_get_input(init->gpio_inst))
            m_pin_state |= pin_mask;
        else
            m_pin_state &= ~(pin_mask);

        m_pin_transition |= (pin_mask);

        // Start the detection timer
        nrfx_timer_compare(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer, NRF_TIMER_CC_CHANNEL0, init->debounce_time_us, true);
        nrfx_timer_clear(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer);
        nrfx_timer_enable(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer);
/*    }
    else
    {
        m_pin_transition &= ~pin_mask;
    }*/
}

int syshal_switch_init(uint32_t instance, void (*callback_function)(const syshal_switch_state_t*))
{
    if (instance >= SWITCH_TOTAL_NUMBER)
        return SYSHAL_SWITCH_ERROR_INVALID_INSTANCE;

    if (!timer_init)
    {
        const nrfx_timer_config_t timer_config =
        {
            .frequency          = NRF_TIMER_FREQ_1MHz,
            .mode               = NRF_TIMER_MODE_TIMER,
            .bit_width          = NRF_TIMER_BIT_WIDTH_32,
            .interrupt_priority = TIMER_Inits[TIMER_BUTTON_DEBOUNCE].irq_priority,
            .p_context          = NULL
        };
        nrfx_timer_init(&TIMER_Inits[TIMER_BUTTON_DEBOUNCE].timer, &timer_config, timer_evt_handler);
    }

    switch_state[instance].callback = callback_function;
    switch_state[instance].is_init = true;

    syshal_gpio_init(Switch_Inits[instance].gpio_inst);
    syshal_gpio_enable_interrupt(Switch_Inits[instance].gpio_inst, syshal_switch_interrupt_priv);

    return SYSHAL_SWITCH_NO_ERROR;
}

int syshal_switch_get(uint32_t instance, syshal_switch_state_t *state)
{
    if (instance >= SWITCH_TOTAL_NUMBER)
        return SYSHAL_SWITCH_ERROR_INVALID_INSTANCE;

    *state = apply_active_low_opt(instance, syshal_gpio_get_input(Switch_Inits[instance].gpio_inst));

    return SYSHAL_SWITCH_NO_ERROR;
}
