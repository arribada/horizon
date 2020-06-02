/**
  ******************************************************************************
  * @file     syshal_time.c
  * @brief    System hardware abstraction layer for system time.
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

#include "syshal_time.h"
#include "nrfx_timer.h"
#include "nrfx_ppi.h"
#include "bsp.h"
#include "nrf_delay.h"
#include "debug.h"

static nrf_ppi_channel_t ppi_channel;
static bool is_init = false;

void timer_irq(nrf_timer_event_t event_type, void* p_context) 
{
}

void counter_irq(nrf_timer_event_t event_type, void* p_context) 
{
}

int syshal_time_init(void)
{
    int ret;

    const nrfx_timer_config_t timer_config =
    {
        .frequency          = NRF_TIMER_FREQ_1MHz,
        .mode               = NRF_TIMER_MODE_TIMER,
        .bit_width          = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = TIMER_Inits[TIMER_SYSTICK_TIMER].irq_priority,
        .p_context          = NULL
    };

    ret = nrfx_timer_init(&TIMER_Inits[TIMER_SYSTICK_TIMER].timer, &timer_config, timer_irq);
    if (ret != NRFX_SUCCESS)
        return SYSHAL_TIME_ERROR_INIT;

    nrfx_timer_extended_compare(&TIMER_Inits[TIMER_SYSTICK_TIMER].timer, NRF_TIMER_CC_CHANNEL0, nrf_timer_ms_to_ticks(1, NRF_TIMER_FREQ_1MHz), NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

    const nrfx_timer_config_t counter_config =
    {
        .frequency          = NRF_TIMER_FREQ_1MHz,
        .mode               = NRF_TIMER_MODE_LOW_POWER_COUNTER,
        .bit_width          = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = TIMER_Inits[TIMER_SYSTICK_COUNTER].irq_priority,
        .p_context          = NULL
    };

    ret = nrfx_timer_init(&TIMER_Inits[TIMER_SYSTICK_COUNTER].timer, &counter_config, timer_irq);
    if (ret != NRFX_SUCCESS)
        return SYSHAL_TIME_ERROR_INIT;

    ret = nrfx_ppi_channel_alloc(&ppi_channel);
    if (ret != NRFX_SUCCESS)
        return SYSHAL_TIME_ERROR_INIT;

    nrfx_ppi_channel_assign(ppi_channel, nrfx_timer_compare_event_address_get(&TIMER_Inits[TIMER_SYSTICK_TIMER].timer, NRF_TIMER_CC_CHANNEL0), nrfx_timer_task_address_get(&TIMER_Inits[TIMER_SYSTICK_COUNTER].timer, NRF_TIMER_TASK_COUNT));
    nrfx_ppi_channel_fork_assign(ppi_channel, nrfx_timer_capture_task_address_get(&TIMER_Inits[TIMER_SYSTICK_COUNTER].timer, NRF_TIMER_CC_CHANNEL0));
    nrfx_ppi_channel_enable(ppi_channel);

    nrfx_timer_enable(&TIMER_Inits[TIMER_SYSTICK_COUNTER].timer);
    nrfx_timer_enable(&TIMER_Inits[TIMER_SYSTICK_TIMER].timer);

    is_init = true;

    return SYSHAL_TIME_NO_ERROR;
}

int syshal_time_term(void)
{
    if (is_init)
    {
        nrfx_timer_disable(&TIMER_Inits[TIMER_SYSTICK_TIMER].timer);
        nrfx_timer_disable(&TIMER_Inits[TIMER_SYSTICK_COUNTER].timer);

        nrfx_ppi_channel_disable(ppi_channel);
        nrfx_ppi_channel_free(ppi_channel);

        nrfx_timer_uninit(&TIMER_Inits[TIMER_SYSTICK_TIMER].timer);
        nrfx_timer_uninit(&TIMER_Inits[TIMER_SYSTICK_COUNTER].timer);
    }

    is_init = false;

    return SYSHAL_TIME_NO_ERROR;
}

uint32_t syshal_time_get_ticks_ms(void)
{
    return nrf_timer_cc_read(TIMER_Inits[TIMER_SYSTICK_COUNTER].timer.p_reg, NRF_TIMER_CC_CHANNEL0);
}

uint32_t syshal_time_get_ticks_us(void)
{
    DEBUG_PR_TRACE("%s NOT IMPLEMENTED", __FUNCTION__);

    return syshal_time_get_ticks_ms() * 1000;
}

void syshal_time_delay_us(uint32_t us)
{
    nrf_delay_us(us);
}

void syshal_time_delay_ms(uint32_t ms)
{
    nrf_delay_ms(ms);
}
