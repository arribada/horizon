/* syshal_axl.h - HAL for accelerometer device
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

#ifndef _SYSHAL_AXL_H_
#define _SYSHAL_AXL_H_

#include <stdint.h>
#include <stdbool.h>

#define SYSHAL_AXL_NO_ERROR                   ( 0)
#define SYSHAL_AXL_ERROR_PROVISIONING_NEEDED  (-1)
#define SYSHAL_AXL_ERROR_DEVICE_UNRESPONSIVE  (-2)

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} syshal_axl_data_t;

int syshal_axl_init(void);
int syshal_axl_term(void);
int syshal_axl_sleep(void);
int syshal_axl_wake(void);
bool syshal_axl_awake(void);
int syshal_axl_tick(void);

__attribute__((weak)) void syshal_axl_callback(syshal_axl_data_t data);

#endif /* _SYSHAL_AXL_H_ */