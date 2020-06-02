/* syshal_time.h - HAL for time keeping
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

#ifndef _SYSHAL_TIME_H_
#define _SYSHAL_TIME_H_

#include <stdint.h>

#define SYSHAL_TIME_NO_ERROR   ( 0)
#define SYSHAL_TIME_ERROR_INIT (-1)

int syshal_time_init(void);
int syshal_time_term(void);
uint32_t syshal_time_get_ticks_us(void);
uint32_t syshal_time_get_ticks_ms(void);
void syshal_time_delay_us(uint32_t us);
void syshal_time_delay_ms(uint32_t ms);

#define TICKS_PER_SECOND ( 1000 )
#define ROUND_NEAREST_MULTIPLE(value, magnitude) ( (value + magnitude / 2) / magnitude ) // Integer round to nearest manitude
#define TIME_IN_SECONDS ( ROUND_NEAREST_MULTIPLE(syshal_time_get_ticks_ms(), TICKS_PER_SECOND) ) // Convert millisecond time to seconds

#endif /* _SYSHAL_TIME_H_ */