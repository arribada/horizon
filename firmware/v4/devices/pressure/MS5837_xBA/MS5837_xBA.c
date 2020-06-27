/* MS5837_xBA.c - Device driver for MS5837_xBA pressure sensors
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

#include <math.h>
#include "MS5837_xBA.h"
#include "syshal_i2c.h"
#include "syshal_time.h"
#include "syshal_pressure.h"
#include "syshal_timer.h"
#include "sys_config.h"
#include "bsp.h"
#include "debug.h"

uint16_t MS5837_xBA_coefficient[8];
const uint8_t resolution_priv = CMD_ADC_256; // The conversion resolution
static timer_handle_t MS5837_sampling_timer_priv;
static bool initialised_priv = false;

static void MS5837_timer_callback(void);

int syshal_pressure_init(void)
{
    // Read PROM contents
    MS5837_xBA_read_prom(&MS5837_xBA_coefficient[0]);

    // The last 4 bits of the 1st prom region form a CRC error checking code.
    uint8_t crc4_read = (uint8_t) (MS5837_xBA_coefficient[0] >> 12);

    // Calculate the actual crc value
    uint8_t crc4_actual = MS5837_xBA_calculate_crc4(&MS5837_xBA_coefficient[0]);

    // Compare the calculated crc to the one read
    if (crc4_actual != crc4_read)
    {
        DEBUG_PR_TRACE("%s() CRC mismatch: crc4_read = 0x%02X, crc4_actual = 0x%02X", __FUNCTION__, crc4_read, crc4_actual);
        return SYSHAL_PRESSURE_ERROR_CRC_MISMATCH;
    }

    // Create timer for sampling with
    syshal_timer_init(&MS5837_sampling_timer_priv, MS5837_timer_callback);

    initialised_priv = true;

    return SYSHAL_PRESSURE_NO_ERROR;
}

int syshal_pressure_term(void)
{
    if (initialised_priv)
    {
        // Delete sampling timer
        syshal_timer_term(MS5837_sampling_timer_priv);

        initialised_priv = false;
    }

    return SYSHAL_PRESSURE_NO_ERROR;
}

int syshal_pressure_sleep(void)
{
    syshal_timer_cancel(MS5837_sampling_timer_priv);

    return SYSHAL_PRESSURE_NO_ERROR;
}

int syshal_pressure_wake(void)
{
    // Start the sampling timer
    syshal_timer_set(MS5837_sampling_timer_priv, periodic, (uint32_t) round((float) 1000.0f / sys_config.pressure_sample_rate.contents.sample_rate));

    MS5837_timer_callback(); // Manually trigger a reading now so we get a first reading straight away

    return SYSHAL_PRESSURE_NO_ERROR;
}

bool syshal_pressure_awake(void)
{
    return (syshal_timer_running(MS5837_sampling_timer_priv) == SYSHAL_TIMER_RUNNING) && initialised_priv;
}

int syshal_pressure_tick(void)
{
    return SYSHAL_PRESSURE_NO_ERROR;
}

uint8_t MS5837_xBA_calculate_crc4(uint16_t prom[])
{
    uint32_t n_rem = 0; // crc reminder

    uint16_t crc_temp = prom[0]; // Save the end of the configuration data

    prom[0] &= 0x0FFF; // CRC byte is replaced by 0
    prom[7] = 0; // Subsidiary value, set to 0

    for (uint32_t i = 0; i < 16; ++i)
    {
        // choose LSB or MSB
        if (i % 2 == 1)
            n_rem ^= (prom[i >> 1]) & 0x00FF;
        else
            n_rem ^= (prom[i >> 1] >> 8);

        for (uint32_t j = 8; j > 0; j--) // For each bit in a byte
        {
            if (n_rem & (0x8000))
                n_rem = (n_rem << 1) ^ 0x3000;
            else
                n_rem = (n_rem << 1);
        }
    }

    n_rem = (0x000F & (n_rem >> 12)); // final 4-bit reminder is CRC code
    prom[0] = crc_temp; // restore the crc_read to its original place

    return (uint8_t) (n_rem ^ 0x00);
}

void MS5837_xBA_send_command(uint8_t command)
{
    syshal_i2c_transfer(I2C_PRESSURE, MS5837_I2C_ADDRESS, &command, sizeof(command));
}

void MS5837_xBA_read_prom(uint16_t prom[])
{
    for (uint32_t i = 0; i < 7; ++i)
    {
        uint8_t read_buffer[2];

        // The PROM starts at address 0xA0
        MS5837_xBA_send_command(CMD_PROM | (i * 2));
        syshal_i2c_receive(I2C_PRESSURE, MS5837_I2C_ADDRESS, read_buffer, 2);

        prom[i] = (uint16_t) (((uint16_t)read_buffer[0] << 8) + read_buffer[1]);
    }
}

// Retrieve ADC measurement from the device.
// Select measurement type and precision
uint32_t MS5837_xBA_get_adc(uint8_t measurement, uint8_t precision)
{
    // Initiate a ADC read
    MS5837_xBA_send_command(CMD_ADC_CONV | measurement | precision);

    // Wait for conversion to complete
    syshal_time_delay_ms(1); // general delay
    switch (precision)
    {
        case CMD_ADC_256:  syshal_time_delay_ms(1); break;
        case CMD_ADC_512:  syshal_time_delay_ms(3); break;
        case CMD_ADC_1024: syshal_time_delay_ms(4); break;
        case CMD_ADC_2048: syshal_time_delay_ms(6); break;
        case CMD_ADC_4096: syshal_time_delay_ms(10); break;
        case CMD_ADC_8192: syshal_time_delay_ms(20); break;
    }

    // Issue an ADC read command
    MS5837_xBA_send_command(CMD_ADC_READ);

    // Read the ADC values
    uint8_t buffer[3];
    syshal_i2c_receive(I2C_PRESSURE, MS5837_I2C_ADDRESS, (uint8_t *) &buffer[0], sizeof(buffer));

    return ((uint32_t)buffer[0] << 16) + ((uint32_t)buffer[1] << 8) + buffer[2];
}

void MS5837_timer_callback(void)
{
    syshal_pressure_callback(MS5837_get_pressure());
}

/**
 * @brief      Pressure callback stub, should be overriden by the user application
 *
 * @param[in]  data  The data that was read
 */
__attribute__((weak)) void syshal_pressure_callback(int32_t pressure)
{
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);
}