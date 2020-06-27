/* syshal_spi.h - HAL for SPI
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

#ifndef _SYSHAL_SPI_H_
#define _SYSHAL_SPI_H_

#include <stdint.h>

/* Constants */

#define SYSHAL_SPI_NO_ERROR                0
#define SYSHAL_SPI_ERROR_INVALID_INSTANCE -1
#define SYSHAL_SPI_ERROR_BUSY             -2
#define SYSHAL_SPI_ERROR_TIMEOUT          -3
#define SYSHAL_SPI_ERROR_DEVICE           -4

/* Macros */

/* Types */

/* Functions */

int syshal_spi_init(uint32_t instance);
int syshal_spi_term(uint32_t instance);
int syshal_spi_transfer(uint32_t instance, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
int syshal_spi_transfer_continous(uint32_t instance, const uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
int syshal_spi_finish_transfer(uint32_t instance);

#endif /* _SYSHAL_SPI_H_ */
