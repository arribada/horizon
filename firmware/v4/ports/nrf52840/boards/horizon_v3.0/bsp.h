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

#ifndef _BSP_H_
#define _BSP_H_

#define BOARD_MAJOR_VERSION  (3)
#define BOARD_MINOR_VERSION  (0)

#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include "nrfx_spim.h"
#include "nrfx_twim.h"
#include "nrfx_uarte.h"
#include "nrfx_qspi.h"
#include "nrfx_saadc.h"
#include "nrfx_timer.h"
#include "nrfx_pwm.h"
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
#define TIMER_SYSTICK_TIMER   TIMER_3
#define TIMER_SYSTICK_COUNTER TIMER_4
#define RTC_SOFT_WATCHDOG     RTC_1
#define RTC_TIME_KEEPING      RTC_2
#define ADC_BATTERY           ADC_CHANNEL_0
#define SALTWATER_SWITCH      SWITCH_0
#define REED_SWITCH           SWITCH_1
#define DFU_BUTTON            SWITCH_2

// Interrupt priorities (0, 1, 4  are reserved for the softdevice)
#define INTERRUPT_PRIORITY_WATCHDOG  2
#define INTERRUPT_PRIORITY_RTC_1     2
#define INTERRUPT_PRIORITY_RTC_2     3
#define INTERRUPT_PRIORITY_UART_0    4
#define INTERRUPT_PRIORITY_UART_1    4
#define INTERRUPT_PRIORITY_I2C_0     6
#define INTERRUPT_PRIORITY_I2C_1     6
#define INTERRUPT_PRIORITY_SPI_0     6
#define INTERRUPT_PRIORITY_SPI_1     6
#define INTERRUPT_PRIORITY_SPI_2     6
#define INTERRUPT_PRIORITY_SPI_3     6
#define INTERRUPT_PRIORITY_QSPI_0    6
#define INTERRUPT_PRIORITY_TIMER_0   6
#define INTERRUPT_PRIORITY_TIMER_1   6
#define INTERRUPT_PRIORITY_TIMER_2   6
#define INTERRUPT_PRIORITY_TIMER_3   6
#define INTERRUPT_PRIORITY_TIMER_4   6
#define INTERRUPT_PRIORITY_PWM_0     6
#define INTERRUPT_PRIORITY_PWM_1     6
#define INTERRUPT_PRIORITY_PWM_2     6
#define INTERRUPT_PRIORITY_PWM_3     6
#define INTERRUPT_PRIORITY_ADC       6
#define INTERRUPT_PRIORITY_RTC_0     6

///////////////////////////////// GPIO definitions ////////////////////////////////
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

typedef struct
{
    uint32_t             pin_number;
    nrf_gpio_pin_dir_t   dir;
    nrf_gpio_pin_input_t input;
    nrf_gpio_pin_pull_t  pull;
    nrf_gpio_pin_drive_t drive;
    nrf_gpio_pin_sense_t sense;

    nrfx_gpiote_in_config_t gpiote_in_config;
} GPIO_InitTypeDefAndInst_t;

extern GPIO_InitTypeDefAndInst_t GPIO_Inits[GPIO_TOTAL_NUMBER];

///////////////////////////////// SPI definitions /////////////////////////////////
typedef enum
{
#if NRFX_SPIM0_ENABLED
    SPI_0,
#endif
#if NRFX_SPIM1_ENABLED
    SPI_1,
#endif
#if NRFX_SPIM2_ENABLED
    SPI_2,
#endif
#if NRFX_SPIM3_ENABLED
    SPI_3,
#endif
    SPI_TOTAL_NUMBER
} SPI_t;

typedef struct
{
    nrfx_spim_t spim;
    nrfx_spim_config_t spim_config;
} SPI_InitTypeDefAndInst_t;

extern SPI_InitTypeDefAndInst_t SPI_Inits[SPI_TOTAL_NUMBER];

///////////////////////////////// I2C definitions /////////////////////////////////
typedef enum
{
#if NRFX_TWIM0_ENABLED
    I2C_0,
#endif
#if NRFX_TWIM1_ENABLED
    I2C_1,
#endif
    I2C_TOTAL_NUMBER
} I2C_t;

typedef struct
{
    nrfx_twim_t twim;
    nrfx_twim_config_t twim_config;
} I2C_InitTypeDefAndInst_t;

extern const I2C_InitTypeDefAndInst_t I2C_Inits[I2C_TOTAL_NUMBER];

///////////////////////////////// UART definitions ////////////////////////////////
#define UART_RX_BUF_SIZE 512

typedef enum
{
#if NRFX_UARTE0_ENABLED
    UART_0,
#endif
#if NRFX_UARTE1_ENABLED
    UART_1,
#endif
    UART_TOTAL_NUMBER
} UART_t;

typedef struct
{
    nrfx_uarte_t uarte;
    nrfx_uarte_config_t uarte_config;
} UART_InitTypeDefAndInst_t;

extern const UART_InitTypeDefAndInst_t UART_Inits[UART_TOTAL_NUMBER];

///////////////////////////////// QSPI definitions /////////////////////////////////
typedef enum
{
#ifdef NRFX_QSPI_ENABLED
    QSPI_0,
#endif
    QSPI_TOTAL_NUMBER
} QSPI_t;

typedef struct
{
    nrfx_qspi_config_t qspi_config;
} QSPI_InitTypeDefAndInst_t;

extern QSPI_InitTypeDefAndInst_t QSPI_Inits[QSPI_TOTAL_NUMBER];

////////////////////////////////// ADC definitions /////////////////////////////////
typedef enum
{
    ADC_CHANNEL_0,
    ADC_TOTAL_CHANNELS
} ADC_t;

typedef struct
{
    nrfx_saadc_config_t saadc_config;
    nrf_saadc_channel_config_t saadc_config_channel_config[ADC_TOTAL_CHANNELS];
} ADC_InitTypeDefAndInst_t;

extern ADC_InitTypeDefAndInst_t ADC_Inits;

///////////////////////////////// TIMER definitions ////////////////////////////////
typedef enum
{
#if NRFX_CHECK(NRFX_TIMER0_ENABLED)
    TIMER_RESERVED, // Reserved by the softdevice
#endif
#if NRFX_CHECK(NRFX_TIMER1_ENABLED)
    TIMER_1,
#endif
#if NRFX_CHECK(NRFX_TIMER2_ENABLED)
    TIMER_2,
#endif
#if NRFX_CHECK(NRFX_TIMER3_ENABLED)
    TIMER_3,
#endif
#if NRFX_CHECK(NRFX_TIMER4_ENABLED)
    TIMER_4,
#endif
    TIMER_TOTAL_NUMBER
} TIMER_t;

typedef struct
{
    nrfx_timer_t timer;
    uint8_t irq_priority;
} TIMER_InitTypeDefAndInst_t;

extern TIMER_InitTypeDefAndInst_t TIMER_Inits[TIMER_TOTAL_NUMBER];

////////////////////////////////// PWM definitions /////////////////////////////////
typedef enum
{
#if NRFX_CHECK(NRFX_PWM0_ENABLED)
    PWM_0,
#endif
#if NRFX_CHECK(NRFX_PWM1_ENABLED)
    PWM_1,
#endif
#if NRFX_CHECK(NRFX_PWM2_ENABLED)
    PWM_2,
#endif
#if NRFX_CHECK(NRFX_PWM3_ENABLED)
    PWM_3,
#endif
    PWM_TOTAL_NUMBER
} PWM_t;

typedef struct
{
    nrfx_pwm_t pwm;
    uint8_t irq_priority;
} PWM_InitTypeDefAndInst_t;

extern PWM_InitTypeDefAndInst_t PWM_Inits[PWM_TOTAL_NUMBER];

////////////////////////////////// RTC definitions /////////////////////////////////
typedef enum
{
#if NRFX_CHECK(NRFX_RTC0_ENABLED)
    RTC_RESERVED, // Reserved by the softdevice
#endif
#if NRFX_CHECK(NRFX_RTC1_ENABLED)
    RTC_1,
#endif
#if NRFX_CHECK(NRFX_RTC2_ENABLED)
    RTC_2,
#endif
    RTC_TOTAL_NUMBER
} RTC_t;

typedef struct
{
    nrfx_rtc_t rtc;
    uint8_t irq_priority;
} RTC_InitTypeDefAndInst_t;

extern RTC_InitTypeDefAndInst_t RTC_Inits[RTC_TOTAL_NUMBER];

//////////////////////////////// FLASH definitions ////////////////////////////////
#define FS_DEVICE (0)

//////////////////////////////// Switch definitions ///////////////////////////////
typedef enum
{
    SWITCH_0,
    SWITCH_1,
    SWITCH_2,
    SWITCH_TOTAL_NUMBER
} switch_t;

typedef struct
{
    uint32_t gpio_inst;
    uint32_t debounce_time_us;
    bool active_low;
} Switch_InitTypeDefAndInst_t;

extern const Switch_InitTypeDefAndInst_t Switch_Inits[SWITCH_TOTAL_NUMBER];

#endif /* _BSP_H_ */
