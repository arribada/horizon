/**
  ******************************************************************************
  * @file     syshal_rtc.c
  * @brief    System hardware abstraction layer for the real-time clock.
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

#include "syshal_temp.h"
#include "nrf_sdh.h"
#include "nrf_soc.h"
#include "debug.h"

int syshal_temp_init(void)
{
    return SYSHAL_TEMP_NO_ERROR;
}

/**
 * @brief      Get the current temperature of the nRF52 die
 *
 * @return     The current temperature in multiples of 0.25 deg C
 */
int32_t syshal_get_temp(void)
{
    int32_t temp = 0x7FFFFFFF;

#ifdef SOFTDEVICE_PRESENT
    if (nrf_sdh_is_enabled())
    {
        sd_temp_get(&temp);
    }
    else
#endif
    {
        // TODO: Replace this with the nrfx implementation in a later nrfx release version
        NRF_TEMP->TASKS_START = 1;

        while (!NRF_TEMP->EVENTS_DATARDY)
        {}

        temp = NRF_TEMP->TEMP;
    }

    DEBUG_PR_TRACE("Read temperature %ld", (temp + 2) / 4);

    return temp;
}