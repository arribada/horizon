/* Copyright (C) 2018 Arribada
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

// Includes ------------------------------------------------------------------

#include "main.h"
#include "bsp.h"
#include "cexception.h"
#include "debug.h"
#include "sm.h"
#include "sm_main.h"
#include "system_clock.h"
#include "syshal_gpio.h"
#include "syshal_i2c.h"
#include "syshal_spi.h"
#include "syshal_uart.h"
#include "syshal_batt.h"
#include "syshal_gps.h"
#include "syshal_usb.h"
#include "version.h"
#include <string.h>


int main(void)
{
    sm_handle_t state_handle;

    // Reset of all peripherals, Initializes the Flash interface and the Systick
    HAL_Init();

    // Set all pins to Analog to reduce power consumption on unused pins
    GPIO_InitTypeDef GPIO_Init;
    GPIO_Init.Mode = GPIO_MODE_ANALOG;
    GPIO_Init.Pull = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;

    // GPIO ports must be clocked to allow changing of settings
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_Init.Pin = GPIO_PIN_All & ~(GPIO_PIN_13 | GPIO_PIN_14); // Don't change the SWDIO or SWDCLK lines as these are used for the debugging interface
    HAL_GPIO_Init(GPIOA, &GPIO_Init);
    GPIO_Init.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOB, &GPIO_Init);
    HAL_GPIO_Init(GPIOC, &GPIO_Init);
    HAL_GPIO_Init(GPIOD, &GPIO_Init);
    HAL_GPIO_Init(GPIOE, &GPIO_Init);
    HAL_GPIO_Init(GPIOF, &GPIO_Init);

    // PORT A
    GPIO_Init.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_Init.Mode = GPIO_MODE_INPUT;
    GPIO_Init.Pull = GPIO_PULLDOWN;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;

    HAL_GPIO_Init(GPIOA, &GPIO_Init);

    // PORT B
    GPIO_Init.Pin = GPIO_PIN_1 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_13;
    GPIO_Init.Mode = GPIO_MODE_INPUT;
    GPIO_Init.Pull = GPIO_PULLDOWN;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;

    HAL_GPIO_Init(GPIOB, &GPIO_Init);

    // PORT C
    GPIO_Init.Pin = GPIO_PIN_6 | GPIO_PIN_11 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_Init.Mode = GPIO_MODE_INPUT;
    GPIO_Init.Pull = GPIO_PULLDOWN;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;

    HAL_GPIO_Init(GPIOC, &GPIO_Init);

    // PORT D
    GPIO_Init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_Init.Mode = GPIO_MODE_INPUT;
    GPIO_Init.Pull = GPIO_PULLDOWN;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;

    HAL_GPIO_Init(GPIOD, &GPIO_Init);

    // PORT E
    GPIO_Init.Pin = GPIO_PIN_3;
    GPIO_Init.Mode = GPIO_MODE_INPUT;
    GPIO_Init.Pull = GPIO_PULLDOWN;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;

    HAL_GPIO_Init(GPIOE, &GPIO_Init);

    // PORT F
    GPIO_Init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6;
    GPIO_Init.Mode = GPIO_MODE_INPUT;
    GPIO_Init.Pull = GPIO_PULLDOWN;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;

    HAL_GPIO_Init(GPIOF, &GPIO_Init);

    // GPIO_SPI1_CS_BT
    GPIO_Init.Pin = GPIO_PIN_4;
    GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init.Pull = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;

    HAL_GPIO_Init(GPIOA, &GPIO_Init);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

    // GPIO_SPI2_CS_FLASH
    GPIO_Init.Pin = GPIO_PIN_0;
    GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init.Pull = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init.Alternate = 0;

    HAL_GPIO_Init(GPIOD, &GPIO_Init);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);

    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();
    __HAL_RCC_GPIOD_CLK_DISABLE();
    __HAL_RCC_GPIOE_CLK_DISABLE();
    __HAL_RCC_GPIOF_CLK_DISABLE();

    // Configure the system clock
    system_clock_config();

    // Initialize the IWDG
    HAL_IWDG_Init((IWDG_HandleTypeDef *)&IWDG_Init);

    sm_init(&state_handle, sm_main_states);
    sm_set_next_state(&state_handle, SM_MAIN_BOOT);

    while (1)
    {
        CEXCEPTION_T e = CEXCEPTION_NONE;

        Try
        {
            sm_tick(&state_handle);

            // We only kick the IWDG from here -- this assumes that
            // the state machine tick is called sufficiently often and
            // never takes more than 20 seconds to complete
            HAL_IWDG_Refresh((IWDG_HandleTypeDef *)&IWDG_Init);
        }
        Catch (e)
        {
            sm_main_exception_handler(e);
        }
    }

    return 0;

}
