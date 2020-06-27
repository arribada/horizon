/* S25FL128.h - HAL implementation for S25FL128 flash memory device
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

#ifndef _S25FL128_H_
#define _S25FL128_H_

/* Constants */

#define S25FL128_SECTOR_SIZE        (256 * 1024)
#define S25FL128_NUM_SECTORS        64
#define S25FL128_PAGE_SIZE          512

#define S25FL128_MANUFACTURER_ID 0x01
#define S25FL128_DEVICE_ID_MSB 0x20
#define S25FL128_DEVICE_ID_LSB 0x18

/* SPI flash comments */
#define WREN        0x06
#define WRDI        0x04
#define RDID        0x9F
#define RDSR        0x05
#define WRSR        0x01
#define READ        0x03
#define FAST_READ   0x0B
#define PP          0x02
#define SE          0xD8
#define BE          0xC7
#define DP          0xB9
#define RES         0xAB

/* SPI flash status bits */
#define RDSR_BUSY   (1 << 0)
#define RDSR_WEL    (1 << 1)
#define RDSR_BP0    (1 << 2)
#define RDSR_BP1    (1 << 3)
#define RDSR_BP2    (1 << 4)
#define RDSR_SRWD   (1 << 7)

/* Macros */

#ifndef S25FL128_MAX_DEVICES
#define S25FL128_MAX_DEVICES        1
#endif

/* Types */

#endif /* _S25FL128_H_ */
