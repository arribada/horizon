/* syshal_firmware.C - HAL for writing firmware images to MCU FLASH
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

#include "syshal_firmware.h"
#include "stm32f0xx_hal.h"
#include "bsp.h"
#include "syshal_gpio.h"

static uint32_t writing_address; // The FLASH address we're currently writing to
static uint32_t bytes_remaining; // Number of buffer.bytes yet to be written to FLASH

static union
{
    uint16_t half_word;
    uint8_t bytes[2];
} buffer;

__RAMFUNC void FLASH_WaitForLastOperation_priv(void)
{
    // Wait for the FLASH operation to complete by polling on BUSY flag to be reset.
    // Even if the FLASH operation fails, the BUSY flag will be reset
    while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY))
    {}

    // Check FLASH End of Operation flag
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP))
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP); // Clear FLASH End of Operation pending bit
}

__RAMFUNC void FLASH_Program_HalfWord_priv(uint32_t address, uint16_t data)
{
    // Wait for last operation to be completed
    FLASH_WaitForLastOperation_priv();

    // Proceed to program the new data
    SET_BIT(FLASH->CR, FLASH_CR_PG);

    // Write data in the address
    *(__IO uint16_t *)address = data;

    // Wait for last operation to be completed
    FLASH_WaitForLastOperation_priv();

    // If the program operation is completed, disable the PG Bit */
    CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
}

__RAMFUNC int syshal_firmware_prepare(void)
{
    __disable_irq(); // Don't allow any interrupts to interrupt us

    writing_address = FLASH_BASE; // Start writing at the beginning of the FLASH
    bytes_remaining = 0;

    HAL_FLASH_Unlock(); // Unlock the Flash to enable the flash control register access

    // Erase entire flash
    FLASH_WaitForLastOperation_priv();

    // Execute FLASH mass erase
    SET_BIT(FLASH->CR, FLASH_CR_MER);
    SET_BIT(FLASH->CR, FLASH_CR_STRT);

    FLASH_WaitForLastOperation_priv();

    CLEAR_BIT(FLASH->CR, FLASH_CR_MER); // If the erase operation is completed, disable the MER Bit
    return SYSHAL_FIRMWARE_NO_ERROR;
}

__RAMFUNC int syshal_firmware_write(uint8_t * data, uint32_t size)
{
    // Iterate through every discrete byte
    for (uint32_t i = 0; i < size; ++i)
    {
        // Fill up blocks of 16 bits
        buffer.bytes[bytes_remaining] = data[i];
        bytes_remaining++;

        // If we have a full 16 bits, write it to FLASH
        if (bytes_remaining == 2)
        {
            FLASH_Program_HalfWord_priv(writing_address, buffer.half_word);
            writing_address += 2;
            bytes_remaining = 0;
        }
    }

    return SYSHAL_FIRMWARE_NO_ERROR;
}

__RAMFUNC int syshal_firmware_flush(void)
{
    // Flush any remaining data to the FLASH
    if (bytes_remaining)
    {
        FLASH_Program_HalfWord_priv(writing_address, buffer.half_word);
        writing_address += 2;
        bytes_remaining = 0;
    }

    SET_BIT(FLASH->CR, FLASH_CR_LOCK); // Lock the FLASH to disable the flash control register access

    return SYSHAL_FIRMWARE_NO_ERROR;
}
