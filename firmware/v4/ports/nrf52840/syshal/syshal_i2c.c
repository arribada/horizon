/**
  ******************************************************************************
  * @file     syshal_i2c.c
  * @brief    System hardware abstraction layer for I2C.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2019 Arribada</center></h2>
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

#include "syshal_i2c.h"
#include "syshal_pmu.h"
#include "nrfx_twim.h"
#include "nrf_gpio.h"
#include "debug.h"
#include "bsp.h"

static volatile nrfx_twim_evt_t last_twim_event;

static void twim_event_handler(nrfx_twim_evt_t const * p_event, void * p_context)
{
    last_twim_event = *p_event;
}

static uint32_t twim_write(uint32_t instance, uint8_t slaveAddress, const uint8_t * buffer, uint32_t length, bool stop)
{
    nrfx_twim_xfer_desc_t xfer_desc = NRFX_TWIM_XFER_DESC_TX(slaveAddress, (uint8_t *) buffer, length);
    uint32_t flags = stop ? 0 : NRFX_TWIM_FLAG_TX_NO_STOP;

    APP_ERROR_CHECK(nrfx_twim_xfer(&I2C_Inits[instance].twim, &xfer_desc, flags));
    while (nrfx_twim_is_busy(&I2C_Inits[instance].twim))
        syshal_pmu_sleep(SLEEP_LIGHT);

    return length;
}

static uint32_t twim_read(uint32_t instance, uint8_t slaveAddress, uint8_t * buffer, uint32_t length)
{
    nrfx_twim_xfer_desc_t xfer_desc = NRFX_TWIM_XFER_DESC_RX(slaveAddress, buffer, length);

    APP_ERROR_CHECK(nrfx_twim_xfer(&I2C_Inits[instance].twim, &xfer_desc, 0));
    while (nrfx_twim_is_busy(&I2C_Inits[instance].twim))
        syshal_pmu_sleep(SLEEP_LIGHT);

    return length;
}

int syshal_i2c_init(uint32_t instance)
{
    if (instance >= I2C_TOTAL_NUMBER)
        return SYSHAL_I2C_ERROR_INVALID_INSTANCE;

    nrfx_twim_init(&I2C_Inits[instance].twim, &I2C_Inits[instance].twim_config, twim_event_handler, NULL);

    // Disable the internal pullups that nrfx_twim_init() has enabled
    nrf_gpio_cfg(I2C_Inits[instance].twim_config.scl, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
    nrf_gpio_cfg(I2C_Inits[instance].twim_config.sda, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);

    nrfx_twim_enable(&I2C_Inits[instance].twim);

    // Check to see if either the SDA or SCL pins are held low
    // If they are then the I2C bus won't operate
    if (!nrf_gpio_pin_read(I2C_Inits[instance].twim_config.sda))
    {
        DEBUG_PR_ERROR("I2C_%lu SDA pin stuck low!", instance);
        return SYSHAL_I2C_ERROR_INTERFACE;
    }

    if (!nrf_gpio_pin_read(I2C_Inits[instance].twim_config.scl))
    {
        DEBUG_PR_ERROR("I2C_%lu SCL pin stuck low!", instance);
        return SYSHAL_I2C_ERROR_INTERFACE;
    }

    return SYSHAL_I2C_NO_ERROR;
}

int syshal_i2c_term(uint32_t instance)
{
    if (instance >= I2C_TOTAL_NUMBER)
        return SYSHAL_I2C_ERROR_INVALID_INSTANCE;

    nrfx_twim_uninit(&I2C_Inits[instance].twim);

    return SYSHAL_I2C_NO_ERROR;
}

int syshal_i2c_transfer(uint32_t instance, uint8_t slaveAddress, uint8_t * data, uint32_t size)
{
    if (instance >= I2C_TOTAL_NUMBER)
        return SYSHAL_I2C_ERROR_INVALID_INSTANCE;

    return twim_write(instance, slaveAddress, data, size, true);
}

uint32_t syshal_i2c_receive(uint32_t instance, uint8_t slaveAddress, uint8_t * data, uint32_t size)
{
    if (instance >= I2C_TOTAL_NUMBER)
        return SYSHAL_I2C_ERROR_INVALID_INSTANCE;

    return twim_read(instance, slaveAddress, data, size);
}

int syshal_i2c_read_reg(uint32_t instance, uint8_t slaveAddress, uint8_t regAddress, uint8_t * data, uint32_t size)
{
    if (instance >= I2C_TOTAL_NUMBER)
        return SYSHAL_I2C_ERROR_INVALID_INSTANCE;

    twim_write(instance, slaveAddress, &regAddress, sizeof(regAddress), false);
    uint32_t rx_length = twim_read(instance, slaveAddress, data, size);

    if (NRFX_TWIM_EVT_DONE != last_twim_event.type)
        return SYSHAL_I2C_ERROR_TIMEOUT;
    else
        return rx_length;
}

int syshal_i2c_write_reg(uint32_t instance, uint8_t slaveAddress, uint8_t regAddress, uint8_t * data, uint32_t size)
{
    if (instance >= I2C_TOTAL_NUMBER)
        return SYSHAL_I2C_ERROR_INVALID_INSTANCE;

    nrfx_twim_xfer_desc_t xfer_desc = NRFX_TWIM_XFER_DESC_TXTX(slaveAddress, &regAddress, 1, data, size);

    APP_ERROR_CHECK(nrfx_twim_xfer(&I2C_Inits[instance].twim, &xfer_desc, 0));
    while (nrfx_twim_is_busy(&I2C_Inits[instance].twim))
        syshal_pmu_sleep(SLEEP_LIGHT);

    if (NRFX_TWIM_EVT_DONE != last_twim_event.type)
        return SYSHAL_I2C_ERROR_TIMEOUT;
    else
        return size;
}

int syshal_i2c_is_device_ready(uint32_t instance, uint8_t slaveAddress)
{
    if (instance >= I2C_TOTAL_NUMBER)
        return SYSHAL_I2C_ERROR_INVALID_INSTANCE;

    uint8_t dummy_buffer;

    twim_read(instance, slaveAddress, &dummy_buffer, sizeof(dummy_buffer));

    if (NRFX_TWIM_EVT_DONE != last_twim_event.type)
        return SYSHAL_I2C_ERROR_TIMEOUT;
    else
        return SYSHAL_I2C_NO_ERROR;
}
