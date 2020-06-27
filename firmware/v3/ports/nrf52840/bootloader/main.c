/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup bootloader_secure_ble main.c
 * @{
 * @ingroup dfu_bootloader_api
 * @brief Bootloader project main file for secure DFU.
 *
 */

#include <stdint.h>
#include "boards.h"
#include "nrf_mbr.h"
#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_dfu_timers.h"
#include "nrf_dfu.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"
#include "nrfx_timer.h"

const nrfx_timer_t timer = NRFX_TIMER_INSTANCE(1);

#define LED_BLINK_PERIOD_US (1000000)
#define GPIO_LED_RED   (NRF_GPIO_PIN_MAP(1,  7))
#define GPIO_LED_GREEN (NRF_GPIO_PIN_MAP(1, 10))
#define GPIO_LED_BLUE  (NRF_GPIO_PIN_MAP(1,  4))

#define HANDLER_MODE_EXIT 0xFFFFFFF9 // When this is jumped to, the CPU will exit interrupt context
// (handler mode), and pop values from the stack into registers.
// See ARM's documentation for "Exception entry and return".
#define EXCEPTION_STACK_WORD_COUNT 8 // The number of words popped from the stack when
// HANDLER_MODE_EXIT is branched to.

enum
{
    SEQ_RED,
    SEQ_GREEN,
    SEQ_BLUE,
    SEQ_COUNT
} led_sequence = SEQ_RED;

/**@brief Function that sets the stack pointer and link register, and starts executing a particular address.
 *
 * @param[in]  new_msp  The new value to set in the main stack pointer.
 * @param[in]  new_lr   The new value to set in the link register.
 * @param[in]  addr     The address to execute.
 */
#if defined ( __CC_ARM )
__ASM __STATIC_INLINE void jump_to_addr(uint32_t new_msp, uint32_t new_lr, uint32_t addr)
{
    MSR MSP, R0;
    MOV LR,  R1;
    BX       R2;
}
#else
__STATIC_INLINE void jump_to_addr(uint32_t new_msp, uint32_t new_lr, uint32_t addr)
{
    __ASM volatile ("MSR MSP, %[arg]" : : [arg] "r" (new_msp));
    __ASM volatile ("MOV LR,  %[arg]" : : [arg] "r" (new_lr) : "lr");
    __ASM volatile ("BX       %[arg]" : : [arg] "r" (addr));
}
#endif

static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();

#if NRF_MODULE_ENABLED(NRF_LOG_BACKEND_RTT)
    // To allow the buffer to be flushed by the host.
    nrf_delay_ms(100);
#endif
#ifdef NRF_DFU_DEBUG_VERSION
    NRF_BREAKPOINT_COND;
#endif
    NVIC_SystemReset();
}

void timer_evt_handler(nrf_timer_event_t event_type, void * p_context)
{
    led_sequence++;
    if (led_sequence >= SEQ_COUNT)
        led_sequence = SEQ_RED;

    switch (led_sequence)
    {
        case SEQ_RED:
            nrf_gpio_pin_clear(GPIO_LED_RED);
            nrf_gpio_pin_set(GPIO_LED_GREEN);
            nrf_gpio_pin_set(GPIO_LED_BLUE);
            break;
        case SEQ_GREEN:
            nrf_gpio_pin_set(GPIO_LED_RED);
            nrf_gpio_pin_clear(GPIO_LED_GREEN);
            nrf_gpio_pin_set(GPIO_LED_BLUE);
            break;
        case SEQ_BLUE:
            nrf_gpio_pin_set(GPIO_LED_RED);
            nrf_gpio_pin_set(GPIO_LED_GREEN);
            nrf_gpio_pin_clear(GPIO_LED_BLUE);
            break;
        default:
            break;
    }
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    NRF_LOG_ERROR("%s:%d", p_file_name, line_num);
    on_error();
}


void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}


void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
    on_error();
}

/**@brief Function for booting an app as if the chip was reset.
 *
 * @param[in]  vector_table_addr  The address of the app's vector table.
 */
__STATIC_INLINE void app_start(uint32_t vector_table_addr)
{
    const uint32_t current_isr_num = (__get_IPSR() & IPSR_ISR_Msk);
    const uint32_t new_msp         = *((uint32_t *)(vector_table_addr));                    // The app's Stack Pointer is found as the first word of the vector table.
    const uint32_t reset_handler   = *((uint32_t *)(vector_table_addr + sizeof(uint32_t))); // The app's Reset Handler is found as the second word of the vector table.
    const uint32_t new_lr          = 0xFFFFFFFF;

    __set_CONTROL(0x00000000);   // Set CONTROL to its reset value 0.
    __set_PRIMASK(0x00000000);   // Set PRIMASK to its reset value 0.
    __set_BASEPRI(0x00000000);   // Set BASEPRI to its reset value 0.
    __set_FAULTMASK(0x00000000); // Set FAULTMASK to its reset value 0.

    if (current_isr_num == 0)
    {
        // The CPU is in Thread mode (main context).
        jump_to_addr(new_msp, new_lr, reset_handler); // Jump directly to the App's Reset Handler.
    }
    else
    {
        // The CPU is in Handler mode (interrupt context).

        const uint32_t exception_stack[EXCEPTION_STACK_WORD_COUNT] = // To be copied onto the stack.
        {
            0x00000000,    // New value of R0. Cleared by setting to 0.
            0x00000000,    // New value of R1. Cleared by setting to 0.
            0x00000000,    // New value of R2. Cleared by setting to 0.
            0x00000000,    // New value of R3. Cleared by setting to 0.
            0x00000000,    // New value of R12. Cleared by setting to 0.
            0xFFFFFFFF,    // New value of LR. Cleared by setting to all 1s.
            reset_handler, // New value of PC. The CPU will continue by executing the App's Reset Handler.
            xPSR_T_Msk,    // New value of xPSR (Thumb mode set).
        };
        const uint32_t exception_sp = new_msp - sizeof(exception_stack);

        memcpy((uint32_t *)exception_sp, exception_stack, sizeof(exception_stack)); // 'Push' exception_stack onto the App's stack.

        jump_to_addr(exception_sp, new_lr, HANDLER_MODE_EXIT); // 'Jump' to the special value to exit handler mode. new_lr is superfluous here.
        // exception_stack will be popped from the stack, so the resulting SP will be the new_msp.
        // Execution will continue from the App's Reset Handler.
    }
}

void nrf_bootloader_app_start_final(uint32_t vector_table_addr)
{
    // Reset the GPIO pin settings
    nrf_gpio_cfg_default(NRF_BL_DFU_ENTER_METHOD_BUTTON_PIN);
    nrf_gpio_cfg_default(GPIO_LED_RED);
    nrf_gpio_cfg_default(GPIO_LED_GREEN);
    nrf_gpio_cfg_default(GPIO_LED_BLUE);

    // Disable our LED timer
    nrfx_timer_disable(&timer);
    nrfx_timer_clear(&timer);
    nrfx_timer_uninit(&timer);

    // Run application
    app_start(vector_table_addr);
}

/**
 * @brief Function notifies certain events in DFU process.
 */
static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
    switch (evt_type)
    {
        case NRF_DFU_EVT_DFU_FAILED:
        case NRF_DFU_EVT_DFU_ABORTED:
        case NRF_DFU_EVT_DFU_INITIALIZED:
        case NRF_DFU_EVT_TRANSPORT_ACTIVATED:
        case NRF_DFU_EVT_DFU_STARTED:
            break;
        default:
            break;
    }
}


/**@brief Function for application main entry. */
int main(void)
{
    uint32_t ret_val;

    (void) NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("Inside main");

    nrf_gpio_cfg_output(GPIO_LED_RED);
    nrf_gpio_cfg_output(GPIO_LED_GREEN);
    nrf_gpio_cfg_output(GPIO_LED_BLUE);

    nrfx_timer_config_t timer_config =
    {
        .frequency          = NRF_TIMER_FREQ_1MHz,
        .mode               = NRF_TIMER_MODE_TIMER,
        .bit_width          = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = 6,
        .p_context          = NULL
    };

    nrfx_timer_init(&timer, &timer_config, timer_evt_handler);

    nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, LED_BLINK_PERIOD_US, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrfx_timer_enable(&timer);

    ret_val = nrf_bootloader_init(dfu_observer);
    APP_ERROR_CHECK(ret_val);

    // Either there was no DFU functionality enabled in this project or the DFU module detected
    // no ongoing DFU operation and found a valid main application.
    // Boot the main application.
    nrf_bootloader_app_start();

    // Should never be reached.
    NRF_LOG_INFO("After main");
}

/**
 * @}
 */
