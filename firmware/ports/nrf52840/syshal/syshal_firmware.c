/**
  ******************************************************************************
  * @file     syshal_firmware.c
  * @brief    System hardware abstraction layer for writing firmware images to
  *           the FLASH.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2019 Arribada</center></h2>
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
  *
  ******************************************************************************
  */

#include "syshal_firmware.h"
#include "syshal_pmu.h"
#include "sm_main.h"
#include "syshal_rtc.h"
#include "nrf_sdh.h"
#include "fs.h"
#include "nrf_dfu_settings.h"
#include "crc32.h"
#include "debug.h"

// Defined in the GNU linker file
extern void * __m_flash_start;
extern void * __m_flash_size;

#define APPLICATION_BASE_ADDR ((uint32_t) &__m_flash_start)
#define APPLICATION_LENGTH    ((uint32_t) &__m_flash_size)

static union
{
    uint32_t word;
    uint8_t bytes[4];
} buffer;

static uint32_t writing_address; // The FLASH address we're currently writing to
static uint32_t bytes_remaining; // Number of buffer.bytes yet to be written to FLASH

static int fs_error_mapping(int error_code)
{
    switch (error_code)
    {
        case FS_ERROR_FILE_NOT_FOUND:
            return SYSHAL_FIRMWARE_ERROR_FILE_NOT_FOUND;
        default:
            return SYSHAL_FIRMWARE_ERROR_FS;
    }
}

static __RAMFUNC void nrf_nvmc_write_words_ramfunc(uint32_t address, const uint32_t * src, uint32_t num_words)
{
    uint32_t i;

    // Enable write.
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
    __ISB();
    __DSB();

    for (i = 0; i < num_words; i++)
    {
        ((uint32_t*)address)[i] = src[i];
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {;}
    }

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    __ISB();
    __DSB();
}

static __RAMFUNC void nrf_nvmc_page_erase_ramfunc(uint32_t address)
{
    // Enable erase.
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
    __ISB();
    __DSB();

    // Erase the page
    NRF_NVMC->ERASEPAGE = address;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {;}

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    __ISB();
    __DSB();
}

static __RAMFUNC int syshal_firmware_prepare(uint32_t app_version)
{
    syshal_rtc_stash_time();

    nrf_sdh_disable_request(); // Disable the softdevice

    // Update the application version in the DFU settings page in FLASH
    s_dfu_settings.app_version = app_version;

    nrf_dfu_settings_write_and_backup(NULL);

    __disable_irq(); // Don't allow any interrupts to interrupt us

    uint32_t page_size = NRF_FICR->CODEPAGESIZE;
    uint32_t pages_to_erase = APPLICATION_LENGTH / page_size;

    // Erase all application firmware pages
    for (uint32_t i = 0; i < pages_to_erase; ++i)
    {
        nrf_nvmc_page_erase_ramfunc(APPLICATION_BASE_ADDR + page_size * i);
    }

    writing_address = APPLICATION_BASE_ADDR;
    bytes_remaining = 0;

    return SYSHAL_FIRMWARE_NO_ERROR;
}

static __RAMFUNC int syshal_firmware_write(uint8_t * data, uint32_t size)
{
    // Iterate through every discrete byte
    for (uint32_t i = 0; i < size; ++i)
    {
        // Fill up blocks of 32 bits
        buffer.bytes[bytes_remaining] = data[i];
        bytes_remaining++;

        // If we have a full 32 bits, write it to FLASH
        if (bytes_remaining == 4)
        {
            nrf_nvmc_write_words_ramfunc(writing_address, &buffer.word, 1);
            writing_address += 4;
            bytes_remaining = 0;
        }
    }

    return SYSHAL_FIRMWARE_NO_ERROR;
}

static __RAMFUNC int syshal_firmware_flush(void)
{
    // Flush any remaining data to the FLASH
    if (bytes_remaining)
    {
        nrf_nvmc_write_words_ramfunc(writing_address, &buffer.word, 1);
        writing_address += 4;
        bytes_remaining = 0;
    }

    return SYSHAL_FIRMWARE_NO_ERROR;
}

__RAMFUNC int syshal_firmware_update(uint32_t local_file_id, uint32_t app_version)
{
    uint32_t bytes_actually_read;
    uint8_t read_buffer[1024];
    fs_handle_t file_handle;
    uint32_t total_bytes = 0;
    uint32_t *ptr_crc = NULL;
    uint32_t crc;
    int ret;

    // Calculate the CRC of the firmware image we are to apply and store it in flash
    // This is required as otherwise the DFU bootloader will reject the image and fail to boot
    ret = fs_open(file_system, &file_handle, local_file_id, FS_MODE_READONLY, NULL);
    if (ret)
        return fs_error_mapping(ret);

    do
    {
        ret = fs_read(file_handle, &read_buffer, sizeof(read_buffer), &bytes_actually_read);
        total_bytes += bytes_actually_read;
        crc = crc32_compute(read_buffer, bytes_actually_read, ptr_crc);
        ptr_crc = &crc;

        syshal_pmu_kick_watchdog();
    }
    while (FS_ERROR_END_OF_FILE != ret);

    fs_close(file_handle);

    s_dfu_settings.bank_0.image_crc = crc;
    s_dfu_settings.bank_0.image_size = total_bytes;
    ret = fs_open(file_system, &file_handle, local_file_id, FS_MODE_READONLY, NULL);
    if (ret)
        return fs_error_mapping(ret);

    syshal_firmware_prepare(app_version);

    // Everything from here on out MUST reside in RAM as our FLASH has been erased
    do
    {
        ret = fs_read(file_handle, &read_buffer, sizeof(read_buffer), &bytes_actually_read);
        syshal_firmware_write(read_buffer, bytes_actually_read);
        syshal_pmu_kick_watchdog();
    }
    while (FS_ERROR_END_OF_FILE != ret);

    syshal_firmware_flush();

    // Reset the device
    for (;;)
    {
        NVIC_SystemReset();
    }
}