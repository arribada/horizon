/* syshal_button.h - HAL for implementing buttons
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

#ifndef _SYSHAL_BUTTON_H_
#define _SYSHAL_BUTTON_H_

#include <stdint.h>

// Constants
#define SYSHAL_BUTTON_NO_ERROR               ( 0)
#define SYSHAL_BUTTON_ERROR_NOT_INITIALISED  (-1)

typedef enum
{
    SYSHAL_BUTTON_PRESSED,
    SYSHAL_BUTTON_RELEASED
} syshal_button_event_id_t;

typedef struct
{
    syshal_button_event_id_t id;
    struct
    {
        uint32_t duration_ms;
    } released;
} syshal_button_event_t;

int syshal_button_init(uint32_t pin);
int syshal_button_term(uint32_t pin);

void syshal_button_callback(syshal_button_event_t event);

#endif /* _SYSHAL_BUTTON_H_ */