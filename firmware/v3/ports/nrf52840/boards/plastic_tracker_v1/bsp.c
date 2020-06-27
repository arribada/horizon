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

#include "bsp.h"

///////////////////////////////// GPIO definitions ////////////////////////////////
GPIO_InitTypeDefAndInst_t GPIO_Inits[GPIO_TOTAL_NUMBER] =
{
    // pin number, direction, input, pull, drive sense
    /* GPIO_DEBUG       */ {NRF_GPIO_PIN_MAP(0, 11), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_EXT1_GPIO1  */ {NRF_GPIO_PIN_MAP(1, 14), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_D0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_EXT1_GPIO2  */ {NRF_GPIO_PIN_MAP(1, 13), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_EXT1_GPIO3  */ {NRF_GPIO_PIN_MAP(1,  1), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_EXT2_GPIO1  */ {NRF_GPIO_PIN_MAP(1, 15), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_EXT2_GPIO2  */ {NRF_GPIO_PIN_MAP(0, 31), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_EXT2_GPIO3  */ {NRF_GPIO_PIN_MAP(1,  5), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_EXT2_GPIO4  */ {NRF_GPIO_PIN_MAP(0, 30), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_EXT2_GPIO5  */ {NRF_GPIO_PIN_MAP(0, 29), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_EXT2_GPIO6  */ {NRF_GPIO_PIN_MAP(0, 28), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_SWS         */ {NRF_GPIO_PIN_MAP(0,  2), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIO_PIN_NOPULL, false, true, false}},
    /* GPIO_REED_SW     */ {NRF_GPIO_PIN_MAP(1,  3), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {NRF_GPIOTE_POLARITY_HITOLO, NRF_GPIO_PIN_NOPULL, false, true, false}},
    /* GPIO_GPOUT       */ {NRF_GPIO_PIN_MAP(1, 12), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_LED_GREEN   */ {NRF_GPIO_PIN_MAP(1, 10), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_LED_RED     */ {NRF_GPIO_PIN_MAP(1,  7), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_LED_BLUE    */ {NRF_GPIO_PIN_MAP(1,  4), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_DRDY_M      */ {NRF_GPIO_PIN_MAP(0,  4), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_INT_M       */ {NRF_GPIO_PIN_MAP(1,  6), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_DEN_AG      */ {NRF_GPIO_PIN_MAP(0, 17), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_INT1_AG     */ {NRF_GPIO_PIN_MAP(1,  2), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_INT2_AG     */ {NRF_GPIO_PIN_MAP(0, 13), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_FLASH_IO2   */ {NRF_GPIO_PIN_MAP(0, 22), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_FLASH_IO3   */ {NRF_GPIO_PIN_MAP(1,  0), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_GPS_EXT_INT */ {NRF_GPIO_PIN_MAP(1, 11), NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, {}},
    /* GPIO_DFU_BOOT    */ {NRF_GPIO_PIN_MAP(0, 25), NRF_GPIO_PIN_DIR_INPUT,  NRF_GPIO_PIN_INPUT_CONNECT,    NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE, { NRF_GPIOTE_POLARITY_HITOLO, NRF_GPIO_PIN_PULLUP, false, true, false}},
};

///////////////////////////////// SPI definitions /////////////////////////////////
SPI_InitTypeDefAndInst_t SPI_Inits[SPI_TOTAL_NUMBER] =
{
#if NRFX_SPIM0_ENABLED
    {
        .spim = NRFX_SPIM_INSTANCE(0),
        {
            .sck_pin  = NRFX_SPIM_PIN_NOT_USED,
            .mosi_pin = NRFX_SPIM_PIN_NOT_USED,
            .miso_pin = NRFX_SPIM_PIN_NOT_USED,
            .ss_pin   = NRFX_SPIM_PIN_NOT_USED,
            .ss_active_high = false,
            .irq_priority = INTERRUPT_PRIORITY_SPI_0,
            .orc = 0xFF, // Over-run character
            .frequency = NRF_SPIM_FREQ_4M,
            .mode = NRF_SPIM_MODE_0,
            .bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST
        }
    },
#endif
#if NRFX_SPIM1_ENABLED
    {
        .spim = NRFX_SPIM_INSTANCE(1),
        {
            .sck_pin  = NRFX_SPIM_PIN_NOT_USED,
            .mosi_pin = NRFX_SPIM_PIN_NOT_USED,
            .miso_pin = NRFX_SPIM_PIN_NOT_USED,
            .ss_pin   = NRFX_SPIM_PIN_NOT_USED,
            .ss_active_high = false,
            .irq_priority = INTERRUPT_PRIORITY_SPI_1,
            .orc = 0xFF, // Over-run character
            .frequency = NRF_SPIM_FREQ_4M,
            .mode = NRF_SPIM_MODE_0,
            .bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST
        }
    },
#endif
#if NRFX_SPIM2_ENABLED
    {
        .spim = NRFX_SPIM_INSTANCE(2),
        {
            .sck_pin  = NRF_GPIO_PIN_MAP(0, 8),
            .mosi_pin = NRF_GPIO_PIN_MAP(0, 6),
            .miso_pin = NRF_GPIO_PIN_MAP(0, 7),
            .ss_pin   = NRF_GPIO_PIN_MAP(0, 5),
            .ss_active_high = false,
            .irq_priority = INTERRUPT_PRIORITY_SPI_2,
            .orc = 0xFF, // Over-run character
            .frequency = NRF_SPIM_FREQ_8M,
            .mode = NRF_SPIM_MODE_1,
            .bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST
        }
    },
#endif
#if NRFX_SPIM3_ENABLED
    {
        .spim = NRFX_SPIM_INSTANCE(3),
        {
            .sck_pin  = NRF_GPIO_PIN_MAP(0, 19),
            .mosi_pin = NRF_GPIO_PIN_MAP(0, 21),
            .miso_pin = NRF_GPIO_PIN_MAP(0, 23),
            .ss_pin   = NRF_GPIO_PIN_MAP(0, 24),
            .ss_active_high = false,
            .irq_priority = INTERRUPT_PRIORITY_SPI_3,
            .orc = 0xFF, // Over-run character
            .frequency = NRF_SPIM_FREQ_32M,
            .mode = NRF_SPIM_MODE_0,
            .bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST
        }
    }
#endif
};

///////////////////////////////// I2C definitions /////////////////////////////////
const I2C_InitTypeDefAndInst_t I2C_Inits[I2C_TOTAL_NUMBER] =
{
#if NRFX_TWIM0_ENABLED
    {
        .twim = NRFX_TWIM_INSTANCE(0),
        {
            .scl = NRFX_SPIM_PIN_NOT_USED,
            .sda = NRFX_SPIM_PIN_NOT_USED,
            .frequency = 26738688, // 26738688 = 100k, 67108864 = 250k, 104857600 = 400k
            .interrupt_priority = INTERRUPT_PRIORITY_I2C_0,
            .hold_bus_uninit = 0, // Hold pull up state on gpio pins after uninit <0 = Disabled, 1 = Enabled>
        }
    },
#endif
#if NRFX_TWIM1_ENABLED
    {
        .twim = NRFX_TWIM_INSTANCE(1),
        {
            .scl = NRF_GPIO_PIN_MAP(0, 15),
            .sda = NRF_GPIO_PIN_MAP(0, 27),
            .frequency = 104857600, // 26738688 = 100k, 67108864 = 250k, 104857600 = 400k
            .interrupt_priority = INTERRUPT_PRIORITY_I2C_1,
            .hold_bus_uninit = 0, // Hold pull up state on gpio pins after uninit <0 = Disabled, 1 = Enabled>
        }
    }
#endif
};

/////////////////////////////////// UART definitions ////////////////////////////////

// Supported baudrates:
// <323584=> 1200 baud
// <643072=> 2400 baud
// <1290240=> 4800 baud
// <2576384=> 9600 baud
// <3862528=> 14400 baud
// <5152768=> 19200 baud
// <7716864=> 28800 baud
// <8388608=> 31250 baud
// <10289152=> 38400 baud
// <15007744=> 56000 baud
// <15400960=> 57600 baud
// <20615168=> 76800 baud
// <30801920=> 115200 baud
// <61865984=> 230400 baud
// <67108864=> 250000 baud
// <121634816=> 460800 baud
// <251658240=> 921600 baud
// <268435456=> 1000000 baud

const UART_InitTypeDefAndInst_t UART_Inits[UART_TOTAL_NUMBER] =
{
#if NRFX_UARTE0_ENABLED
    {
        .uarte = NRFX_UARTE_INSTANCE(0),
        {
            .pseltxd = NRF_GPIO_PIN_MAP(1, 9),
            .pselrxd = NRF_GPIO_PIN_MAP(1, 8),
            .pselcts = NRF_UARTE_PSEL_DISCONNECTED,
            .pselrts = NRF_UARTE_PSEL_DISCONNECTED,
            .p_context = NULL, // Context passed to interrupt handler
            .hwfc = NRF_UARTE_HWFC_DISABLED,
            .parity = NRF_UARTE_PARITY_EXCLUDED,
            .baudrate = 121634816, // See table above
            .interrupt_priority = INTERRUPT_PRIORITY_UART_0,
        }
    },
#endif
#if NRFX_UARTE1_ENABLED
    {
        .uarte = NRFX_UARTE_INSTANCE(1),
        {
            .pseltxd = NRF_GPIO_PIN_MAP(0, 26),
            .pselrxd = NRF_GPIO_PIN_MAP(0, 14),
            .pselcts = NRF_GPIO_PIN_MAP(0, 16),
            .pselrts = NRF_GPIO_PIN_MAP(0, 20),
            .p_context = NULL, // Context passed to interrupt handler
            .hwfc = NRF_UARTE_HWFC_ENABLED,
            .parity = NRF_UARTE_PARITY_EXCLUDED,
            .baudrate = 30801920, // See table above
            .interrupt_priority = INTERRUPT_PRIORITY_UART_1,
        }
    }
#endif
};

/////////////////////////////////// QSPI definitions ////////////////////////////////

// Supported frequencies
// <0=> 32MHz/1
// <1=> 32MHz/2
// <2=> 32MHz/3
// <3=> 32MHz/4
// <4=> 32MHz/5
// <5=> 32MHz/6
// <6=> 32MHz/7
// <7=> 32MHz/8
// <8=> 32MHz/9
// <9=> 32MHz/10
// <10=> 32MHz/11
// <11=> 32MHz/12
// <12=> 32MHz/13
// <13=> 32MHz/14
// <14=> 32MHz/15
// <15=> 32MHz/16

QSPI_InitTypeDefAndInst_t QSPI_Inits[QSPI_TOTAL_NUMBER] =
{
#ifdef NRFX_QSPI_ENABLED
    {
        {
            .xip_offset = 0, // Address offset in the external memory for Execute in Place operation
            {
                .sck_pin = NRF_GPIO_PIN_MAP(0, 19),
                .csn_pin = NRF_GPIO_PIN_MAP(0, 24),
                .io0_pin = NRF_GPIO_PIN_MAP(0, 21),
                .io1_pin = NRF_GPIO_PIN_MAP(0, 23),
                .io2_pin = NRF_GPIO_PIN_MAP(0, 22),
                .io3_pin = NRF_GPIO_PIN_MAP(1,  0),
            },
            {
                .readoc = NRF_QSPI_READOC_READ4IO, // Number of data lines and opcode used for reading
                .writeoc = NRF_QSPI_WRITEOC_PP4O,  // Number of data lines and opcode used for writing
                .addrmode = NRF_QSPI_ADDRMODE_24BIT,
                .dpmconfig = false, // Deep power-down mode enable
            },
            {
                .sck_delay = 0, // SCK delay in units of 62.5 ns  <0-255>
                .dpmen = false, // Deep power-down mode enable
                .spi_mode = NRF_QSPI_MODE_0,
                .sck_freq = 15, // Frequency divider (see table above)
            },
            .irq_priority = INTERRUPT_PRIORITY_QSPI_0
        }
    }
#endif
};

////////////////////////////////// ADC definitions /////////////////////////////////
ADC_InitTypeDefAndInst_t ADC_Inits =
{
    {
        .resolution = NRF_SAADC_RESOLUTION_14BIT,
        .oversample = NRF_SAADC_OVERSAMPLE_DISABLED,
        .interrupt_priority = INTERRUPT_PRIORITY_ADC,
        .low_power_mode = false
    },
    {
        {
            // ADC_CHANNEL_0
            .resistor_p = NRF_SAADC_RESISTOR_DISABLED,
            .resistor_n = NRF_SAADC_RESISTOR_DISABLED,
            .gain = NRF_SAADC_GAIN1_6,
            .reference = NRF_SAADC_REFERENCE_INTERNAL,
            .acq_time = NRF_SAADC_ACQTIME_40US,
            .mode = NRF_SAADC_MODE_SINGLE_ENDED,
            .burst = NRF_SAADC_BURST_DISABLED,
            .pin_p = NRF_SAADC_INPUT_AIN1,
            .pin_n = NRF_SAADC_INPUT_DISABLED
        }
    }
};

///////////////////////////////// TIMER definitions ////////////////////////////////
TIMER_InitTypeDefAndInst_t TIMER_Inits[TIMER_TOTAL_NUMBER] =
{
#if NRFX_CHECK(NRFX_TIMER0_ENABLED)
    {
        .timer = NRFX_TIMER_INSTANCE(0),
        .irq_priority = INTERRUPT_PRIORITY_TIMER_0
    },
#endif
#if NRFX_CHECK(NRFX_TIMER1_ENABLED)
    {
        .timer = NRFX_TIMER_INSTANCE(1),
        .irq_priority = INTERRUPT_PRIORITY_TIMER_1
    },
#endif
#if NRFX_CHECK(NRFX_TIMER2_ENABLED)
    {
        .timer = NRFX_TIMER_INSTANCE(2),
        .irq_priority = INTERRUPT_PRIORITY_TIMER_2
    },
#endif
#if NRFX_CHECK(NRFX_TIMER3_ENABLED)
    {
        .timer = NRFX_TIMER_INSTANCE(3),
        .irq_priority = INTERRUPT_PRIORITY_TIMER_3
    },
#endif
#if NRFX_CHECK(NRFX_TIMER4_ENABLED)
    {
        .timer = NRFX_TIMER_INSTANCE(4),
        .irq_priority = INTERRUPT_PRIORITY_TIMER_4
    }
#endif
};

////////////////////////////////// PWM definitions /////////////////////////////////
PWM_InitTypeDefAndInst_t PWM_Inits[PWM_TOTAL_NUMBER] =
{
#if NRFX_CHECK(NRFX_PWM0_ENABLED)
    {
        .pwm = NRFX_PWM_INSTANCE(0),
        .irq_priority = INTERRUPT_PRIORITY_PWM_0
    },
#endif
#if NRFX_CHECK(NRFX_PWM1_ENABLED)
    {
        .pwm = NRFX_PWM_INSTANCE(1),
        .irq_priority = INTERRUPT_PRIORITY_PWM_1
    },
#endif
#if NRFX_CHECK(NRFX_PWM2_ENABLED)
    {
        .pwm = NRFX_PWM_INSTANCE(2),
        .irq_priority = INTERRUPT_PRIORITY_PWM_2
    },
#endif
#if NRFX_CHECK(NRFX_PWM3_ENABLED)
    {
        .pwm = NRFX_PWM_INSTANCE(3),
        .irq_priority = INTERRUPT_PRIORITY_PWM_3
    }
#endif
};

////////////////////////////////// RTC definitions /////////////////////////////////
RTC_InitTypeDefAndInst_t RTC_Inits[RTC_TOTAL_NUMBER] =
{
#if NRFX_CHECK(NRFX_RTC0_ENABLED)
    {
        .rtc = NRFX_RTC_INSTANCE(0),
        .irq_priority = INTERRUPT_PRIORITY_RTC_0
    },
#endif
#if NRFX_CHECK(NRFX_RTC1_ENABLED)
    {
        .rtc = NRFX_RTC_INSTANCE(1),
        .irq_priority = INTERRUPT_PRIORITY_RTC_1
    },
#endif
#if NRFX_CHECK(NRFX_RTC2_ENABLED)
    {
        .rtc = NRFX_RTC_INSTANCE(2),
        .irq_priority = INTERRUPT_PRIORITY_RTC_2
    }
#endif
};

/////////////////////////////////// IWDG definitions ////////////////////////////////
//
//// NOTE: This can't be const since it must be accessed from RAM during FW update procedure
//IWDG_HandleTypeDef IWDG_Init =
//{
//    IWDG,
//    {
//        .Prescaler = IWDG_PRESCALER_256,  /* 6 => 40 KHz / 256 pre-scaler = 156.25 Hz tick period */
//        .Reload = 3125,  /* Equates to 20 seconds */
//        .Window = 0x00000FFF
//    }
//};
