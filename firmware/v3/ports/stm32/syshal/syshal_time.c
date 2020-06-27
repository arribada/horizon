/**
  ******************************************************************************
  * @file     syshal_time.c
  * @brief    System hardware abstraction layer for system time.
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

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_tim.h"
#include "syshal_time.h"

TIM_HandleTypeDef htim2;

int syshal_time_init(void)
{
    // Setup timer 2 as free running at a frequency of 1MHz. This is used to determine microseconds elapsed
    __HAL_RCC_TIM2_CLK_ENABLE();

    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_MasterConfigTypeDef sMasterConfig;

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = (HAL_RCC_GetHCLKFreq() / 1000000) - 1; // Set clock timer to 1us or 1MHz
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFFFFFF;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim2);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);

    HAL_TIM_Base_Start(&htim2);

    return SYSHAL_TIME_NO_ERROR;
}

/**
 * @brief      Get time since power on in us
 *
 * @return     Time in microseconds
 */
inline uint32_t syshal_time_get_ticks_us(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

/**
 * @brief      Wait for a given time (blocking)
 *
 * @param[in]  ms    Time in microseconds
 */
inline void syshal_time_delay_us(uint32_t us)
{
    uint32_t currentTime = syshal_time_get_ticks_us();
    while (syshal_time_get_ticks_us() - currentTime < us)
    {}
}

/**
 * @brief      Get time since power on in ms
 *
 * @return     Time in milliseconds
 */
inline uint32_t syshal_time_get_ticks_ms(void)
{
    return HAL_GetTick();
}

/**
 * @brief      Wait for a given time (blocking)
 *
 * @param[in]  ms    Time in milliseconds
 */
inline void syshal_time_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}