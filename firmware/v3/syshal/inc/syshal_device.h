/* syshal_device.h - HAL for getting details of the device
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

#ifndef _SYSHAL_DEVICE_H_
#define _SYSHAL_DEVICE_H_

#include <stdint.h>
#include <stdbool.h>

#define SYSHAL_DEVICE_NO_ERROR	    ( 0)
#define SYSHAL_DEVICE_ERROR_DEVICE	(-1)

typedef uint8_t device_id_t[8];

int syshal_device_init(void);
int syshal_device_id(device_id_t *device_id);
int syshal_device_set_dfu_entry_flag(bool set);
int syshal_device_firmware_version(uint32_t *version);

#endif /* _SYSHAL_TIMER_H_ */