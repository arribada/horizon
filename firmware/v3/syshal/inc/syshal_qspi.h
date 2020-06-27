/* syshal_qspi.h - HAL for QSPI
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

#ifndef _SYSHAL_QSPI_H_
#define _SYSHAL_QSPI_H_

#include <stdint.h>
#include <stdbool.h>

// Constants
#define SYSHAL_QSPI_NO_ERROR                ( 0)
#define SYSHAL_QSPI_ERROR_INVALID_INSTANCE  (-1)
#define SYSHAL_QSPI_INVALID_PARM			(-2)

typedef enum
{
    SYSHAL_QSPI_ERASE_SIZE_4KB,  /**< Erase 4 kB block (flash command 0x20). */
    SYSHAL_QSPI_ERASE_SIZE_64KB, /**< Erase 64 kB block (flash command 0xD8). */
    SYSHAL_QSPI_ERASE_SIZE_ALL   /**< Erase all (flash command 0xC7). */
} syshal_qspi_erase_size_t;

int syshal_qspi_init(uint32_t instance);
int syshal_qspi_term(uint32_t instance);

int syshal_qspi_read(uint32_t instance, uint32_t src_address, uint8_t * data, uint32_t size);
int syshal_qspi_write(uint32_t instance, uint32_t dst_address, const uint8_t * data, uint32_t size);
int syshal_qspi_erase(uint32_t instance, uint32_t start_address, syshal_qspi_erase_size_t size);
int syshal_qspi_transfer(uint32_t instance, uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

#endif /* _SYSHAL_QSPI_H_ */