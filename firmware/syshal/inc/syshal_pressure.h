/* syshal_pressure.h - HAL for pressure sensor
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

#ifndef _SYSHAL_PRESSURE_H_
#define _SYSHAL_PRESSURE_H_

// Constants
#define SYSHAL_PRESSURE_NO_ERROR            ( 0)
#define SYSHAL_PRESSURE_ERROR_CRC_MISMATCH  (-1)

#include <stdbool.h>

int syshal_pressure_init(void);
int syshal_pressure_term(void);
int syshal_pressure_sleep(void);
int syshal_pressure_wake(void);
bool syshal_pressure_awake(void);
int syshal_pressure_tick(void);

__attribute__((weak)) void syshal_pressure_callback(int32_t pressure);

#endif /* _SYSHAL_PRESSURE_H_ */