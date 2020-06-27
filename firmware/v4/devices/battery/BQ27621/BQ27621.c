/* BQ27621.c - HAL for battery monitoring using the BQ27621-G1 fuel gauge
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

// Datasheet: http://www.ti.com/product/bq27621-g1

#include <stdbool.h>
#include "BQ27621.h"
#include "syshal_i2c.h"
#include "syshal_batt.h"
#include "syshal_time.h"
#include "syshal_gpio.h"
#include "syshal_rtc.h"
#include "syshal_adc.h"
#include "debug.h"
#include "bsp.h"

//////////////////////////////////////// Internal functions ////////////////////////////////////////

// Read and return the contents of the flag register
#define ERRORVAL -1
#define BQ27621_TIME_BETWEEN_TRANSFERS_MS (1)
#define BQ27621_NUM_OF_ZERO_CHARGE_REQUIRED (10)

#define VOLTAGE_DIV_R1 (680000)
#define VOLTAGE_DIV_R2 (1000000)

#define DELAY_BETWEEN_READS_S (2)

static uint32_t number_of_charge_levels_zero;
static uint32_t last_transfer;
static bool device_found;

static inline int BQ27621_write(uint8_t regAddress, uint16_t value)
{
    uint8_t data[2];
    data[1] = (uint8_t) (value >> 8);
    data[0] = (uint8_t) (value & 0xFF);
    return syshal_i2c_write_reg(I2C_BATTERY, BQ27621_ADDR, regAddress, data, 2);
}

static inline int BQ27621_read(uint8_t regAddress, uint16_t * value)
{
    uint8_t read_bytes[2];
    int length = syshal_i2c_read_reg(I2C_BATTERY, BQ27621_ADDR, regAddress, read_bytes, sizeof(read_bytes));

    if (2 != length)
        return length;

    *value = (uint16_t) ( ((uint16_t)(read_bytes[1]) << 8) | ((uint16_t)(read_bytes[0]) & 0xff) );

    return 0;
}

static inline int BQ27621_read_flags(uint16_t * flags)
{
    return BQ27621_read(BQ27621_REG_FLAGS, flags);
}

static int BQ27621_cfgupdate(bool active)
{
    const uint32_t limit = 100;
    uint32_t try = limit;
    uint16_t status = 0;
    uint16_t cmd = active ? BQ27621_SET_CFGUPDATE : BQ27621_SOFT_RESET;

    BQ27621_write(BQ27621_REG_CTRL, cmd);

    BQ27621_read_flags(&status);

    do
    {
        syshal_time_delay_ms(1);
        BQ27621_read_flags(&status);
    }
    while ((!!(status & BQ27621_FLAG_CFGUPD)) != active && --try);

    if (!try)
    {
        DEBUG_PR_ERROR("Timed out waiting for cfgupdate flag %d", active);
        return ERRORVAL;
    }

    if ((limit - try) > 3)
    {
        DEBUG_PR_WARN("Cfgupdate %d, retries %lu", active, limit - try);
    }

    return 0;
}

static inline int BQ27621_set_cfgupdate(void)
{
    int status = BQ27621_cfgupdate(true);
    if (ERRORVAL == status)
        DEBUG_PR_ERROR("Bus error on set_cfgupdate: %d", status);

    return status;
}

static inline int BQ27621_soft_reset(void)
{
    DEBUG_PR_TRACE("%s", __FUNCTION__);
    int status = BQ27621_cfgupdate(false);
    if (ERRORVAL == status)
        DEBUG_PR_ERROR("Bus error on soft_reset: %d", status);

    return status;
}

//////////////////////////////////////// Exposed functions ////////////////////////////////////////

int syshal_batt_init(void)
{
    uint16_t flags = 0;
    uint32_t retries = 5;

    device_found = false;

    DEBUG_PR_TRACE("BQ27621 init");

    syshal_gpio_init(GPIO_GPOUT);

    syshal_adc_init(ADC_BATTERY);

    number_of_charge_levels_zero = 0;

    if (syshal_i2c_is_device_ready(I2C_BATTERY, BQ27621_ADDR) != SYSHAL_I2C_NO_ERROR)
    {
        DEBUG_PR_ERROR("BQ27621 unresponsive");
        return SYSHAL_BATT_ERROR_DEVICE_UNRESPONSIVE;
    }

    // Issue a soft reset if the ITPOR flag is set to clear it
    do
    {
        BQ27621_soft_reset();
        BQ27621_read_flags(&flags);
        retries--;

        if (retries == 0)
        {
            DEBUG_PR_ERROR("BQ27621 unresponsive");
            return SYSHAL_BATT_ERROR_DEVICE_UNRESPONSIVE;
        }
    }
    while (flags & BQ27621_FLAG_ITPOR);

    syshal_rtc_get_uptime(&last_transfer);

    device_found = true;

    return SYSHAL_BATT_NO_ERROR;
}

int syshal_batt_voltage(uint16_t * voltage)
{
    syshal_adc_read(ADC_BATTERY, voltage);

    *voltage = ((float) *voltage * (VOLTAGE_DIV_R1 + VOLTAGE_DIV_R2)) / VOLTAGE_DIV_R2;

    DEBUG_PR_TRACE("Read voltage: %u mV", *voltage);
    return SYSHAL_BATT_NO_ERROR;
}

/**
 * @brief      Returns the percentage charge left in the battery
 *
 * @return     0 -> 100%
 */
int syshal_batt_level(uint8_t * level)
{
    uint32_t current_transfer;
    int status;

    static uint8_t last_level = 0xFF;

    if (!device_found)
        return SYSHAL_BATT_ERROR_DEVICE_UNRESPONSIVE;

    syshal_rtc_get_uptime(&current_transfer);

    if (current_transfer - last_transfer < DELAY_BETWEEN_READS_S)
    {
        *level = last_level; // It is not safe to ask the device for another reading so soon so use our last value
        return SYSHAL_BATT_NO_ERROR;
    }

    last_transfer = current_transfer;

    status = syshal_i2c_read_reg(I2C_BATTERY, BQ27621_ADDR, BQ27621_REG_STATE_OF_CHARGE, level, sizeof(*level));

    if (SYSHAL_I2C_ERROR_TIMEOUT == status)
    {
        DEBUG_PR_TRACE("SYSHAL_BATT_ERROR_TIMEOUT");
        return SYSHAL_BATT_ERROR_TIMEOUT;
    }
    else if (status <= 0)
    {
        DEBUG_PR_TRACE("SYSHAL_BATT_ERROR_DEVICE_UNRESPONSIVE");
        return SYSHAL_BATT_ERROR_DEVICE_UNRESPONSIVE;
    }

    DEBUG_PR_TRACE("Read battery level: %u", *level);

    if (0 == *level)
    {
        number_of_charge_levels_zero++;
        DEBUG_PR_WARN("BQ27621 zero reading");
        if (number_of_charge_levels_zero < BQ27621_NUM_OF_ZERO_CHARGE_REQUIRED)
            *level = last_level; // Don't return a level of 0 until we have read a fair few in a row to be certain
        else
            last_level = *level;
    }
    else
    {
        last_level = *level;
        number_of_charge_levels_zero = 0;
    }

    return SYSHAL_BATT_NO_ERROR;
}