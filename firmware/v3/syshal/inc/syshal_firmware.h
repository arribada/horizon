/* syshal_firmware.h - HAL for writing firmware images to MCU FLASH
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

#ifndef _SYSHAL_FIRMWARE_H_
#define _SYSHAL_FIRMWARE_H_

#include <stdint.h>

#define SYSHAL_FIRMWARE_NO_ERROR                ( 0)
#define SYSHAL_FIRMWARE_ERROR_DEVICE            (-1)
#define SYSHAL_FIRMWARE_ERROR_BUSY              (-2)
#define SYSHAL_FIRMWARE_ERROR_TIMEOUT           (-3)
#define SYSHAL_FIRMWARE_ERROR_FILE_NOT_FOUND    (-4)
#define SYSHAL_FIRMWARE_ERROR_FS                (-5)

// Used for placing function into RAM
// Enforce long calls so that we can jump directly from FLASH to RAM
// Optimise for size to save RAM space
#define __RAMFUNC __attribute__ ((long_call, optimize("Os"), section (".ramfunc")))

int syshal_firmware_update(uint32_t local_file_id, uint32_t app_version);

#endif /* _SYSHAL_FIRMWARE_H_ */