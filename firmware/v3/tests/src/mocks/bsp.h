/* Copyright (C) 2019 Arribada
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

#ifndef _BSP_H_
#define _BSP_H_

#include <stdint.h>
#include "nrfx_rtc.h"

// Logical device mappings to physical devices
#define UART_DEBUG            UART_0
#define UART_GPS              UART_0
#define UART_CELLULAR         UART_1
#define I2C_AXL               I2C_1
#define I2C_BATTERY           I2C_1
#define I2C_PRESSURE          I2C_1
#define SPI_SATALLITE         SPI_2
#define SPI_FLASH             SPI_3
#define PWM_LED               PWM_0
#define TIMER_UART_TIMEOUT    TIMER_1
#define TIMER_BUTTON_DEBOUNCE TIMER_2
#define TIMER_LED             TIMER_3
#define RTC_SOFT_WATCHDOG     RTC_1
#define RTC_TIME_KEEPING      RTC_2
#define ADC_BATTERY           ADC_CHANNEL_0

typedef enum
{
    GPIO_DEBUG,
    GPIO_EXT1_GPIO1,
    GPIO_EXT1_GPIO2,
    GPIO_EXT1_GPIO3,
    GPIO_EXT2_GPIO1,
    GPIO_EXT2_GPIO2,
    GPIO_EXT2_GPIO3,
    GPIO_EXT2_GPIO4,
    GPIO_EXT2_GPIO5,
    GPIO_EXT2_GPIO6,
    GPIO_SWS,
    GPIO_REED_SW,
    GPIO_GPOUT,
    GPIO_LED_GREEN,
    GPIO_LED_RED,
    GPIO_LED_BLUE,
    GPIO_DRDY_M,
    GPIO_INT_M,
    GPIO_DEN_AG,
    GPIO_INT1_AG,
    GPIO_INT2_AG,
    GPIO_FLASH_IO2,
    GPIO_FLASH_IO3,
    GPIO_GPS_EXT_INT,
    GPIO_DFU_BOOT,
    GPIO_TOTAL_NUMBER
} GPIO_Pins_t;

typedef enum
{
    SPI_0,
    SPI_1,
    SPI_2,
    SPI_3,
    SPI_TOTAL_NUMBER
} SPI_t;

typedef enum
{
    I2C_0,
    I2C_1,
    I2C_TOTAL_NUMBER
} I2C_t;

typedef enum
{
    UART_0,
    UART_1,
    UART_TOTAL_NUMBER
} UART_t;

////////////////////////////////// RTC definitions /////////////////////////////////
typedef enum
{
    RTC_RESERVED, // Reserved by the softdevice
    RTC_1,
    RTC_2,
    RTC_TOTAL_NUMBER
} RTC_t;

typedef struct
{
    nrfx_rtc_t rtc;
    uint8_t irq_priority;
} RTC_InitTypeDefAndInst_t;

extern RTC_InitTypeDefAndInst_t RTC_Inits[RTC_TOTAL_NUMBER];

#define FS_DEVICE (0)

#endif /* _BSP_H_ */
