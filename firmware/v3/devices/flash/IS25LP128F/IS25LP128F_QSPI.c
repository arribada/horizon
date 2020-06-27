/* IS25LP128F.c - HAL implementation for IS25LP128F flash memory device
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

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "bsp.h"
#include "debug.h"
#include "IS25LP128F.h"
#include "syshal_flash.h"
#include "syshal_gpio.h"
#include "syshal_qspi.h"
#include "syshal_firmware.h"

/* Constants */

/* Macros */
#undef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

/* Types */

/* Static Variables */

/* Keep track of the SPI device number for a given
 * logical flash drive.
 */
static uint32_t qspi_devices[IS25LP128F_MAX_DEVICES];

/* Local buffers for building SPI commands.  We
 * provision for 4 extra bytes to allow for the
 * command byte plus 3 bytes of addressing.
 */
static uint8_t  qspi_tx_buf[IS25LP128F_PAGE_SIZE + 4];
static uint8_t  *qspi_rx_buf = qspi_tx_buf;

/* Static Functions */

static uint32_t get_logical_drive_from_qspi_device(uint32_t qspi_device)
{
    for (uint32_t i = 0; i < IS25LP128F_MAX_DEVICES; i++)
        if (qspi_devices[i] == qspi_device)
            return i;
    assert(0);
}

/*! \brief Obtain flash device's status register.
 *
 * \param qspi_device[in] SPI device number for comms.
 * \param status[out] pointer for storing status register.
 *
 * \return Refer to syshal_spi error codes definitions.
 */
static int IS25LP128F_status(uint32_t qspi_device, uint8_t *status)
{
    int ret;

    qspi_tx_buf[0] = RDSR;
    qspi_tx_buf[1] = 0;

    ret = syshal_qspi_transfer(qspi_device, qspi_tx_buf, qspi_rx_buf, 2);

    *status = qspi_rx_buf[0];

    return ret;
}

/*! \brief Execute write enable command with flash device.
 *
 * \param qspi_device[in] SPI device number for comms.
 *
 * \return Refer to syshal_spi error codes definitions.
 */
static int IS25LP128F_wren(uint32_t qspi_device)
{
    int ret;

    qspi_tx_buf[0] = WREN;

    ret = syshal_qspi_transfer(qspi_device, qspi_tx_buf, NULL, 1);

    return ret;
}

/* Exported Functions */

/*! \brief Flash busy handler
 *
 * This handler function can be used to perform useful work
 * whilst busy-waiting on a flash busy condition e.g.,
 * during erase operations this can lock out processing
 * for a significant period of time.
 *
 * \param drive[in] Logical drive number.
 *
 */
__attribute__((weak)) void syshal_flash_busy_handler(uint32_t drive)
{
    (void)drive;
    /* Do not modify -- override with your own handler function */
}

/*! \brief Initialize flash drive.
 *
 * A logical flash drive is associated with the specified
 * SPI device instance number
 *
 * \param drive[in] Logical drive number.
 * \param device[in] SPI device instance for subsequent comms.
 *
 * \return SYSHAL_FLASH_NO_ERROR on success.
 * \return SYSHAL_FLASH_ERROR_DEVICE on SPI initialization error.
 * \return SYSHAL_FLASH_ERROR_INVALID_DRIVE if the drive number is invalid.
 */
int syshal_flash_init(uint32_t drive, uint32_t device)
{
    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    qspi_devices[drive] = device;

    uint8_t status;

    // Read and check the SPI device ID matches the expected value
    qspi_tx_buf[0] = RDJDID;
    qspi_tx_buf[1] = 0;
    qspi_tx_buf[2] = 0;
    qspi_tx_buf[3] = 0;

    syshal_qspi_transfer(qspi_devices[drive], qspi_tx_buf, qspi_rx_buf, 4);

    if (qspi_rx_buf[0] != IS25LP128F_MANUFACTURER_ID ||
        qspi_rx_buf[1] != IS25LP128F_MEMORY_TYPE_ID ||
        qspi_rx_buf[2] != IS25LP128F_CAPACITY_ID)
    {
        DEBUG_PR_ERROR("IS25LP128F not found or unresponsive");
        return SYSHAL_FLASH_ERROR_DEVICE;
    }

    // Set FLASH output drive to 12.5%
    IS25LP128F_wren(qspi_devices[drive]);
    qspi_tx_buf[0] = SERPV;
    qspi_tx_buf[1] = 1 << 5;
    syshal_qspi_transfer(qspi_devices[drive], qspi_tx_buf, NULL, 2);

    // Switch to QSPI mode
    IS25LP128F_wren(qspi_devices[drive]);

    qspi_tx_buf[0] = WRSR;
    qspi_tx_buf[1] = STATUS_QE;
    syshal_qspi_transfer(qspi_devices[drive], qspi_tx_buf, NULL, 2);

    // Wait for QSPI to be programmed
    do
    {
        if (IS25LP128F_status(qspi_devices[drive], &status))
            return SYSHAL_FLASH_ERROR_DEVICE;
        syshal_flash_busy_handler(get_logical_drive_from_qspi_device(qspi_devices[drive]));
    }
    while (status & STATUS_WIP);

    return SYSHAL_FLASH_NO_ERROR;
}

/*! \brief Terminate a flash drive.
 *
 * A logical flash drive is associated with the specified
 * SPI device instance number
 *
 * \param drive[in] Logical drive number.
 *
 * \return SYSHAL_FLASH_NO_ERROR on success.
 * \return SYSHAL_FLASH_ERROR_DEVICE on SPI initialization error.
 * \return SYSHAL_FLASH_ERROR_INVALID_DRIVE if the drive number is invalid.
 */
int syshal_flash_term(uint32_t drive)
{
    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    return SYSHAL_FLASH_NO_ERROR;
}

/*! \brief Erase flash sectors.
 *
 * The input address and size are both assumed to be sector
 * aligned.
 *
 * \param drive[in] Logical drive number.
 * \param address[in] Byte address in flash to erase.
 * \param size[in] Number of bytes in flash to erase.
 *
 * \return SYSHAL_FLASH_NO_ERROR on success.
 * \return SYSHAL_FLASH_ERROR_DEVICE on SPI error.
 * \return SYSHAL_FLASH_ERROR_INVALID_DRIVE if the drive number is invalid.
 */
int syshal_flash_erase(uint32_t drive, uint32_t address, uint32_t size)
{
    uint8_t status;

    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    /* Iteratively erase sectors */
    for (uint32_t i = 0; i < size; i += IS25LP128F_SECTOR_SIZE)
    {
        if (syshal_qspi_erase(qspi_devices[drive], address, SYSHAL_QSPI_ERASE_SIZE_64KB))
            return SYSHAL_FLASH_ERROR_DEVICE;

        address += IS25LP128F_SECTOR_SIZE;

        /* Busy wait until the sector has erased */
        do
        {
            if (IS25LP128F_status(qspi_devices[drive], &status))
                return SYSHAL_FLASH_ERROR_DEVICE;
            syshal_flash_busy_handler(get_logical_drive_from_qspi_device(qspi_devices[drive]));
        }
        while (status & STATUS_WIP);
    }

    return SYSHAL_FLASH_NO_ERROR;
}

/*! \brief Write data to flash drive.
 *
 * It is not permitted to write across page boundaries with
 * write operations.  This will lead to unknown behaviour.
 *
 * \param drive[in] Logical drive number.
 * \param src[in] Source buffer pointer containing bytes to write.
 * \param address[in] Byte address in flash to write.
 * \param size[in] Number of bytes in flash to write.
 *
 * \return SYSHAL_FLASH_NO_ERROR on success.
 * \return SYSHAL_FLASH_ERROR_DEVICE on SPI error.
 * \return SYSHAL_FLASH_ERROR_INVALID_DRIVE if the drive number is invalid.
 */
int syshal_flash_write(uint32_t drive, const void *src, uint32_t address, uint32_t size)
{
    uint8_t status;

    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    while (size > 0)
    {
        uint16_t wr_size = MIN(size, IS25LP128F_PAGE_SIZE);

        if (syshal_qspi_write(qspi_devices[drive], address, src, wr_size))
            return SYSHAL_FLASH_ERROR_DEVICE;

        size -= wr_size;
        address += wr_size;

        /* Busy wait until the sector has erased */
        do
        {
            if (IS25LP128F_status(qspi_devices[drive], &status))
                return SYSHAL_FLASH_ERROR_DEVICE;
            syshal_flash_busy_handler(get_logical_drive_from_qspi_device(qspi_devices[drive]));
        }
        while (status & STATUS_WIP);
    }

    return SYSHAL_FLASH_NO_ERROR;
}

/*! \brief Read data from flash drive.
 *
 * It is not permitted to read across page boundaries with
 * read operations.  This will lead to unknown behaviour.
 *
 * \param drive[in] Logical drive number.
 * \param dest[out] Destination buffer pointer to copy read data into.
 * \param address[in] Byte address in flash to read.
 * \param size[in] Number of bytes in flash to read.
 *
 * \return SYSHAL_FLASH_NO_ERROR on success.
 * \return SYSHAL_FLASH_ERROR_DEVICE on SPI error.
 * \return SYSHAL_FLASH_ERROR_INVALID_DRIVE if the drive number is invalid.
 */
__RAMFUNC int syshal_flash_read(uint32_t drive, void *dest, uint32_t address, uint32_t size)
{
    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    while (size > 0)
    {
        uint16_t rd_size = MIN(size, IS25LP128F_PAGE_SIZE);

        if (syshal_qspi_read(qspi_devices[drive], address, dest, rd_size))
            return SYSHAL_FLASH_ERROR_DEVICE;

        size -= rd_size;
        address += rd_size;
        dest += rd_size;
    }

    return SYSHAL_FLASH_NO_ERROR;
}
