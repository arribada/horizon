#include <stdbool.h>
#include <stdint.h>
#include "nrfx_rtc.h"

#define NUMBER_COMPARE_CHANNELS (4)

typedef enum
{
    NRFX_DRV_STATE_UNINITIALIZED, ///< Uninitialized.
    NRFX_DRV_STATE_INITIALIZED,   ///< Initialized but powered off.
    NRFX_DRV_STATE_POWERED_ON,    ///< Initialized and powered on.
} nrfx_drv_state_t;

/**@brief RTC driver instance control block structure. */
typedef struct
{
    nrfx_rtc_handler_t handle;
    nrfx_drv_state_t   state;
    nrfx_rtc_config_t  config;
    bool overflow_irq;
    uint32_t counter : 24; // 24 bit internal counter
    struct
    {
        uint32_t value : 24;
        bool irq_enabled;
    } compare_channel[NUMBER_COMPARE_CHANNELS];
} nrfx_rtc_cb_t;

static nrfx_rtc_cb_t      m_cb[NRFX_RTC_ENABLED_COUNT];

nrfx_err_t nrfx_rtc_init(nrfx_rtc_t const * const  p_instance,
                         nrfx_rtc_config_t const * p_config,
                         nrfx_rtc_handler_t        handler)
{
    nrfx_err_t err_code;

    m_cb[p_instance->instance_id].handle = handler;

    if (m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_UNINITIALIZED)
        return NRFX_ERROR_INVALID_STATE;

    memcpy(&m_cb[p_instance->instance_id].config, p_config, sizeof(nrfx_rtc_config_t));

    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_INITIALIZED;
    m_cb[p_instance->instance_id].counter = 0;

    return NRFX_SUCCESS;
}

void nrfx_rtc_uninit(nrfx_rtc_t const * const p_instance)
{
    m_cb[p_instance->instance_id].handle = 0;
    m_cb[p_instance->instance_id].counter = 0;
    m_cb[p_instance->instance_id].overflow_irq = false;

    for (uint32_t i = 0; i < NUMBER_COMPARE_CHANNELS; ++i)
    {
        m_cb[p_instance->instance_id].compare_channel[i].irq_enabled = false;
    }

    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_UNINITIALIZED;
}

void nrfx_rtc_enable(nrfx_rtc_t const * const p_instance)
{
    if (m_cb[p_instance->instance_id].state != NRFX_DRV_STATE_INITIALIZED)
        return;

    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_POWERED_ON;
}

void nrfx_rtc_disable(nrfx_rtc_t const * const p_instance)
{
    if (m_cb[p_instance->instance_id].state == NRFX_DRV_STATE_UNINITIALIZED)
        return;

    m_cb[p_instance->instance_id].state = NRFX_DRV_STATE_INITIALIZED;
}

uint32_t nrfx_rtc_counter_get(nrfx_rtc_t const * const p_instance)
{
    return m_cb[p_instance->instance_id].counter;
}

void nrfx_rtc_counter_clear(nrfx_rtc_t const * const p_instance)
{
    m_cb[p_instance->instance_id].counter = 0;
}

void nrfx_rtc_overflow_enable(nrfx_rtc_t const * const p_instance, bool enable_irq)
{
    m_cb[p_instance->instance_id].overflow_irq = enable_irq;
}

void nrfx_rtc_overflow_disable(nrfx_rtc_t const * const p_instance)
{
    m_cb[p_instance->instance_id].overflow_irq = false;
}

nrfx_err_t nrfx_rtc_cc_set(nrfx_rtc_t const * const p_instance,
                           uint32_t                 channel,
                           uint32_t                 val,
                           bool                     enable_irq)
{
    if (m_cb[p_instance->instance_id].state == NRFX_DRV_STATE_UNINITIALIZED)
        return NRFX_ERROR_INVALID_STATE;

    m_cb[p_instance->instance_id].compare_channel[channel].irq_enabled = enable_irq;
    m_cb[p_instance->instance_id].compare_channel[channel].value = val;

    return NRFX_SUCCESS;
}

uint32_t nrfx_rtc_cc_get(nrfx_rtc_t const * const p_instance, uint32_t channel)
{
    return m_cb[p_instance->instance_id].compare_channel[channel].value;
}

void nrfx_rtc_increment_tick(void)
{
    for (uint32_t i = 0; i < NRFX_RTC_ENABLED_COUNT; ++i)
    {
        if (NRFX_DRV_STATE_POWERED_ON == m_cb[i].state)
        {
            m_cb[i].counter++;

            // Check for overflow conditions
            if (m_cb[i].counter == 0 &&
                m_cb[i].overflow_irq)
            {
                if (m_cb[i].handle)
                    m_cb[i].handle(NRFX_RTC_INT_OVERFLOW);
            }
            
            // Check for compare channel conditions
            for (uint32_t j = 0; j < NUMBER_COMPARE_CHANNELS; ++j)
            {
                if (m_cb[i].compare_channel[j].value == m_cb[i].counter &&
                    m_cb[i].compare_channel[j].irq_enabled)
                {
                    m_cb[i].compare_channel[j].irq_enabled = false;
                    if (m_cb[i].handle)
                        m_cb[i].handle(j);
                }
            }
        }
    }
}

void nrfx_rtc_increment_second(void)
{
    // Loop unrolled for faster execution
    nrfx_rtc_increment_tick();
    nrfx_rtc_increment_tick();
    nrfx_rtc_increment_tick();
    nrfx_rtc_increment_tick();
    nrfx_rtc_increment_tick();
    nrfx_rtc_increment_tick();
    nrfx_rtc_increment_tick();
    nrfx_rtc_increment_tick();
}