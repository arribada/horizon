/* IS25LP128F.h - HAL implementation for IS25LP128F flash memory device
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

#ifndef _IS25LP128F_H_
#define _IS25LP128F_H_

/* Constants */

#define IS25LP128F_SECTOR_SIZE        (256 * 256)
#define IS25LP128F_NUM_SECTORS        256
#define IS25LP128F_PAGE_SIZE          256

#define IS25LP128F_MANUFACTURER_ID 0x9D
#define IS25LP128F_MEMORY_TYPE_ID  0x60
#define IS25LP128F_CAPACITY_ID     0x18

/* SPI flash comments */
#define PP          0x02 // Page program
#define READ        0x03 // Normal read operation
#define WREN        0x06 // Write enable
#define WRDI        0x04
#define RDJDIDQ     0xAF // Read JEDEC ID QPI mode
#define RDJDID      0x9F // Read JEDEC ID SPI mode
#define RDSR        0x05
#define WRSR        0x01
#define RDERP       0x81 // Read extended read parameters
#define RDBR        0x16 // Read bank address register
#define SERPNV      0x85 // Set Extended Read Parameters (Non-Volatile)
#define SERPV       0x83 // Set Extended Read Parameters (Volatile)
#define BER64K		0xD8 // Block erase 64Kbyte
#define POWER_DOWN	0xB9 // Power down the device
#define POWER_UP	0xAB // Wake the device from a powered down state

/* SPI flash status bits */
#define STATUS_WIP    (1 << 0) // Write in progress
#define STATUS_WEL    (1 << 1) // Write enable latch
#define STATUS_BP0    (1 << 2) // Block protection bit
#define STATUS_BP1    (1 << 3) // Block protection bit
#define STATUS_BP2    (1 << 4) // Block protection bit
#define STATUS_BP3    (1 << 5) // Block protection bit
#define STATUS_QE     (1 << 6) // Quad enable bit
#define STATUS_SRWD   (1 << 7) // Status Register Write Disable

/* Macros */

#define POWER_DOWN_TIME_US (3)
#define POWER_UP_TIME_US   (5)

#ifndef IS25LP128F_MAX_DEVICES
#define IS25LP128F_MAX_DEVICES        1
#endif

/* Types */

#endif /* _IS25LP128F_H_ */
