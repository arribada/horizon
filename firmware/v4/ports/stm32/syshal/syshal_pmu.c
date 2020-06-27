/**
  ******************************************************************************
  * @file     syshal_pmu.c
  * @brief    System hardware abstraction layer for sleep states.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2018 Arribada</center></h2>
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

#include "bsp.h"
#include "system_clock.h"
#include "syshal_pmu.h"
#include "syshal_gpio.h"
#include "syshal_firmware.h"
#include "stm32f0xx_hal_gpio.h"
#include "debug.h"

/**
 * @brief      Set the desired power mode
 *
 * @param[in]  level  The power level
 */
void syshal_pmu_set_level(syshal_pmu_power_level_t level)
{

    switch (level)
    {
        case POWER_STOP:
            DEBUG_PR_TRACE("Entering POWER_STOP mode");
            HAL_SuspendTick(); // Disable the Systick to prevent it waking us up
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
            system_clock_config(); // Re-establish the clock settings
            HAL_ResumeTick(); // Re-enable the Systick
            break;

        case POWER_SLEEP:
            DEBUG_PR_TRACE("Entering POWER_SLEEP mode");
            HAL_SuspendTick(); // Disable the Systick to prevent it waking us up
            HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
            HAL_ResumeTick(); // Re-enable the Systick
            break;

        case POWER_STANDBY:
            DEBUG_PR_TRACE("Entering POWER_STANDBY mode");
            HAL_PWR_EnterSTANDBYMode();
            system_clock_config(); // Re-establish the clock settings
            break;

        default:
            break;
    }

}

/**
 * @brief      Causes a software reset of the MCU
 */
__RAMFUNC void syshal_pmu_reset(void)
{
    // Ensure all outstanding memory accesses including buffered writes are completed before reset
    __DSB();
    SCB->AIRCR  = ((0x5FAUL << SCB_AIRCR_VECTKEY_Pos) |
                   SCB_AIRCR_SYSRESETREQ_Msk);
    __DSB(); // Ensure completion of memory access

    for (;;) // Wait until reset
    {
        __NOP();
    }
}

uint32_t syshal_pmu_get_startup_status(void)
{
    uint32_t csr = RCC->CSR;
    RCC->CSR |= (1<<24);
    return csr;
}

__RAMFUNC void syshal_pmu_kick_watchdog(void)
{
    __HAL_IWDG_RELOAD_COUNTER((IWDG_HandleTypeDef *)&IWDG_Init);
}
