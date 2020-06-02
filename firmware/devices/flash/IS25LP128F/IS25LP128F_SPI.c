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
#include "syshal_spi.h"
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
static uint32_t spi_devices[IS25LP128F_MAX_DEVICES];

/* Local buffers for building SPI commands.  We
 * provision for 4 extra bytes to allow for the
 * command byte plus 3 bytes of addressing.
 */
static uint8_t  spi_tx_buf[IS25LP128F_PAGE_SIZE + 4];
static uint8_t  spi_rx_buf[IS25LP128F_PAGE_SIZE + 4];

/* Static Functions */

static uint32_t get_logical_drive_from_spi_device(uint32_t spi_device)
{
    for (uint32_t i = 0; i < IS25LP128F_MAX_DEVICES; i++)
        if (spi_devices[i] == spi_device)
            return i;
    assert(0);
}

/*! \brief Obtain flash device's status register.
 *
 * \param spi_device[in] SPI device number for comms.
 * \param status[out] pointer for storing status register.
 *
 * \return Refer to syshal_spi error codes definitions.
 */
static int IS25LP128F_status(uint32_t spi_device, uint8_t *status)
{
    int ret;

    spi_tx_buf[0] = RDSR;
    spi_tx_buf[1] = 0;

    ret = syshal_spi_transfer(spi_device, spi_tx_buf, spi_rx_buf, 2);

    *status = spi_rx_buf[1];

    return ret;
}

/*! \brief Execute write enable command with flash device.
 *
 * \param spi_device[in] SPI device number for comms.
 *
 * \return Refer to syshal_spi error codes definitions.
 */
static int IS25LP128F_wren(uint32_t spi_device)
{
    int ret;

    spi_tx_buf[0] = WREN;

    ret = syshal_spi_transfer(spi_device, spi_tx_buf, NULL, 1);

    return ret;
}

/*! \brief Execute erase sector command with flash device.
 *
 * Following command execution, this routine will busy wait until
 * the status register indicates it is no longer busy.  This
 * allows back-to-back operations to be performed without
 * first checking for a busy condition.
 *
 * \param spi_device[in] SPI device number for comms.
 * \param addr[in] Physical byte address in flash to erase.
 *
 * \return Refer to syshal_spi error codes definitions.
 */
static int IS25LP128F_erase_sector(uint32_t spi_device, uint32_t addr)
{
    int ret;
    uint8_t status;

    spi_tx_buf[0] = BER64K;
    spi_tx_buf[1] = (uint8_t) (addr >> 16);
    spi_tx_buf[2] = (uint8_t) (addr >> 8);
    spi_tx_buf[3] = (uint8_t) (addr);

    ret = syshal_spi_transfer(spi_device, spi_tx_buf, spi_rx_buf, 4);

    if (ret) return ret;

    /* Busy wait until the sector has erased */
    ret = IS25LP128F_status(spi_device, &status);
    while ((status & STATUS_WIP) && !ret)
    {
        syshal_flash_busy_handler(get_logical_drive_from_spi_device(spi_device));
        ret = IS25LP128F_status(spi_device, &status);
    }

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

    spi_devices[drive] = device;

    uint8_t status;

    // Set the unused pins high
    syshal_gpio_init(GPIO_FLASH_IO2);
    syshal_gpio_init(GPIO_FLASH_IO3);
    syshal_gpio_set_output_high(GPIO_FLASH_IO2);
    syshal_gpio_set_output_high(GPIO_FLASH_IO3);

    syshal_flash_wakeup(drive); // Wake the device on the off chance it was asleep

    // Read and check the SPI device ID matches the expected value
    spi_tx_buf[0] = RDJDID;
    spi_tx_buf[1] = 0;
    spi_tx_buf[2] = 0;
    spi_tx_buf[3] = 0;

    syshal_spi_transfer(spi_devices[drive], spi_tx_buf, spi_rx_buf, 4);

    if (spi_rx_buf[1] != IS25LP128F_MANUFACTURER_ID ||
        spi_rx_buf[2] != IS25LP128F_MEMORY_TYPE_ID ||
        spi_rx_buf[3] != IS25LP128F_CAPACITY_ID)
    {
        DEBUG_PR_ERROR("IS25LP128F not found or unresponsive");
        return SYSHAL_FLASH_ERROR_DEVICE;
    }

    // Set FLASH output drive to 12.5%
    IS25LP128F_wren(spi_devices[drive]);
    spi_tx_buf[0] = SERPV;
    spi_tx_buf[1] = 1 << 5;
    syshal_spi_transfer(spi_devices[drive], spi_tx_buf, NULL, 2);

    // Make sure we're not in QSPI mode
    IS25LP128F_wren(spi_devices[drive]);

    spi_tx_buf[0] = WRSR;
    spi_tx_buf[1] = 0;
    syshal_spi_transfer(spi_devices[drive], spi_tx_buf, NULL, 2);

    // Wait for QSPI to be programmed
    do
    {
        if (IS25LP128F_status(spi_devices[drive], &status))
            return SYSHAL_FLASH_ERROR_DEVICE;
        syshal_flash_busy_handler(get_logical_drive_from_spi_device(spi_devices[drive]));
    }
    while (status & STATUS_WIP);

    return SYSHAL_FLASH_NO_ERROR;
}

int syshal_flash_get_size(uint32_t drive, uint32_t *size)
{
    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    *size = IS25LP128F_SECTOR_SIZE * IS25LP128F_NUM_SECTORS;

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
    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    /* Enable writes */
    if (IS25LP128F_wren(spi_devices[drive]))
        return SYSHAL_FLASH_ERROR_DEVICE;

    /* Iteratively erase sectors */
    for (uint32_t i = 0; i < size; i += IS25LP128F_SECTOR_SIZE)
    {
        if (IS25LP128F_erase_sector(spi_devices[drive], address))
            return SYSHAL_FLASH_ERROR_DEVICE;
        address += IS25LP128F_SECTOR_SIZE;
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
    int ret;
    uint8_t status;

    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    /* Enable writes */
    if (IS25LP128F_wren(spi_devices[drive]))
        return SYSHAL_FLASH_ERROR_DEVICE;

    while (size > 0)
    {
        spi_tx_buf[0] = PP;
        spi_tx_buf[1] = (uint8_t) (address >> 16);
        spi_tx_buf[2] = (uint8_t) (address >> 8);
        spi_tx_buf[3] = (uint8_t) (address);

        uint16_t wr_size = MIN(size, IS25LP128F_PAGE_SIZE);
        memcpy(&spi_tx_buf[4], src, wr_size);

        ret = syshal_spi_transfer(spi_devices[drive], spi_tx_buf, spi_rx_buf, wr_size + 4);

        if (ret)
            return SYSHAL_FLASH_ERROR_DEVICE;
        size -= wr_size;
        address += wr_size;

        /* Busy wait until the sector has erased */
        do
        {
            if (IS25LP128F_status(spi_devices[drive], &status))
                return SYSHAL_FLASH_ERROR_DEVICE;
        } while (status & STATUS_WIP);
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
    int ret;

    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    //memset(spi_tx_buf, 0, sizeof(spi_tx_buf)); // Can't use as memset doesn't reside in RAM
    for (uint32_t i = 0; i < sizeof(spi_tx_buf); ++i)
        spi_tx_buf[i] = 0;

    spi_tx_buf[0] = READ;

    while (size > 0)
    {
        spi_tx_buf[1] = (uint8_t) (address >> 16);
        spi_tx_buf[2] = (uint8_t) (address >> 8);
        spi_tx_buf[3] = (uint8_t) (address);

        uint16_t rd_size = MIN(size, IS25LP128F_PAGE_SIZE);

        ret = syshal_spi_transfer(spi_devices[drive], spi_tx_buf, spi_rx_buf, rd_size + 4);
        if (ret) 
            return SYSHAL_FLASH_ERROR_DEVICE;

        size -= rd_size;
        address += rd_size;
        //memcpy(dest, &spi_rx_buf[4], rd_size); // Can't use as memcpy doesn't reside in RAM
        for (uint32_t i = 0; i < rd_size; ++i)
            ((uint8_t *) dest)[i] = spi_rx_buf[4 + i];
        dest = ((uint8_t *)dest) + rd_size;
    }

    return SYSHAL_FLASH_NO_ERROR;
}

int syshal_flash_sleep(uint32_t drive)
{
    int ret;

    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    spi_tx_buf[0] = POWER_DOWN;
    ret = syshal_spi_transfer(spi_devices[drive], spi_tx_buf, spi_rx_buf, 1);
    if (ret) 
        return SYSHAL_FLASH_ERROR_DEVICE;

    syshal_time_delay_us(POWER_DOWN_TIME_US);

    return SYSHAL_FLASH_NO_ERROR;
}

int syshal_flash_wakeup(uint32_t drive)
{
    int ret;

    if (drive >= IS25LP128F_MAX_DEVICES)
        return SYSHAL_FLASH_ERROR_INVALID_DRIVE;

    spi_tx_buf[0] = POWER_UP;
    ret = syshal_spi_transfer(spi_devices[drive], spi_tx_buf, spi_rx_buf, 1);
    if (ret) 
        return SYSHAL_FLASH_ERROR_DEVICE;

    syshal_time_delay_us(POWER_UP_TIME_US);

    return SYSHAL_FLASH_NO_ERROR;
}