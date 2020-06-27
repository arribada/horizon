/* syshal_device.c - HAL for getting details of the device
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

#include <string.h>
#include "syshal_device.h"
#include "nrf.h"
#include "nrf_soc.h"
#include "nrf_dfu_settings.h"

void * __stack_chk_guard = (void *)(0xDEADBEEF);

void __stack_chk_fail(void) // called by stack checking code if guard variable is corrupted
{
#ifndef DEBUG_DISABLED
    printf("\r\nSTACK CORRUPTION DETECTED\r\n");
#endif
}

void HardFault_Handler(void)
{
#ifndef DEBUG_DISABLED
    printf("\r\nHARD FAULT DETECTED\r\n");
#endif
    NVIC_SystemReset();
}

#define BOOTLOADER_DFU_START (0xB1)

int syshal_device_init(void)
{
    // Load the DFU Settings from the the bootloader_settings_page
    if (nrf_dfu_settings_init(false) != NRF_SUCCESS)
        return SYSHAL_DEVICE_ERROR_DEVICE;

    return SYSHAL_DEVICE_NO_ERROR;
}

int syshal_device_id(device_id_t * device_id)
{
    if (sizeof(device_id_t) < sizeof(NRF_FICR->DEVICEID))
        return SYSHAL_DEVICE_ERROR_DEVICE;
    memcpy(device_id, (const uint8_t*)NRF_FICR->DEVICEID, sizeof(NRF_FICR->DEVICEID));
    
    return SYSHAL_DEVICE_NO_ERROR;
}

int syshal_device_set_dfu_entry_flag(bool set)
{
    if (set)
        sd_power_gpregret_set(0, BOOTLOADER_DFU_START);
    else
        sd_power_gpregret_set(0, 0x00);

    return SYSHAL_DEVICE_NO_ERROR;
}

int syshal_device_firmware_version(uint32_t *version)
{
	*version = s_dfu_settings.app_version;
	return SYSHAL_DEVICE_NO_ERROR;
}

int syshal_device_bootloader_version(uint32_t *version)
{
	*version = *(uint32_t*)0x000FDFFC; // Defined in the bootloader linker file
	return SYSHAL_DEVICE_NO_ERROR;
}
