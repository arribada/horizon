/* syshal_flash.h - HAL for flash device
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

#ifndef _SYSHAL_FLASH_H_
#define _SYSHAL_FLASH_H_

#include <stdint.h>

/* Constants */

#define SYSHAL_FLASH_NO_ERROR            ( 0)
#define SYSHAL_FLASH_ERROR_INVALID_DRIVE (-1)
#define SYSHAL_FLASH_ERROR_DEVICE        (-2)

/* Macros */

/* Types */

/* Functions */

int syshal_flash_init(uint32_t drive, uint32_t device);
int syshal_flash_term(uint32_t drive);
int syshal_flash_erase(uint32_t drive, uint32_t address, uint32_t size);
int syshal_flash_write(uint32_t drive, const void *src, uint32_t address, uint32_t size);
int syshal_flash_read(uint32_t drive, void *dest, uint32_t address, uint32_t size);
int syshal_flash_get_size(uint32_t drive, uint32_t *size);
int syshal_flash_sleep(uint32_t drive);
int syshal_flash_wakeup(uint32_t drive);
__attribute__((weak)) void syshal_flash_busy_handler(uint32_t drive);

#endif /* _SYSHAL_FLASH_H_ */
