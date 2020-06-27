/**
  ******************************************************************************
  * @file     syshal_uart.c
  * @brief    System hardware abstraction layer for UART.
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

#include <string.h>
#include "syshal_uart.h"
#include "syshal_pmu.h"
#include "syshal_gpio.h"
#include "nrfx_timer.h"
#include "ring_buffer.h"
#include "debug.h"
#include "bsp.h"

#define UART_TX_TEMP_BUFFER_SIZE (1024)
#define UART_TX_TIMEOUT_MS (200)

static volatile bool timeout_occurred;
static volatile bool rx_pending[UART_TOTAL_NUMBER];
static ring_buffer_t rx_buffer[UART_TOTAL_NUMBER];
static uint8_t rx_data[UART_TOTAL_NUMBER][UART_RX_BUF_SIZE];
static volatile uint8_t rx_byte[UART_TOTAL_NUMBER];
static uint32_t set_baudrate[UART_TOTAL_NUMBER] = {0};
static volatile bool is_init[UART_TOTAL_NUMBER];

static void handle_interrupt(uint32_t instance)
{
    if (nrf_uarte_event_check(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_ENDRX))
    {
        nrf_uarte_event_clear(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_ENDRX);
        rb_safe_insert(&rx_buffer[instance], rx_byte[instance]);

        if (NRF_UARTE_HWFC_ENABLED == UART_Inits[instance].uarte_config.hwfc)
        {
            if (rb_is_full(&rx_buffer[instance]))
            {
                rx_pending[instance] = false;
            }
            else
            {
                rx_pending[instance] = true;
                nrf_uarte_task_trigger(UART_Inits[instance].uarte.p_reg, NRF_UARTE_TASK_STARTRX);
            }
        }
    }
}

void nrfx_uarte_0_irq_handler(void)
{
    handle_interrupt(UART_0);
}

void nrfx_uarte_1_irq_handler(void)
{
    handle_interrupt(UART_1);
}

static void apply_config(uint32_t instance, nrfx_uarte_t const * p_instance, nrfx_uarte_config_t const * p_config)
{
    if (p_config->pseltxd != NRF_UARTE_PSEL_DISCONNECTED)
    {
        nrf_gpio_pin_set(p_config->pseltxd);
        nrf_gpio_cfg_output(p_config->pseltxd);
    }
    if (p_config->pselrxd != NRF_UARTE_PSEL_DISCONNECTED)
    {
        nrf_gpio_cfg_input(p_config->pselrxd, NRF_GPIO_PIN_NOPULL);
    }

    if (!set_baudrate[instance])
        nrf_uarte_baudrate_set(p_instance->p_reg, p_config->baudrate);
    else
        nrf_uarte_baudrate_set(p_instance->p_reg, set_baudrate[instance]);
    nrf_uarte_configure(p_instance->p_reg, p_config->parity, p_config->hwfc);
    nrf_uarte_txrx_pins_set(p_instance->p_reg, p_config->pseltxd, p_config->pselrxd);
    if (p_config->hwfc == NRF_UARTE_HWFC_ENABLED)
    {
        if (p_config->pselcts != NRF_UARTE_PSEL_DISCONNECTED)
        {
            nrf_gpio_cfg_input(p_config->pselcts, NRF_GPIO_PIN_NOPULL);
        }
        if (p_config->pselrts != NRF_UARTE_PSEL_DISCONNECTED)
        {
            nrf_gpio_pin_set(p_config->pselrts);
            nrf_gpio_cfg_output(p_config->pselrts);
        }
        nrf_uarte_hwfc_pins_set(p_instance->p_reg, p_config->pselrts, p_config->pselcts);
    }
}

int syshal_uart_init(uint32_t instance)
{
    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    // Setup rx buffer
    rb_init(&rx_buffer[instance], UART_RX_BUF_SIZE, &rx_data[instance][0]);

    // Turn off output buffering. This ensures printf prints immediately
    if (instance == UART_DEBUG)
        setvbuf(stdout, NULL, _IONBF, 0);

    apply_config(instance, &UART_Inits[instance].uarte, &UART_Inits[instance].uarte_config);

    nrf_uarte_event_clear(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_ENDRX);
    nrf_uarte_event_clear(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_ENDTX);
    nrf_uarte_event_clear(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_ERROR);
    nrf_uarte_event_clear(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_RXTO);
    nrf_uarte_int_enable(UART_Inits[instance].uarte.p_reg, NRF_UARTE_INT_ENDRX_MASK);

    NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(UART_Inits[instance].uarte.p_reg), UART_Inits[instance].uarte_config.interrupt_priority);
    NRFX_IRQ_ENABLE(nrfx_get_irq_number(UART_Inits[instance].uarte.p_reg));

    nrf_uarte_enable(UART_Inits[instance].uarte.p_reg);

    // Only enable continous reception if hardware flow control is disabled
    if (NRF_UARTE_HWFC_DISABLED == UART_Inits[instance].uarte_config.hwfc)
        nrf_uarte_shorts_enable(UART_Inits[instance].uarte.p_reg, NRF_UARTE_SHORT_ENDRX_STARTRX);

    rx_pending[instance] = true;
    nrf_uarte_rx_buffer_set(UART_Inits[instance].uarte.p_reg, (uint8_t *) &rx_byte[instance], 1);
    nrf_uarte_task_trigger(UART_Inits[instance].uarte.p_reg, NRF_UARTE_TASK_STARTRX);

    is_init[instance] = true;

    return SYSHAL_UART_NO_ERROR;
}


static void pins_to_default(nrfx_uarte_t const * p_instance)
{
    /* Reset pins to default states */
    uint32_t txd;
    uint32_t rxd;
    uint32_t rts;
    uint32_t cts;

    txd = nrf_uarte_tx_pin_get(p_instance->p_reg);
    rxd = nrf_uarte_rx_pin_get(p_instance->p_reg);
    rts = nrf_uarte_rts_pin_get(p_instance->p_reg);
    cts = nrf_uarte_cts_pin_get(p_instance->p_reg);

    nrf_uarte_txrx_pins_disconnect(p_instance->p_reg);
    nrf_uarte_hwfc_pins_disconnect(p_instance->p_reg);

    if (txd != NRF_UARTE_PSEL_DISCONNECTED)
        nrf_gpio_cfg_default(txd);
    if (rxd != NRF_UARTE_PSEL_DISCONNECTED)
        nrf_gpio_cfg_default(rxd);
    if (cts != NRF_UARTE_PSEL_DISCONNECTED)
        nrf_gpio_cfg_default(cts);
    if (rts != NRF_UARTE_PSEL_DISCONNECTED)
        nrf_gpio_cfg_default(rts);
}

int syshal_uart_term(uint32_t instance)
{
    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

#define DISABLE_ALL UINT32_MAX
    nrf_uarte_shorts_disable(UART_Inits[instance].uarte.p_reg, DISABLE_ALL);
#undef DISABLE_ALL

    is_init[instance] = false;

    nrf_uarte_disable(UART_Inits[instance].uarte.p_reg);

    nrf_uarte_task_trigger(UART_Inits[instance].uarte.p_reg, NRF_UARTE_TASK_STOPRX);
    nrf_uarte_task_trigger(UART_Inits[instance].uarte.p_reg, NRF_UARTE_TASK_STOPTX);

    nrf_uarte_int_disable(UART_Inits[instance].uarte.p_reg, NRF_UARTE_INT_ENDRX_MASK |
                          NRF_UARTE_INT_ENDTX_MASK |
                          NRF_UARTE_INT_ERROR_MASK |
                          NRF_UARTE_INT_RXTO_MASK);
    NRFX_IRQ_DISABLE(nrfx_get_irq_number(UART_Inits[instance].uarte.p_reg));

    pins_to_default(&UART_Inits[instance].uarte);

    // [NGPT-475] Power cycle the UART peripheral to ensure the lowest current consumption is achieved
    // See https://devzone.nordicsemi.com/f/nordic-q-a/26030/how-to-reach-nrf52840-uarte-current-supply-specification
    if (instance == UART_0)
    {
        *(volatile uint32_t *)0x40002FFC = 0;
        *(volatile uint32_t *)0x40002FFC;
        *(volatile uint32_t *)0x40002FFC = 1;
    }
    else if (instance == UART_1)
    {
        *(volatile uint32_t *)0x40028FFC = 0;
        *(volatile uint32_t *)0x40028FFC;
        *(volatile uint32_t *)0x40028FFC = 1;
    }

    rx_pending[instance] = false;

    // We may or may not want to clear the ringbuffer here with rb_reset()
    // It is worth noting that the original STM32 code did not do this

    return SYSHAL_UART_NO_ERROR;
}

int syshal_uart_change_baud(uint32_t instance, uint32_t baudrate)
{
    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    DEBUG_PR_TRACE("syshal_uart_change_baud(UART_%lu, %lu)", instance, baudrate);

    nrf_uarte_baudrate_t nrf_baudrate;

    switch (baudrate)
    {
        case 1200:
            nrf_baudrate = NRF_UARTE_BAUDRATE_1200;
            break;
        case 2400:
            nrf_baudrate = NRF_UARTE_BAUDRATE_2400;
            break;
        case 4800:
            nrf_baudrate = NRF_UARTE_BAUDRATE_4800;
            break;
        case 9600:
            nrf_baudrate = NRF_UARTE_BAUDRATE_9600;
            break;
        case 14400:
            nrf_baudrate = NRF_UARTE_BAUDRATE_14400;
            break;
        case 19200:
            nrf_baudrate = NRF_UARTE_BAUDRATE_19200;
            break;
        case 28800:
            nrf_baudrate = NRF_UARTE_BAUDRATE_28800;
            break;
        case 31250:
            nrf_baudrate = NRF_UARTE_BAUDRATE_31250;
            break;
        case 38400:
            nrf_baudrate = NRF_UARTE_BAUDRATE_38400;
            break;
        case 56000:
            nrf_baudrate = NRF_UARTE_BAUDRATE_56000;
            break;
        case 57600:
            nrf_baudrate = NRF_UARTE_BAUDRATE_57600;
            break;
        case 76800:
            nrf_baudrate = NRF_UARTE_BAUDRATE_76800;
            break;
        case 115200:
            nrf_baudrate = NRF_UARTE_BAUDRATE_115200;
            break;
        case 230400:
            nrf_baudrate = NRF_UARTE_BAUDRATE_230400;
            break;
        case 250000:
            nrf_baudrate = NRF_UARTE_BAUDRATE_250000;
            break;
        case 460800:
            nrf_baudrate = NRF_UARTE_BAUDRATE_460800;
            break;
        case 921600:
            nrf_baudrate = NRF_UARTE_BAUDRATE_921600;
            break;
        case 1000000:
            nrf_baudrate = NRF_UARTE_BAUDRATE_1000000;
            break;

        default:
            nrf_baudrate = NRF_UARTE_BAUDRATE_9600;
            break;
    }

    nrf_uarte_baudrate_set(UART_Inits[instance].uarte.p_reg, nrf_baudrate);
    set_baudrate[instance] = nrf_baudrate;

    return SYSHAL_UART_NO_ERROR;
}

int syshal_uart_get_baud(uint32_t instance, uint32_t * baudrate)
{
    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    switch (UART_Inits[instance].uarte.p_reg->BAUDRATE)
    {
        case NRF_UARTE_BAUDRATE_1200:
            *baudrate = 1200;
            break;
        case NRF_UARTE_BAUDRATE_2400:
            *baudrate = 2400;
            break;
        case NRF_UARTE_BAUDRATE_4800:
            *baudrate = 4800;
            break;
        case NRF_UARTE_BAUDRATE_9600:
            *baudrate = 9600;
            break;
        case NRF_UARTE_BAUDRATE_14400:
            *baudrate = 14400;
            break;
        case NRF_UARTE_BAUDRATE_19200:
            *baudrate = 19200;
            break;
        case NRF_UARTE_BAUDRATE_28800:
            *baudrate = 28800;
            break;
        case NRF_UARTE_BAUDRATE_31250:
            *baudrate = 31250;
            break;
        case NRF_UARTE_BAUDRATE_38400:
            *baudrate = 38400;
            break;
        case NRF_UARTE_BAUDRATE_56000:
            *baudrate = 56000;
            break;
        case NRF_UARTE_BAUDRATE_57600:
            *baudrate = 57600;
            break;
        case NRF_UARTE_BAUDRATE_76800:
            *baudrate = 76800;
            break;
        case NRF_UARTE_BAUDRATE_115200:
            *baudrate = 115200;
            break;
        case NRF_UARTE_BAUDRATE_230400:
            *baudrate = 230400;
            break;
        case NRF_UARTE_BAUDRATE_250000:
            *baudrate = 250000;
            break;
        case NRF_UARTE_BAUDRATE_460800:
            *baudrate = 460800;
            break;
        case NRF_UARTE_BAUDRATE_921600:
            *baudrate = 921600;
            break;
        case NRF_UARTE_BAUDRATE_1000000:
            *baudrate = 1000000;
            break;

        default:
            *baudrate = 9600;
            break;
    }

    return SYSHAL_UART_NO_ERROR;
}

static bool uart_tx_send(uint32_t instance, uint8_t * data, uint32_t size)
{
    bool endtx;
    bool txstopped;
    uint32_t start_time;

    nrf_uarte_event_clear(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_ENDTX);
    nrf_uarte_event_clear(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_TXSTOPPED);
    nrf_uarte_tx_buffer_set(UART_Inits[instance].uarte.p_reg, data, size);
    nrf_uarte_task_trigger(UART_Inits[instance].uarte.p_reg, NRF_UARTE_TASK_STARTTX);

    start_time = syshal_time_get_ticks_ms();

    do
    {
        endtx     = nrf_uarte_event_check(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_ENDTX);
        txstopped = nrf_uarte_event_check(UART_Inits[instance].uarte.p_reg, NRF_UARTE_EVENT_TXSTOPPED);

        // If CTS is low then assume we are sending data and not to timeout
        // Note: Don't be fooled by nrf_uarte_tx_amount_get() this function only changes after an ENDTX event rendering it useless
        if (nrf_gpio_pin_read(nrf_uarte_cts_pin_get(UART_Inits[instance].uarte.p_reg)) == false)
            start_time = syshal_time_get_ticks_ms();

        // If hardware flow control is enabled use a timeout to prevent sending hanging indefinitely
        if (NRF_UARTE_HWFC_ENABLED == UART_Inits[instance].uarte_config.hwfc)
        {
            if (syshal_time_get_ticks_ms() - start_time > UART_TX_TIMEOUT_MS)
            {
                nrf_uarte_task_trigger(UART_Inits[instance].uarte.p_reg, NRF_UARTE_TASK_STOPTX);
                DEBUG_PR_WARN("%s() timed out", __FUNCTION__);
                return false;
            }
        }
    }
    while ((!endtx) && (!txstopped));

    return true;
}

int syshal_uart_send(uint32_t instance, uint8_t * data, uint32_t size)
{
    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    if (!size)
        return SYSHAL_UART_ERROR_INVALID_SIZE;

    if (nrfx_is_in_ram(data))
    {
        if (!uart_tx_send(instance, data, size))
            return SYSHAL_UART_ERROR_TIMEOUT;
    }
    else
    {
        // UARTE only allows transfers from RAM so we must copy it there
        while (size)
        {
            uint8_t ram_buffer[UART_TX_TEMP_BUFFER_SIZE];
            uint32_t bytes_to_transfer = MIN(size, UART_TX_TEMP_BUFFER_SIZE);

            memcpy(ram_buffer, data, bytes_to_transfer);

            if (!uart_tx_send(instance, ram_buffer, bytes_to_transfer))
                return SYSHAL_UART_ERROR_TIMEOUT;

            size -= bytes_to_transfer;
        }
    }

    return SYSHAL_UART_NO_ERROR;
}

int syshal_uart_receive(uint32_t instance, uint8_t * data, uint32_t size)
{
    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    uint32_t count = rb_occupancy(&rx_buffer[instance]);

    if (size > count)
        size = count;

    for (uint32_t i = 0; i < size; ++i)
    {
        *data++ = rb_remove(&rx_buffer[instance]);
    }

    if (!rx_pending[instance])
    {
        if (!rb_is_full(&rx_buffer[instance]))
        {
            rx_pending[instance] = true;
            nrf_uarte_task_trigger(UART_Inits[instance].uarte.p_reg, NRF_UARTE_TASK_STARTRX);
        }
    }

    return size;
}

bool syshal_uart_peek_at(uint32_t instance, uint8_t * byte, uint32_t location)
{
    if (instance >= UART_TOTAL_NUMBER)
        return false;

    if (syshal_uart_available(instance) < location)
        return false;

    *byte = rb_peek_at(&rx_buffer[instance], location);

    return true;
}

int syshal_uart_flush(uint32_t instance)
{
    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    rb_reset(&rx_buffer[instance]);

    if (!rx_pending[instance])
    {
        rx_pending[instance] = true;
        nrf_uarte_task_trigger(UART_Inits[instance].uarte.p_reg, NRF_UARTE_TASK_STARTRX);
    }

    return SYSHAL_UART_NO_ERROR;
}

uint32_t syshal_uart_available(uint32_t instance)
{
    if (instance >= UART_TOTAL_NUMBER)
        return 0;

    return rb_occupancy(&rx_buffer[instance]); // Return the number of elements stored in the ring buffer
}

static void timer_evt_handler(nrf_timer_event_t event_type, void * p_context)
{
    timeout_occurred = true;
}

int syshal_uart_read_timeout(uint32_t instance, uint8_t * buffer, uint32_t buf_size, uint32_t read_timeout_us, uint32_t last_char_timeout_us, uint32_t * bytes_received)
{
    ret_code_t ret;

    *bytes_received = 0;

    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    // Set up our timeout timer
    nrfx_timer_config_t timer_config =
    {
        .frequency          = NRF_TIMER_FREQ_1MHz,
        .mode               = NRF_TIMER_MODE_TIMER,
        .bit_width          = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = TIMER_Inits[TIMER_UART_TIMEOUT].irq_priority,
        .p_context          = NULL
    };

    timeout_occurred = false;
    bool first_byte_received;

    ret = nrfx_timer_init(&TIMER_Inits[TIMER_UART_TIMEOUT].timer, &timer_config, timer_evt_handler);
    if (ret != NRFX_SUCCESS)
        return SYSHAL_UART_ERROR_DEVICE;

    // If we already have data then we can assume data is incoming and use the shorter last_char timeout
    if (syshal_uart_available(instance))
    {
        nrfx_timer_compare(&TIMER_Inits[TIMER_UART_TIMEOUT].timer, NRF_TIMER_CC_CHANNEL0, last_char_timeout_us, true);
        first_byte_received = true;
    }
    else
    {
        nrfx_timer_compare(&TIMER_Inits[TIMER_UART_TIMEOUT].timer, NRF_TIMER_CC_CHANNEL0, read_timeout_us, true);
        first_byte_received = false;
    }

    nrfx_timer_clear(&TIMER_Inits[TIMER_UART_TIMEOUT].timer);
    nrfx_timer_enable(&TIMER_Inits[TIMER_UART_TIMEOUT].timer);

    uint32_t bytes_left_to_read = buf_size;
    while (bytes_left_to_read)
    {
        if (timeout_occurred)
            break;

        // Have we received any data in to our ringbuffer?
        if (syshal_uart_available(instance))
        {
            // If this is our first byte
            if (!first_byte_received)
            {
                // Then start using the last_char_timeout_us instead
                nrf_timer_cc_write(TIMER_Inits[TIMER_UART_TIMEOUT].timer.p_reg, NRF_TIMER_CC_CHANNEL0, last_char_timeout_us);
                first_byte_received = true;
            }

            // Kick/clear our timeout to stop it firing
            nrfx_timer_clear(&TIMER_Inits[TIMER_UART_TIMEOUT].timer);

            // Fetch the received data
            uint32_t bytes_in_buff = syshal_uart_available(instance);

            // Don't read more data then our given buffer has room for
            if (bytes_in_buff > bytes_left_to_read)
                bytes_in_buff = bytes_left_to_read;

            syshal_uart_receive(instance, buffer, bytes_in_buff);
            buffer += bytes_in_buff;
            *bytes_received += bytes_in_buff;
            bytes_left_to_read -= bytes_in_buff;

            // If we have all our data escape early to avoid accidental sleeping
            if (!bytes_left_to_read)
                break;
        }
        else
        {
            syshal_pmu_sleep(SLEEP_LIGHT);
        }
    }

    // Disable and stop the timer
    nrfx_timer_disable(&TIMER_Inits[TIMER_UART_TIMEOUT].timer);
    nrfx_timer_uninit(&TIMER_Inits[TIMER_UART_TIMEOUT].timer);

    if (timeout_occurred)
        return SYSHAL_UART_ERROR_TIMEOUT;
    else
        return SYSHAL_UART_NO_ERROR;
}

int _write(int file, char *ptr, int len)
{
    // If the UART is not initialised ignore any printf calls to prevent code hangs
    if (!is_init[UART_DEBUG])
        return len;

    uint32_t last_tx_pin = UART_Inits[UART_DEBUG].uarte.p_reg->PSEL.TXD;

    // Change our TX pin to that of our printf output
    // WARN: This is undefined behaviour as the documentation says you should disable the UART instance first
    // It is however not preferable to do this as it will mess with the RX reception
    UART_Inits[UART_DEBUG].uarte.p_reg->PSEL.TXD = GPIO_Inits[GPIO_DEBUG].pin_number;

    syshal_uart_send(UART_DEBUG, (uint8_t *) ptr, len);

    // Change our TX pin back to its original
    UART_Inits[UART_DEBUG].uarte.p_reg->PSEL.TXD = last_tx_pin;

    return len;
}