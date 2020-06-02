/* syshal_led.h - HAL for LED
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

#ifndef _SYSHAL_LED_H_
#define _SYSHAL_LED_H_

#include <stdint.h>
#include <stdbool.h>

/* Constants */

#define SYSHAL_LED_NO_ERROR                   ( 0)
#define SYSHAL_LED_ERROR_LED_OFF              (-1)
#define SYSHAL_LED_ERROR_SEQUENCE_NOT_DEFINE  (-2)
#define SYSHAL_LED_ERROR_INIT                 (-3)
#define SYSHAL_LED_ERROR_COUNT_OVERFLOW       (-4)

#define SYSHAL_LED_COLOUR_BLACK   (0x000000)
#define SYSHAL_LED_COLOUR_RED     (0xFF0000)
#define SYSHAL_LED_COLOUR_GREEN   (0x00FF00)
#define SYSHAL_LED_COLOUR_BLUE    (0x0000FF)
#define SYSHAL_LED_COLOUR_YELLOW  (0xFFFF00)
#define SYSHAL_LED_COLOUR_ORANGE  (0xFFA500)
#define SYSHAL_LED_COLOUR_CYAN    (0x00FFFF)
#define SYSHAL_LED_COLOUR_PURPLE  (0xFF00FF)
#define SYSHAL_LED_COLOUR_WHITE   (0xFFFFFF)

#define SYSHAL_LED_COLOUR_OFF (SYSHAL_LED_COLOUR_BLACK)

/* Macros */

/* Types */

typedef enum
{
    RED_GREEN_BLUE,
} syshal_led_sequence_t;

/* Functions */

int syshal_led_init(void);
int syshal_led_set_solid(uint32_t colour);
int syshal_led_set_blinking(uint32_t colour, uint32_t time_ms);
int syshal_led_get(uint32_t * colour, bool * is_blinking);
int syshal_led_set_sequence(syshal_led_sequence_t sequence, uint32_t time_ms);
int syshal_led_off(void);
void syshal_led_tick(void);

bool syshal_led_is_active(void);

#endif /* _SYSHAL_LED_H_ */


