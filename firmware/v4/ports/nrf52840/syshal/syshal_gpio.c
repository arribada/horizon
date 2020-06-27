/**
  ******************************************************************************
  * @file     syshal_gpio.c
  * @brief    System hardware abstraction layer for GPIO.
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

#include "syshal_gpio.h"
#include "nrf_gpio.h"
#include "bsp.h"
#include <stdint.h>
#include <stdbool.h>
#include "syshal_firmware.h"

typedef struct
{
    uint32_t pin_number;
    void (*callback)(const syshal_gpio_event_t*);
} syshal_gpio_irq_conf_t;

static syshal_gpio_irq_conf_t syshal_gpio_irq_conf[GPIOTE_CH_NUM];

void nrfx_gpiote_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    for (uint32_t ch = 0; ch < GPIOTE_CH_NUM; ++ch)
    {
        if (syshal_gpio_irq_conf[ch].pin_number == pin)
        {
            if (syshal_gpio_irq_conf[ch].callback)
            {
                syshal_gpio_event_t event;
                event.pin_number = pin;

                if (NRF_GPIOTE_POLARITY_LOTOHI)
                    event.id = SYSHAL_GPIO_EVENT_LOW_TO_HIGH;
                else if (NRF_GPIOTE_POLARITY_HITOLO)
                    event.id = SYSHAL_GPIO_EVENT_HIGH_TO_LOW;
                else if (NRF_GPIOTE_POLARITY_TOGGLE)
                    event.id = SYSHAL_GPIO_EVENT_TOGGLE;
                
                syshal_gpio_irq_conf[ch].callback(&event);
            }
            break;
        }
    }
}

int syshal_gpio_init(uint32_t pin)
{
    nrf_gpio_cfg(GPIO_Inits[pin].pin_number,
                 GPIO_Inits[pin].dir,
                 GPIO_Inits[pin].input,
                 GPIO_Inits[pin].pull,
                 GPIO_Inits[pin].drive,
                 GPIO_Inits[pin].sense);

    return SYSHAL_GPIO_NO_ERROR;
}

int syshal_gpio_term(uint32_t pin)
{
    for (uint32_t ch = 0; ch < GPIOTE_CH_NUM; ++ch)
        if (syshal_gpio_irq_conf[ch].pin_number == pin)
            syshal_gpio_disable_interrupt(pin);

    return SYSHAL_GPIO_NO_ERROR;
}

int syshal_gpio_enable_interrupt(uint32_t pin, void (*callback_function)(const syshal_gpio_event_t*))
{
    if (!nrfx_gpiote_is_init())
        nrfx_gpiote_init();

    // Make sure we have a free GPIOTE channel to connect this pin to
    // There is a maximum of 8 in the nRF52480
    for (uint32_t ch = 0; ch < GPIOTE_CH_NUM; ++ch)
    {
        if (!syshal_gpio_irq_conf[ch].callback)
        {
            syshal_gpio_irq_conf[ch].pin_number = GPIO_Inits[pin].pin_number;
            syshal_gpio_irq_conf[ch].callback = callback_function;
            goto success;
        }
    }

    return SYSHAL_GPIO_NO_FREE_INTERRUPT;

success:

    nrfx_gpiote_in_init(GPIO_Inits[pin].pin_number,
                        &GPIO_Inits[pin].gpiote_in_config,
                        nrfx_gpiote_evt_handler);

    nrfx_gpiote_in_event_enable(GPIO_Inits[pin].pin_number, true);

    return SYSHAL_GPIO_NO_ERROR;
}

int syshal_gpio_disable_interrupt(uint32_t pin)
{
    for (uint32_t ch = 0; ch < GPIOTE_CH_NUM; ++ch)
    {
        if (syshal_gpio_irq_conf[ch].pin_number == GPIO_Inits[pin].pin_number)
        {
            nrfx_gpiote_in_event_disable(GPIO_Inits[pin].pin_number);
            nrfx_gpiote_in_uninit(GPIO_Inits[pin].pin_number);

            syshal_gpio_irq_conf[ch].pin_number = 0;
            syshal_gpio_irq_conf[ch].callback = NULL;
            break;
        }
    }

    return SYSHAL_GPIO_NO_ERROR;
}

__RAMFUNC int syshal_gpio_set_output_low(uint32_t pin)
{
    nrf_gpio_pin_clear(GPIO_Inits[pin].pin_number);
    return SYSHAL_GPIO_NO_ERROR;
}

__RAMFUNC int syshal_gpio_set_output_high(uint32_t pin)
{
    nrf_gpio_pin_set(GPIO_Inits[pin].pin_number);
    return SYSHAL_GPIO_NO_ERROR;
}

int syshal_gpio_set_output_toggle(uint32_t pin)
{
    nrf_gpio_pin_toggle(GPIO_Inits[pin].pin_number);
    return SYSHAL_GPIO_NO_ERROR;
}

bool syshal_gpio_get_input(uint32_t pin)
{
    return nrf_gpio_pin_read(GPIO_Inits[pin].pin_number);
}