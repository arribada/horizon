/* syshal_i2c.h - HAL for flash device
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

#ifndef _SYSHAL_I2C_H_
#define _SYSHAL_I2C_H_

#include <stdint.h>

// Constants
#define SYSHAL_I2C_NO_ERROR                ( 0)
#define SYSHAL_I2C_ERROR_INVALID_INSTANCE  (-1)
#define SYSHAL_I2C_ERROR_BUSY              (-2)
#define SYSHAL_I2C_ERROR_TIMEOUT           (-3)
#define SYSHAL_I2C_ERROR_DEVICE            (-4)
#define SYSHAL_I2C_ERROR_INTERFACE         (-5)

int syshal_i2c_init(uint32_t instance);
int syshal_i2c_term(uint32_t instance);
int syshal_i2c_transfer(uint32_t instance, uint8_t slaveAddress, uint8_t * data, uint32_t size);
uint32_t syshal_i2c_receive(uint32_t instance, uint8_t slaveAddress, uint8_t * data, uint32_t size); // returns length of data read
int syshal_i2c_read_reg(uint32_t instance, uint8_t slaveAddress, uint8_t regAddress, uint8_t * data, uint32_t size); // returns length of data read
int syshal_i2c_write_reg(uint32_t instance, uint8_t slaveAddress, uint8_t regAddress, uint8_t * data, uint32_t size);

int syshal_i2c_is_device_ready(uint32_t instance, uint8_t slaveAddress);

#endif /* _SYSHAL_I2C_H_ */