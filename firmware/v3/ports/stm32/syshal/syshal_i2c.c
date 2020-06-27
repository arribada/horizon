/**
  ******************************************************************************
  * @file     syshal_i2c.c
  * @brief    System hardware abstraction layer for I2C.
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
#include "syshal_gpio.h"
#include "syshal_i2c.h"
#include "bsp.h"
#include "debug.h"

// HAL to SYSHAL error code mapping table
static int hal_error_map[] =
{
    SYSHAL_I2C_NO_ERROR,
    SYSHAL_I2C_ERROR_DEVICE,
    SYSHAL_I2C_ERROR_BUSY,
    SYSHAL_I2C_ERROR_TIMEOUT,
};

// Private variables
static I2C_HandleTypeDef hi2c[I2C_TOTAL_NUMBER];

/**
 * @brief      Initialise the given I2C instance
 *
 * @param[in]  instance  The I2C instance
 *
 * @return SYSHAL_I2C_ERROR_INVALID_INSTANCE if instance doesn't exist.
 * @return SYSHAL_I2C_ERROR_DEVICE on HAL error.
 * @return SYSHAL_I2C_ERROR_BUSY if the HW is busy.
 * @return SYSHAL_I2C_ERROR_TIMEOUT if a timeout occurred.
 */
int syshal_i2c_init(uint32_t instance)
{
    HAL_StatusTypeDef status;

    if (instance >= I2C_TOTAL_NUMBER)
        return SYSHAL_I2C_ERROR_INVALID_INSTANCE;

    // Populate internal handlers from bsp
    hi2c[instance].Instance = I2C_Inits[instance].Instance;
    hi2c[instance].Init = I2C_Inits[instance].Init;

    status = HAL_I2C_Init(&hi2c[instance]);

    return hal_error_map[status];
}

/**
 * @brief      Terminate the given I2C instance
 *
 * @param[in]  instance  The I2C instance
 *
 * @return SYSHAL_I2C_ERROR_INVALID_INSTANCE if instance doesn't exist.
 * @return SYSHAL_I2C_ERROR_DEVICE on HAL error.
 * @return SYSHAL_I2C_ERROR_BUSY if the HW is busy.
 * @return SYSHAL_I2C_ERROR_TIMEOUT if a timeout occurred.
 */
int syshal_i2c_term(uint32_t instance)
{
    HAL_StatusTypeDef status;

    if (instance >= I2C_TOTAL_NUMBER)
        return SYSHAL_I2C_ERROR_INVALID_INSTANCE;

    // Terminate the I2C device
    status = HAL_I2C_DeInit(&hi2c[instance]);

    return hal_error_map[status];
}

/**
 * @brief      Transfer the given data to a slave
 *
 * @param[in]  instance      The I2C instance
 * @param[in]  slaveAddress  The slave 8-bit address
 * @param[in]  data          The data buffer to be sent
 * @param[in]  size          The size of the data buffer in bytes
 */
void syshal_i2c_transfer(uint32_t instance, uint8_t slaveAddress, uint8_t * data, uint32_t size)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    status = HAL_I2C_Master_Transmit(&hi2c[instance], (uint16_t)(slaveAddress << 1), data, size, I2C_TIMEOUT);

    // We've timed out. Lets try resetting the i2c bus
    if (HAL_TIMEOUT == status)
    {
        DEBUG_PR_WARN("syshal_i2c_receive timeout, reset");
        syshal_i2c_term(instance);
        syshal_i2c_init(instance);
    }

    if (HAL_OK != status)
    {
        DEBUG_PR_ERROR("%s(%lu, 0x%02X, *data, %lu) failed with %d", __FUNCTION__, instance, slaveAddress, size, status);
    }
}

/**
 * @brief      Receive the given data from a slave
 *
 * @param[in]  instance      The I2C instance
 * @param[in]  slaveAddress  The slave 8-bit address
 * @param[out] data          The data buffer to be read into
 * @param[in]  size          The size of the data to be read in bytes
 *
 * @return     The number of bytes read
 */
uint32_t syshal_i2c_receive(uint32_t instance, uint8_t slaveAddress, uint8_t * data, uint32_t size)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    status = HAL_I2C_Master_Receive(&hi2c[instance], (uint16_t)(slaveAddress << 1), data, size, I2C_TIMEOUT);

    // We've timed out. Lets try resetting the i2c bus
    if (HAL_TIMEOUT == status)
    {
        DEBUG_PR_WARN("syshal_i2c_receive timeout, reset");
        syshal_i2c_term(instance);
        syshal_i2c_init(instance);
    }

    // return number of bytes read as best we can tell
    if (HAL_OK != status)
    {
        DEBUG_PR_ERROR("%s(%lu, 0x%02X, *data, %lu) failed with %d", __FUNCTION__, instance, slaveAddress, size, status);
        return 0;
    }

    return size;
}

/**
 * @brief      Read the given register
 *
 * @param[in]  instance      The I2C instance
 * @param[in]  slaveAddress  The slave 8-bit address
 * @param[in]  regAddress    The register address
 * @param[out] data          The data buffer to be read into
 * @param[in]  size          The size of the data to be read in bytes
 *
 * @return     The number of bytes read
 */
int syshal_i2c_read_reg(uint32_t instance, uint8_t slaveAddress, uint8_t regAddress, uint8_t * data, uint32_t size)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    status = HAL_I2C_Mem_Read(&hi2c[instance], (uint16_t)(slaveAddress << 1), regAddress, 1, data, size, I2C_TIMEOUT);

    // We've timed out. Lets try resetting the i2c bus
    if (HAL_TIMEOUT == status)
    {
        DEBUG_PR_WARN("syshal_i2c_read_reg timeout, reset");
        syshal_i2c_term(instance);
        syshal_i2c_init(instance);
    }

    // return number of bytes read as best we can tell
    if (HAL_OK != status)
    {
        DEBUG_PR_ERROR("%s(addr = 0x%02X, reg = 0x%02X) failed with %d", __FUNCTION__, slaveAddress, regAddress, hal_error_map[status]);
        return hal_error_map[status];
    }

    return size;
}

/**
 * @brief      Write to the given register
 *
 * @param[in]  instance      The I2C instance
 * @param[in]  slaveAddress  The slave 8-bit address
 * @param[in]  regAddress    The register address
 * @param[in]  data          The data buffer to be sent
 * @param[in]  size          The size of the data buffer in bytes
 */
int syshal_i2c_write_reg(uint32_t instance, uint8_t slaveAddress, uint8_t regAddress, uint8_t * data, uint32_t size)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    status = HAL_I2C_Mem_Write(&hi2c[instance], (uint16_t)(slaveAddress << 1), regAddress, 1, data, size, I2C_TIMEOUT);

    // We've timed out. Lets try resetting the i2c bus
    if (HAL_TIMEOUT == status)
    {
        DEBUG_PR_WARN("HAL_I2C_Mem_Write timeout, reset");
        syshal_i2c_term(instance);
        syshal_i2c_init(instance);
    }

    if (HAL_OK != status)
    {
        DEBUG_PR_ERROR("%s failed with %d", __FUNCTION__, status);
        return hal_error_map[status];
    }

    return size;
}

int syshal_i2c_is_device_ready(uint32_t instance, uint8_t slaveAddress)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_IsDeviceReady(&hi2c[instance], (uint16_t)(slaveAddress << 1), 2, 2);

    return hal_error_map[status];
}

// Implement MSP hooks that are called by stm32f0xx_hal_i2c
void HAL_I2C_MspInit(I2C_HandleTypeDef * hi2c)
{

    if (hi2c->Instance == I2C1)
    {
        // Peripheral clock enable
        __HAL_RCC_I2C1_CLK_ENABLE();

        // I2C1 GPIO Configuration
        syshal_gpio_init(GPIO_I2C1_SDA);
        syshal_gpio_init(GPIO_I2C1_SCL);
    }
    if (hi2c->Instance == I2C2)
    {
        // Peripheral clock enable
        __HAL_RCC_I2C2_CLK_ENABLE();

        // I2C2 GPIO Configuration
        syshal_gpio_init(GPIO_I2C2_SDA);
        syshal_gpio_init(GPIO_I2C2_SCL);
    }

}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef * hi2c)
{

    if (hi2c->Instance == I2C1)
    {
        // Peripheral clock disable
        __HAL_RCC_I2C1_CLK_DISABLE();

        // I2C1 GPIO Configuration
        syshal_gpio_term(GPIO_I2C1_SDA);
        syshal_gpio_term(GPIO_I2C1_SCL);
    }
    if (hi2c->Instance == I2C2)
    {
        // Peripheral clock disable
        __HAL_RCC_I2C2_CLK_DISABLE();

        // I2C2 GPIO Configuration
        syshal_gpio_term(GPIO_I2C2_SDA);
        syshal_gpio_term(GPIO_I2C2_SCL);
    }

}