/* nRF52x_regs.h - SPI register definitions for NRF52x
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

#ifndef _NRF52x_REGS_H_
#define _NRF52x_REGS_H_

/* Constants */

#define NRF52_SPI_DATA_PORT_SIZE              254

#define NRF52_FW_WRITE_TIME_PER_WORD_US       100 // Time in us to write a word to internal FLASH

#define NRF52_SPI_WRITE_NOT_READ_ADDR        0x80

/* NRF52 SPI register map */
#define NRF52_REG_ADDR_APP_VERSION           0x00 // Stores app firmware image version
#define NRF52_REG_ADDR_SOFT_DEV_VERSION      0x01 // Stores softdevice version
#define NRF52_REG_ADDR_MODE                  0x02 // The current state the device is in
#define NRF52_REG_ADDR_FW_UPGRADE_SIZE       0x10
#define NRF52_REG_ADDR_FW_UPGRADE_CRC        0x11
#define NRF52_REG_ADDR_INT_STATUS            0x21
#define NRF52_REG_ADDR_INT_ENABLE            0x22
#define NRF52_REG_ADDR_ERROR_CODE            0x23
#define NRF52_REG_ADDR_DEVICE_ADDRESS        0x30 // The 6 byte MAC address
#define NRF52_REG_ADDR_ADVERTISING_INTERVAL  0x31 // Defines the advertising interval in units of 0.625 ms
#define NRF52_REG_ADDR_CONNECTION_INTERVAL   0x32 // Defines the connection interval in units of 1.25 ms
#define NRF52_REG_ADDR_PHY_MODE              0x33 // Defines the PHY mode to use for connections
#define NRF52_REG_ADDR_TX_DATA_PORT          0x60
#define NRF52_REG_ADDR_TX_DATA_LENGTH        0x61 // The amount of space used in the TX buffer (in bytes)
#define NRF52_REG_ADDR_TX_FIFO_SIZE          0x62 // The maximum amount of bytes the device can hold in it's TX buffer
#define NRF52_REG_ADDR_RX_DATA_PORT          0x70
#define NRF52_REG_ADDR_RX_DATA_LENGTH        0x71 // The amount of bytes in the RX buffer waiting to be read via SPI
#define NRF52_REG_ADDR_RX_FIFO_SIZE          0x72 // The maximum amount of bytes the device can hold in it's RX buffer

/* NRF52 SPI register sizes */
#define NRF52_REG_SIZE_APP_VERSION           2
#define NRF52_REG_SIZE_SOFT_DEV_VERSION      2
#define NRF52_REG_SIZE_MODE                  1
#define NRF52_REG_SIZE_FW_UPGRADE_SIZE       4
#define NRF52_REG_SIZE_FW_UPGRADE_CRC        4
#define NRF52_REG_SIZE_FW_UPGRADE_TYPE       1
#define NRF52_REG_SIZE_INT_STATUS            1
#define NRF52_REG_SIZE_INT_ENABLE            1
#define NRF52_REG_SIZE_ERROR_CODE            1
#define NRF52_REG_SIZE_DEVICE_ADDRESS        6
#define NRF52_REG_SIZE_ADVERTISING_INTERVAL  2
#define NRF52_REG_SIZE_CONNECTION_INTERVAL   2
#define NRF52_REG_SIZE_PHY_MODE              1
#define NRF52_REG_SIZE_TX_DATA_PORT          (NRF52_SPI_DATA_PORT_SIZE - 1)
#define NRF52_REG_SIZE_TX_DATA_LENGTH        2
#define NRF52_REG_SIZE_TX_FIFO_SIZE          2
#define NRF52_REG_SIZE_RX_DATA_PORT          NRF52_SPI_DATA_PORT_SIZE
#define NRF52_REG_SIZE_RX_DATA_LENGTH        2
#define NRF52_REG_SIZE_RX_FIFO_SIZE          2

/* Flags for NRF52_REG_ADDR_MODE */
#define NRF52_MODE_IDLE                      0x00
#define NRF52_MODE_FW_UPGRADE                0x01
#define NRF52_MODE_GATT_SERVER               0x03
#define NRF52_MODE_RESET                     0x07
#define NRF52_MODE_LOOPBACK                  0xFF

/* Flags for NRF52_REG_ADDR_INT_STATUS and NRF52_REG_ADDR_INT_ENABLE */
#define NRF52_INT_TX_DATA_SENT               0x01
#define NRF52_INT_RX_DATA_READY              0x02
#define NRF52_INT_GATT_CONNECTED             0x04
#define NRF52_INT_FLASH_PROGRAMMING_DONE     0x08
#define NRF52_INT_ERROR_INDICATION           0x10

/* Flags for NRF52_REG_ADDR_ERROR_CODE */
#define NRF52_ERROR_NONE                     0
#define NRF52_ERROR_CRC                      1
#define NRF52_ERROR_TIMEOUT                  2
#define NRF52_ERROR_LENGTH                   3
#define NRF52_ERROR_FW_TYPE                  4
#define NRF52_ERROR_OVERFLOW                 5

/* Flags for NRF52_REG_ADDR_PHY_MODE */
#define NRF52_PHY_MODE_1_MBPS                0
#define NRF52_PHY_MODE_2_MBPS                1

#endif /* _NRF52x_REGS_H_ */