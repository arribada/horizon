/**
  ******************************************************************************
  * @file     syshal_qspi.c
  * @brief    System hardware abstraction layer for QSPI.
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

#include "syshal_qspi.h"
#include "nrfx_qspi.h"
#include "bsp.h"

int syshal_qspi_init(uint32_t instance)
{
    if (instance >= QSPI_TOTAL_NUMBER)
        return SYSHAL_QSPI_ERROR_INVALID_INSTANCE;

    nrfx_qspi_init(&QSPI_Inits[instance].qspi_config, NULL, NULL);

    return SYSHAL_QSPI_NO_ERROR;
}

int syshal_qspi_term(uint32_t instance)
{
    if (instance >= QSPI_TOTAL_NUMBER)
        return SYSHAL_QSPI_ERROR_INVALID_INSTANCE;

    nrfx_qspi_uninit();

    return SYSHAL_QSPI_NO_ERROR;
}

int syshal_qspi_read(uint32_t instance, uint32_t src_address, uint8_t * data, uint32_t size)
{
    if (instance >= QSPI_TOTAL_NUMBER)
        return SYSHAL_QSPI_ERROR_INVALID_INSTANCE;

    nrfx_qspi_read(data, size, src_address);

    return SYSHAL_QSPI_NO_ERROR;
}

int syshal_qspi_write(uint32_t instance, uint32_t dst_address, const uint8_t * data, uint32_t size)
{
    if (instance >= QSPI_TOTAL_NUMBER)
        return SYSHAL_QSPI_ERROR_INVALID_INSTANCE;

    nrfx_qspi_write(data, size, dst_address);

    return SYSHAL_QSPI_NO_ERROR;
}

int syshal_qspi_erase(uint32_t instance, uint32_t start_address, syshal_qspi_erase_size_t size)
{
    if (instance >= QSPI_TOTAL_NUMBER)
        return SYSHAL_QSPI_ERROR_INVALID_INSTANCE;

    nrf_qspi_erase_len_t length;
    switch (size)
    {
        case SYSHAL_QSPI_ERASE_SIZE_4KB:
            length = QSPI_ERASE_LEN_LEN_4KB;
            break;

        case SYSHAL_QSPI_ERASE_SIZE_64KB:
            length = QSPI_ERASE_LEN_LEN_64KB;
            break;

        case SYSHAL_QSPI_ERASE_SIZE_ALL:
            length = QSPI_ERASE_LEN_LEN_All;
            break;

        default:
            return SYSHAL_QSPI_INVALID_PARM;
            break;
    }

    nrfx_qspi_erase(length, start_address);

    return SYSHAL_QSPI_NO_ERROR;
}

int syshal_qspi_transfer(uint32_t instance, uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    if (instance >= QSPI_TOTAL_NUMBER)
        return SYSHAL_QSPI_ERROR_INVALID_INSTANCE;

    nrf_qspi_cinstr_conf_t config = NRFX_QSPI_DEFAULT_CINSTR(tx_data[0], size);

    config.io3_level = true; // Keep IO3 high during transmission of the OP code

    nrfx_qspi_cinstr_xfer(&config, &tx_data[1], rx_data);

    return SYSHAL_QSPI_NO_ERROR;
}