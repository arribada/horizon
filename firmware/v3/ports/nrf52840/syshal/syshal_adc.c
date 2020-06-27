/**
  ******************************************************************************
  * @file     syshal_adc.c
  * @brief    System hardware abstraction layer for the ADC.
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

#include <stdbool.h>
#include "syshal_adc.h"
#include "bsp.h"
#include "debug.h"

#define ADC_MAX_VALUE (16384)      // 2^14
#define ADC_REFERENCE (0.6f)       // 0.6v internal reference
#define ADC_GAIN      (1.0f/6.0f)  // 1/6 gain

static bool saadc_init;
static bool saadc_channel_init[ADC_TOTAL_CHANNELS];

static void ssadc_event_handler(nrfx_saadc_evt_t const * p_event)
{
}

int syshal_adc_init(uint32_t instance)
{
    if (instance >= ADC_TOTAL_CHANNELS)
        return SYSHAL_ADC_ERROR_INVALID_INSTANCE;

    if (!saadc_init)
    {
        nrfx_saadc_init(&ADC_Inits.saadc_config, ssadc_event_handler);
        nrfx_saadc_calibrate_offset();

        // Wait for calibrate offset done event
        while (nrfx_saadc_is_busy()) // Wait for calibration to complete
        {}
    }

    saadc_init = true;

    if (!saadc_channel_init[instance])
        nrfx_saadc_channel_init(instance, &ADC_Inits.saadc_config_channel_config[instance]);

    saadc_channel_init[instance] = true;

    return SYSHAL_ADC_NO_ERROR;
}

int syshal_adc_term(uint32_t instance)
{
    if (instance >= ADC_TOTAL_CHANNELS)
        return SYSHAL_ADC_ERROR_INVALID_INSTANCE;

    if (saadc_channel_init[instance])
        nrfx_saadc_channel_uninit(instance);

    saadc_channel_init[instance] = false;

    bool all_channels_term = true;
    for (uint32_t i = 0; i < ADC_TOTAL_CHANNELS; ++i)
        if (saadc_channel_init[i])
            all_channels_term = false;

    if (all_channels_term)
    {
        nrfx_saadc_uninit();
        saadc_init = false;
    }

    return SYSHAL_ADC_NO_ERROR;
}

int syshal_adc_read(uint32_t instance, uint16_t * value)
{
    int16_t raw;

    if (instance >= ADC_TOTAL_CHANNELS)
        return SYSHAL_ADC_ERROR_INVALID_INSTANCE;

    if (!saadc_channel_init[instance] || !saadc_init)
        return SYSHAL_ADC_ERROR_NOT_INITIALISED;

    nrfx_saadc_sample_convert(instance, (nrf_saadc_value_t *) &raw);

    *value = ((float) raw) / ((ADC_GAIN / ADC_REFERENCE) * ADC_MAX_VALUE) * 1000.0f;

    return SYSHAL_ADC_NO_ERROR;
}
