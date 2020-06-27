/* retained_ram.h - Header for exposing syshal_rtc RAM retained timestamp
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

#ifndef _RETAINED_RAM_H_
#define _RETAINED_RAM_H_

#include <stdint.h>

extern uint32_t retained_ram_rtc_timestamp __attribute__ ((section(".noinit")));

#endif /* _RETAINED_RAM_H_ */