/**
  ******************************************************************************
  * @file     syshal_spi.c
  * @brief    System hardware abstraction layer for SPI.
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
#include "syshal_spi.h"
#include "syshal_firmware.h"
#include "debug.h"
#include "bsp.h"

/* Private variables */
static SPI_HandleTypeDef hspi[SPI_TOTAL_NUMBER];

/* HAL to SYSHAL error code mapping table */
static volatile int hal_error_map[] =
{
    SYSHAL_SPI_NO_ERROR,
    SYSHAL_SPI_ERROR_DEVICE,
    SYSHAL_SPI_ERROR_BUSY,
    SYSHAL_SPI_ERROR_TIMEOUT,
};

/**
 * @brief      Initialise the given SPI instance
 *
 * @param[in]  instance  The SPI instance
 */
int syshal_spi_init(uint32_t instance)
{
    HAL_StatusTypeDef status;

    if (instance >= SPI_TOTAL_NUMBER)
        return SYSHAL_SPI_ERROR_INVALID_INSTANCE;

    // Populate internal handlers from bsp
    hspi[instance].Instance = SPI_Inits[instance].Instance;
    hspi[instance].Init = SPI_Inits[instance].Init;

    status = HAL_SPI_Init(&hspi[instance]);

    return hal_error_map[status];
}

/**
 * @brief      Deinitialise the given SPI instance
 *
 * @param[in]  instance  The SPI instance
 */
int syshal_spi_term(uint32_t instance)
{
    HAL_StatusTypeDef status;

    if (instance >= SPI_TOTAL_NUMBER)
        return SYSHAL_SPI_ERROR_INVALID_INSTANCE;

    status = HAL_SPI_DeInit(&hspi[instance]);

    return hal_error_map[status];
}

/**
 * @brief      Transfer the given data on SPI
 *
 * The API will block the caller until the requisite amount of data
 * bytes has been sent and received.
 *
 * @param[in]  instance  The SPI instance
 * @param[in]  data      The data buffer to be sent
 * @param[in]  size      The size of the data buffer in bytes
 *
 * @return SYSHAL_SPI_ERROR_DEVICE on HAL error.
 * @return SYSHAL_SPI_ERROR_BUSY if the HW is busy.
 * @return SYSHAL_SPI_ERROR_TIMEOUT if a timeout occurred.
 */
__RAMFUNC int syshal_spi_transfer(uint32_t instance, uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    if (instance >= SPI_TOTAL_NUMBER)
        return SYSHAL_SPI_ERROR_INVALID_INSTANCE;

    status = HAL_SPI_TransmitReceive(&hspi[instance], tx_data, rx_data, size, SPI_TIMEOUT);

    return hal_error_map[status];
}

// Implement MSP hooks that are called by stm32f0xx_hal_spi
void HAL_SPI_MspInit(SPI_HandleTypeDef * hspi)
{
    if (hspi->Instance == SPI1)
    {
        // Peripheral clock enable
        __HAL_RCC_SPI1_CLK_ENABLE();

        // SPI1 GPIO Configuration
        syshal_gpio_init(GPIO_SPI1_MOSI);
        syshal_gpio_init(GPIO_SPI1_MISO);
        syshal_gpio_init(GPIO_SPI1_SCK);
    }

    if (hspi->Instance == SPI2)
    {
        // Peripheral clock enable
        __HAL_RCC_SPI2_CLK_ENABLE();

        // SPI2 GPIO Configuration
        syshal_gpio_init(GPIO_SPI2_MOSI);
        syshal_gpio_init(GPIO_SPI2_MISO);
        syshal_gpio_init(GPIO_SPI2_SCK);
    }

}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef * hspi)
{

    if (hspi->Instance == SPI1)
    {
        // Peripheral clock disable
        __HAL_RCC_SPI1_CLK_DISABLE();

        // SPI1 GPIO Configuration
        syshal_gpio_term(GPIO_SPI1_MOSI);
        syshal_gpio_term(GPIO_SPI1_MISO);
        syshal_gpio_term(GPIO_SPI1_SCK);
    }

    if (hspi->Instance == SPI2)
    {
        // Peripheral clock disable
        __HAL_RCC_SPI2_CLK_DISABLE();

        // SPI2 GPIO Configuration
        syshal_gpio_term(GPIO_SPI2_MOSI);
        syshal_gpio_term(GPIO_SPI2_MISO);
        syshal_gpio_term(GPIO_SPI2_SCK);
    }

}
