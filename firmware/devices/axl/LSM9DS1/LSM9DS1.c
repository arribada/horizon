/* LSM9DS1.c - HAL for accelerometer device
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

#include "debug.h"
#include "LSM9DS1.h"
#include "sys_config.h"
#include "syshal_axl.h"
#include "syshal_gpio.h"
#include "syshal_i2c.h"
#include "bsp.h"

// Internal variables used to sleep and wake the device
static uint8_t LSM9D1_CTRL_REG6_XL_wake_state;
static uint8_t LSM9D1_CTRL_REG6_XL_power_down_state;
static volatile bool new_data_pending = false;
static volatile bool device_awake = false;

/**
 * @brief      Interrupt handler for when accelerometer data is ready
 */
static void syshal_axl_int1_ag_interrupt_priv(const syshal_gpio_event_t *event)
{
    // This flag is handled externally to the interrupt to ensure that the I2C device does not get used by two functions at once
    new_data_pending = true;
}

/**
 * @brief      Find the closest value in out of the given values in an array
 *
 * @param[in]  target         The target value
 * @param[in]  valid_options  The valid options array
 * @param[in]  length         The number of values in the options array
 *
 * @return     The closest value in the array to the target value
 */
static uint16_t find_closest_value_priv(uint16_t target, const uint16_t * valid_options, uint32_t length)
{
    if (!length)
        return 0;

    // Handle all special/edge cases
    if (1 == length)
        return valid_options[0];

    if (target <= valid_options[0])
        return valid_options[0];

    if (target >= valid_options[length - 1])
        return valid_options[length - 1];

    for (uint32_t i = 0; i < length - 2; ++i)
    {
        // Check if the target is between two options
        if ( (target >= valid_options[i]) && (target <= valid_options[i + 1]) )
        {
            // If it is then return the closest of the two
            if ( (target - valid_options[i]) >= (valid_options[i + 1] - target) )
                return valid_options[i + 1];
            else
                return valid_options[i];
        }
    }

    return 0;
}

/**
 * @brief      Initialises the accelerometer
 *
 * @return     SYSHAL_AXL_PROVISIONING_NEEDED if configuration tags still need
 *             to be set
 * @return     SYSHAL_AXL_NO_ERROR on success
 */
int syshal_axl_init(void)
{
    if (syshal_i2c_is_device_ready(I2C_AXL, LSM9D1_AG_ADDR) != SYSHAL_I2C_NO_ERROR)
    {
        DEBUG_PR_ERROR("LSM9DS1 unresponsive");
        return SYSHAL_AXL_ERROR_DEVICE_UNRESPONSIVE;
    }

    // Fetch all required configuration tags
    int ret;

    ret = sys_config_get(SYS_CONFIG_TAG_AXL_SAMPLE_RATE, NULL);
    if (ret < 0)
        return SYSHAL_AXL_ERROR_PROVISIONING_NEEDED;

    // Enable the Accelerometer axis
    uint8_t reg_value = LSM9D1_CTRL_REG5_XL_ZEN_XL | LSM9D1_CTRL_REG5_XL_YEN_XL | LSM9D1_CTRL_REG5_XL_XEN_XL;
    syshal_i2c_write_reg(I2C_AXL, LSM9D1_AG_ADDR, LSM9D1_CTRL_REG5_XL, &reg_value, 1);

    // The supported options by the LSM9DS1 device
    uint16_t valid_sample_rate_options[6] = {10, 50, 119, 238, 476, 952}; // Number of readings per second

    // Convert the configuration tag options to the closest supported option
    uint16_t sample_rate = find_closest_value_priv(sys_config.axl_sample_rate.contents.sample_rate, valid_sample_rate_options, sizeof(valid_sample_rate_options) / sizeof(*valid_sample_rate_options));

    // Populate the accelerometer CTRL_REG6_XL register
    reg_value = 0;

    switch (sample_rate)
    {
        case 10:
            DEBUG_PR_TRACE("AXL sample rate: 10 Hz");
            reg_value |= LSM9D1_CTRL_REG6_XL_ODR_XL_10HZ;
            break;
        case 50:
            DEBUG_PR_TRACE("AXL sample rate: 50 Hz");
            reg_value |= LSM9D1_CTRL_REG6_XL_ODR_XL_50HZ;
            break;
        case 119:
            DEBUG_PR_TRACE("AXL sample rate: 119 Hz");
            reg_value |= LSM9D1_CTRL_REG6_XL_ODR_XL_119HZ;
            break;
        case 238:
            DEBUG_PR_TRACE("AXL sample rate: 238 Hz");
            reg_value |= LSM9D1_CTRL_REG6_XL_ODR_XL_238HZ;
            break;
        case 476:
            DEBUG_PR_TRACE("AXL sample rate: 476 Hz");
            reg_value |= LSM9D1_CTRL_REG6_XL_ODR_XL_476HZ;
            break;
        case 952:
            DEBUG_PR_TRACE("AXL sample rate: 952 Hz");
            reg_value |= LSM9D1_CTRL_REG6_XL_ODR_XL_952HZ;
            break;
    }

    reg_value |= LSM9D1_CTRL_REG6_XL_FS_XL_4G; // +/- 4G scale

    LSM9D1_CTRL_REG6_XL_wake_state = reg_value;
    LSM9D1_CTRL_REG6_XL_power_down_state = reg_value & (~LSM9D1_CTRL_REG6_XL_ODR_XL_MASK);
    syshal_i2c_write_reg(I2C_AXL, LSM9D1_AG_ADDR, LSM9D1_CTRL_REG6_XL, &LSM9D1_CTRL_REG6_XL_power_down_state, 1);

    // Setup the INT1_A/G interrupt GPIO pin to generate interrupts
    syshal_gpio_init(GPIO_INT1_AG);
    syshal_gpio_enable_interrupt(GPIO_INT1_AG, syshal_axl_int1_ag_interrupt_priv);

    // Enable accelerometer data ready interrupt generation on INT1_A/G
    uint8_t LSM9D1_INT1_CTRL_register = LSM9D1_INT1_CTRL_INT_DRDY_XL;
    syshal_i2c_write_reg(I2C_AXL, LSM9D1_AG_ADDR, LSM9D1_INT1_CTRL, &LSM9D1_INT1_CTRL_register, 1);

    // Is there already data waiting to be read?
    new_data_pending = syshal_gpio_get_input(GPIO_INT1_AG);
    device_awake = false;

    return SYSHAL_AXL_NO_ERROR;
}

int syshal_axl_term(void)
{
    if (device_awake)
        syshal_axl_sleep();

    return SYSHAL_AXL_NO_ERROR;
}

/**
 * @brief      Sleeps the accelerometer, halting reading and lowering power
 *             consumption
 *
 * @return     SYSHAL_AXL_NO_ERROR on success
 */
int syshal_axl_sleep(void)
{
    DEBUG_PR_TRACE("Sleeping AXL");
    syshal_i2c_write_reg(I2C_AXL, LSM9D1_AG_ADDR, LSM9D1_CTRL_REG6_XL, &LSM9D1_CTRL_REG6_XL_power_down_state, 1);
    device_awake = false;

    return SYSHAL_AXL_NO_ERROR;
}

/**
 * @brief      Wakes the accelerometer from a sleep state
 *
 * @return     SYSHAL_AXL_NO_ERROR on success
 */
int syshal_axl_wake(void)
{
    DEBUG_PR_TRACE("Waking AXL");
    syshal_i2c_write_reg(I2C_AXL, LSM9D1_AG_ADDR, LSM9D1_CTRL_REG6_XL, &LSM9D1_CTRL_REG6_XL_wake_state, 1);
    device_awake = true;

    return SYSHAL_AXL_NO_ERROR;
}

bool syshal_axl_awake(void)
{
    return device_awake;
}

int syshal_axl_tick(void)
{
    if (new_data_pending)
    {
        // Read the avaliable data from the accelerometer
        uint8_t temp[6];
        syshal_axl_data_t accl_data;
        new_data_pending = false;
        int bytes_read = syshal_i2c_read_reg(I2C_AXL, LSM9D1_AG_ADDR, LSM9D1_OUT_X_L_XL, &temp[0], 6); // Read 6 bytes, beginning at OUT_X_L_XL

        if (bytes_read < 0)
        {
            new_data_pending = true;
            return SYSHAL_AXL_NO_ERROR;
        }

        accl_data.x = (temp[1] << 8) | temp[0];
        accl_data.y = (temp[3] << 8) | temp[2];
        accl_data.z = (temp[5] << 8) | temp[4];
        syshal_axl_callback(accl_data); // Generate a callback event
    }

    return SYSHAL_AXL_NO_ERROR;
}

/**
 * @brief      AXL callback stub, should be overriden by the user application
 *
 * @param[in]  data  The data that was read
 */
__attribute__((weak)) void syshal_axl_callback(syshal_axl_data_t data)
{
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);
}