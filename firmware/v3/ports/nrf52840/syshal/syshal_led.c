/* syshal_led.c - HAL for LED
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
#include "syshal_led.h"
#include "bsp.h"
#include "syshal_time.h"
#include "nrfx_pwm.h"
#include "app_util_platform.h" // CRITICAL_REGION def

#define SYSHAL_LED_ON (1)
#define SYSHAL_LED_OFF (0)

#define LED_BLINK_POOL_US   (1000) // Every 1 ms
#define TICKS_PER_OVERFLOW  (16777216) // 16777216 = 2 ^ 24
#define MAX_VALUE           (TICKS_PER_OVERFLOW / COUNT_1MS) // 134 seconds

#define UINT32_COLOUR_TO_UINT15_RED(x)   (((x >> 16) & 0xFF) << 7)
#define UINT32_COLOUR_TO_UINT15_GREEN(x) (((x >> 8) & 0xFF) << 7)
#define UINT32_COLOUR_TO_UINT15_BLUE(x)  ((x & 0xFF) << 7)

static enum
{
    SOLID,
    BLINK,
    SEQUENCE,
    OFF,
} current_type;

static volatile uint32_t current_colour = SYSHAL_LED_COLOUR_OFF;
static volatile uint8_t last_state;
static syshal_led_sequence_t current_sequence;
static nrf_pwm_values_individual_t pwm_values;
static uint32_t start_blink_time_ms;
static uint32_t blink_period_ms;

static inline void set_colour(uint32_t colour)
{
    if (!colour)
    {
        if (!nrfx_pwm_is_stopped(&PWM_Inits[PWM_LED].pwm))
        {
            nrfx_pwm_stop(&PWM_Inits[PWM_LED].pwm, true);
            // Set the pins to high impedance to reduce current draw
            nrf_gpio_cfg_default(GPIO_Inits[GPIO_LED_RED].pin_number);
            nrf_gpio_cfg_default(GPIO_Inits[GPIO_LED_GREEN].pin_number);
            nrf_gpio_cfg_default(GPIO_Inits[GPIO_LED_BLUE].pin_number);
        }
        return;
    }

    if (nrfx_pwm_is_stopped(&PWM_Inits[PWM_LED].pwm))
    {
        // Set our pins back to being outputs
        nrf_gpio_cfg_output(GPIO_Inits[GPIO_LED_RED].pin_number);
        nrf_gpio_cfg_output(GPIO_Inits[GPIO_LED_GREEN].pin_number);
        nrf_gpio_cfg_output(GPIO_Inits[GPIO_LED_BLUE].pin_number);
    }

    pwm_values.channel_0 = UINT32_COLOUR_TO_UINT15_RED(colour);
    pwm_values.channel_1 = UINT32_COLOUR_TO_UINT15_GREEN(colour);
    pwm_values.channel_2 = UINT32_COLOUR_TO_UINT15_BLUE(colour);

    nrf_pwm_sequence_t const pwm_sequence =
    {
        .values.p_individual = &pwm_values,
        .length              = NRF_PWM_VALUES_LENGTH(pwm_values),
        .repeats             = 0,
        .end_delay           = 0
    };

    nrfx_pwm_simple_playback(&PWM_Inits[PWM_LED].pwm, &pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);
}

int syshal_led_init(void)
{
    // Setup the LED pwm instance
    const nrfx_pwm_config_t pwm_config =
    {
        .output_pins  = {
            GPIO_Inits[GPIO_LED_RED].pin_number   | NRFX_PWM_PIN_INVERTED,
            GPIO_Inits[GPIO_LED_GREEN].pin_number | NRFX_PWM_PIN_INVERTED,
            GPIO_Inits[GPIO_LED_BLUE].pin_number  | NRFX_PWM_PIN_INVERTED,
            NRFX_PWM_PIN_NOT_USED,
        },
        .irq_priority = PWM_Inits[PWM_LED].irq_priority,
        .base_clock   = NRF_PWM_CLK_16MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = 0x7FFF, // 15 bit counter
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode    = NRF_PWM_STEP_AUTO,
    };

    nrfx_pwm_init(&PWM_Inits[PWM_LED].pwm, &pwm_config, NULL);

    return SYSHAL_LED_NO_ERROR;
}

int syshal_led_set_solid(uint32_t colour)
{
    current_colour = colour;
    current_type = SOLID;
    set_colour(colour);
    return SYSHAL_LED_NO_ERROR;
}

int syshal_led_set_blinking(uint32_t colour, uint32_t time_ms)
{
    current_colour = colour;
    current_type = BLINK;

    last_state = SYSHAL_LED_ON;

    blink_period_ms = time_ms;
    start_blink_time_ms = syshal_time_get_ticks_ms();
    set_colour(colour);

    return SYSHAL_LED_NO_ERROR;
}

int syshal_led_get(uint32_t * colour, bool * is_blinking)
{
    if (current_type == OFF)
        return SYSHAL_LED_ERROR_LED_OFF;

    if (*colour)
        *colour = current_colour;

    if (is_blinking)
        *is_blinking = (current_type == BLINK);

    return SYSHAL_LED_NO_ERROR;
}

int syshal_led_set_sequence(syshal_led_sequence_t sequence, uint32_t time_ms)
{
    switch (sequence)
    {
        case RED_GREEN_BLUE:
            current_type = SEQUENCE;
            current_sequence = RED_GREEN_BLUE;
            current_colour = SYSHAL_LED_COLOUR_RED;

            blink_period_ms = time_ms;
            start_blink_time_ms = syshal_time_get_ticks_ms();

            set_colour(current_colour);
            break;

        default:
            return SYSHAL_LED_ERROR_SEQUENCE_NOT_DEFINE;
            break;
    }

    return SYSHAL_LED_NO_ERROR;
}

int syshal_led_off(void)
{
    current_type = OFF;
    set_colour(SYSHAL_LED_COLOUR_OFF);

    return SYSHAL_LED_NO_ERROR;
}

bool syshal_led_is_active(void)
{
    return (current_type != OFF);
}

void syshal_led_tick(void)
{
    if (current_type == OFF ||
        current_type == SOLID)
    return;

    if (syshal_time_get_ticks_ms() - start_blink_time_ms >= blink_period_ms)
    {
        if (current_type == BLINK)
        {
            if (last_state == SYSHAL_LED_ON)
            {
                last_state = SYSHAL_LED_OFF;
                set_colour(SYSHAL_LED_COLOUR_OFF);
            }
            else
            {
                last_state = SYSHAL_LED_ON;
                set_colour(current_colour);
            }
        }
        else if (current_type == SEQUENCE)
        {
            switch (current_sequence)
            {
                case RED_GREEN_BLUE:
                    switch (current_colour)
                    {
                        case SYSHAL_LED_COLOUR_RED:
                            current_colour = SYSHAL_LED_COLOUR_GREEN;
                            break;
                        case SYSHAL_LED_COLOUR_GREEN:
                            current_colour = SYSHAL_LED_COLOUR_BLUE;
                            break;
                        case SYSHAL_LED_COLOUR_BLUE:
                            current_colour = SYSHAL_LED_COLOUR_RED;
                            break;
                        default:
                            current_colour = SYSHAL_LED_COLOUR_RED;
                            break;
                    }
                    set_colour(current_colour);
                    break;
                default:
                    break;
            }
        }

        start_blink_time_ms = syshal_time_get_ticks_ms();
    }
}