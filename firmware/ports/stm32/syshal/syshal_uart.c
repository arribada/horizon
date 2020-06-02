/* Copyright 2019 Arribada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include "stm32f0xx_hal.h"
#include "syshal_gpio.h"
#include "syshal_uart.h"
#include "ring_buffer.h"
#include "debug.h"

// HAL to SYSHAL error code mapping table
static int hal_error_map[] =
{
    SYSHAL_UART_NO_ERROR,
    SYSHAL_UART_ERROR_DEVICE,
    SYSHAL_UART_ERROR_BUSY,
    SYSHAL_UART_ERROR_TIMEOUT,
};

// Private variables
static UART_HandleTypeDef huart_handles[UART_TOTAL_NUMBER];

// Internal variables
static ring_buffer_t rx_buffer[UART_TOTAL_NUMBER];
static uint8_t rx_data[UART_TOTAL_NUMBER][UART_RX_BUF_SIZE];

/**
 * @brief      Initialise the given UART instance
 *
 * @param[in]  instance  The UART instance
 *
 * @return SYSHAL_UART_ERROR_INVALID_INSTANCE if instance doesn't exist.
 * @return SYSHAL_UART_ERROR_DEVICE on HAL error.
 * @return SYSHAL_UART_ERROR_BUSY if the HW is busy.
 * @return SYSHAL_UART_ERROR_TIMEOUT if a timeout occurred.
 */
int syshal_uart_init(uint32_t instance)
{
    HAL_StatusTypeDef status;

    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    // Populate internal handlers from bsp
    huart_handles[instance].Instance = UART_Inits[instance].Instance;
    huart_handles[instance].Init = UART_Inits[instance].Init;

    // Setup rx buffer
    rb_init(&rx_buffer[instance], UART_RX_BUF_SIZE, &rx_data[instance][0]);

#ifndef DEBUG_DISABLED
#ifdef PRINTF_UART
    // Turn off buffers. This ensure printf prints immediately
    if (instance == PRINTF_UART)
    {
        setvbuf(stdin, NULL, _IONBF, 0);
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
#endif
#endif

    status = HAL_UART_Init(&huart_handles[instance]);

    return hal_error_map[status];
}

/**
 * @brief      Change the baudrate of a UART instance
 *
 * @param[in]  instance  The UART instance
 * @param[in]  baudrate  The baudrate to change to
 *
 * @return     SYSHAL_UART_ERROR_INVALID_INSTANCE if instance doesn't exist.
 * @return     SYSHAL_UART_ERROR_DEVICE on HAL error.
 * @return     SYSHAL_UART_ERROR_BUSY if the HW is busy.
 * @return     SYSHAL_UART_ERROR_TIMEOUT if a timeout occurred.
 */
int syshal_uart_change_baud(uint32_t instance, uint32_t baudrate)
{
    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    DEBUG_PR_TRACE("Changing baudrate on UART_%lu to %lu", instance + 1, baudrate);

    HAL_StatusTypeDef status;

    // Populate internal handlers from bsp
    huart_handles[instance].Instance = UART_Inits[instance].Instance;
    huart_handles[instance].Init = UART_Inits[instance].Init;
    huart_handles[instance].Init.BaudRate = baudrate;

    status = HAL_UART_Init(&huart_handles[instance]);

    return hal_error_map[status];
}

/**
 * @brief      Deinitialise the given UART instance
 *
 * @param[in]  instance  The UART instance
 *
 * @return SYSHAL_UART_ERROR_INVALID_INSTANCE if instance doesn't exist.
 * @return SYSHAL_UART_ERROR_DEVICE on HAL error.
 * @return SYSHAL_UART_ERROR_BUSY if the HW is busy.
 * @return SYSHAL_UART_ERROR_TIMEOUT if a timeout occurred.
 */
int syshal_uart_term(uint32_t instance)
{
    HAL_StatusTypeDef status;

    if (instance >= UART_TOTAL_NUMBER)
        return SYSHAL_UART_ERROR_INVALID_INSTANCE;

    status = HAL_UART_DeInit(&huart_handles[instance]);

    return hal_error_map[status];
}

/**
 * @brief Read one character from a serial port.
 *
 * It's not safe to call this function if the serial port has no data
 * available.
 *
 * @param dev Serial port to read from
 * @return byte read
 * @see usart_data_available()
 */
static inline uint8_t usart_getc_priv(UART_t instance)
{
    return rb_remove(&rx_buffer[instance]); // Remove and return the first item from a ring buffer.
}

/**
 * @brief      Return the amount of data available in a serial port's RX buffer.
 *
 * @param[in]  instance  The serial port to check
 *
 * @return     Number of bytes in serial port's RX buffer.
 */
uint32_t syshal_uart_available(uint32_t instance)
{
    return rb_occupancy(&rx_buffer[instance]); // Return the number of elements stored in the ring buffer
}

/**
 * @brief Nonblocking USART receive.
 * @param dev Serial port to receive bytes from
 * @param buf Buffer to store received bytes into
 * @param size Maximum number of bytes to store
 * @return Number of bytes received
 */
int syshal_uart_receive(uint32_t instance, uint8_t * data, uint32_t size)
{
    uint32_t count = rb_occupancy(&rx_buffer[instance]);

    if (size > count)
        size = count;

    for (uint32_t i = 0; i < size; ++i)
    {
        *data++ = usart_getc_priv(instance);
    }

    return size;
}

// Peek at character at location at nth depth in rx buffer (0 being the oldest/top value of the FIFO). Return true on success
bool syshal_uart_peek_at(uint32_t instance, uint8_t * byte, uint32_t location)
{
    *byte = rb_peek_at(&rx_buffer[instance], location);//rb_peek(&rx_buffer[instance]);//rb_peek_at(&rx_buffer[instance], location);

    if (-1 == *byte)
        return false;
    else
        return true;
}

int syshal_uart_send(uint32_t instance, uint8_t * data, uint32_t size)
{
    if (!size)
        return SYSHAL_UART_ERROR_INVALID_SIZE;

    HAL_StatusTypeDef status;

    // Wait for UART to be free
    if (UART_WaitOnFlagUntilTimeout(&huart_handles[instance], UART_FLAG_TXE, RESET, HAL_GetTick(), UART_TIMEOUT) != HAL_OK)
        return SYSHAL_UART_ERROR_TIMEOUT;

    status = HAL_UART_Transmit(&huart_handles[instance], data, size, UART_TIMEOUT);

    return hal_error_map[status];
}

// Implement MSP hooks that are called by stm32f0xx_hal_uart
void HAL_UART_MspInit(UART_HandleTypeDef * huart)
{
    if (huart->Instance == USART1)
    {
        // Peripheral clock disable
        __HAL_RCC_USART1_CLK_ENABLE();

        // USART1 GPIO Configuration
        syshal_gpio_init(GPIO_UART1_TX);
        syshal_gpio_init(GPIO_UART1_RX);

        // USART1 interrupt Init
        __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE); // Enable the UART Data Register not empty Interrupt

        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }

    if (huart->Instance == USART2)
    {
        // Peripheral clock enable
        __HAL_RCC_USART2_CLK_ENABLE();

        // USART2 GPIO Configuration
        syshal_gpio_init(GPIO_UART2_TX);

        // No receive pin so ignore don't initiate one

        //syshal_gpio_init(GPIO_UART2_RX);

        // USART2 interrupt Init
        //__HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);

        //HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
        //HAL_NVIC_EnableIRQ(USART2_IRQn);
    }

    if (huart->Instance == USART3)
    {
        // Peripheral clock disable
        __HAL_RCC_USART3_CLK_ENABLE();

        // USART3 GPIO Configuration
        syshal_gpio_init(GPIO_UART3_TX);
        syshal_gpio_init(GPIO_UART3_RX);
        syshal_gpio_init(GPIO_UART3_RTS);
        syshal_gpio_init(GPIO_UART3_CTS);

        // USART3 interrupt Init
        __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);

        HAL_NVIC_SetPriority(USART3_4_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART3_4_IRQn);
    }

//    if (huart->Instance == USART4)
//    {
//        // Peripheral clock disable
//        __HAL_RCC_USART4_CLK_ENABLE();
//
//        // USART4 GPIO Configuration
//        syshal_gpio_init(GPIO_UART4_TX);
//        syshal_gpio_init(GPIO_UART4_RX);
//
//        // USART4 interrupt Init
//        __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
//
//        HAL_NVIC_SetPriority(USART3_4_IRQn, 0, 0);
//        HAL_NVIC_EnableIRQ(USART3_4_IRQn);
//    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef * huart)
{

    if (huart->Instance == USART1)
    {
        // Peripheral clock disable
        __HAL_RCC_USART1_CLK_DISABLE();

        // USART1 GPIO Configuration
        syshal_gpio_term(GPIO_UART1_TX);
        syshal_gpio_term(GPIO_UART1_RX);

        // USART1 interrupt DeInit
        HAL_NVIC_DisableIRQ(USART1_IRQn);
    }

    if (huart->Instance == USART2)
    {
        // Peripheral clock disable
        __HAL_RCC_USART2_CLK_DISABLE();

        // USART2 GPIO Configuration
        syshal_gpio_term(GPIO_UART2_TX);
        //syshal_gpio_term(GPIO_UART2_RX);

        // USART2 interrupt DeInit
        //HAL_NVIC_DisableIRQ(USART2_IRQn);
    }

    if (huart->Instance == USART3)
    {
        // Peripheral clock disable
        __HAL_RCC_USART3_CLK_DISABLE();

        // USART3 GPIO Configuration
        syshal_gpio_term(GPIO_UART3_TX);
        syshal_gpio_term(GPIO_UART3_RX);
        syshal_gpio_term(GPIO_UART3_RTS);
        syshal_gpio_term(GPIO_UART3_CTS);
    }

//    if (huart->Instance == USART4)
//    {
//        // Peripheral clock disable
//        __HAL_RCC_USART4_CLK_DISABLE();
//
//        // USART4 GPIO Configuration
//        syshal_gpio_term(GPIO_UART4_TX);
//        syshal_gpio_term(GPIO_UART4_RX);
//    }

}

/**
* @brief This function handles USART1 global interrupt / USART1 wake-up interrupt through EXTI line 25.
*/
void USART1_IRQHandler(void)
{
    // Did we receive data ?
    if (__HAL_UART_GET_IT(&huart_handles[UART_1], UART_IT_RXNE))
    {
        uint16_t byte; // Ensure correct alignment

        byte = huart_handles[UART_1].Instance->RDR;

        uint8_t rxBuffer = (uint8_t)byte;
#ifdef UART_1_SAFE_INSERT
        // If the buffer is full and the user defines UART_SAFE_INSERT, ignore new bytes.
        if (!rb_safe_insert(&rx_buffer[UART_1], rxBuffer))
            DEBUG_PR_ERROR("Rx buffer UART_%u full", UART_1 + 1);
#else
        // If the buffer is full overwrite data
        rb_push_insert(&rx_buffer[UART_1], rxBuffer);
#endif
    }
}

/**
* @brief This function handles USART2 global interrupt.
*/
void USART2_IRQHandler(void)
{
    // Did we receive data ?
    if (__HAL_UART_GET_IT(&huart_handles[UART_2], UART_IT_RXNE))
    {
        uint16_t byte; // Ensure correct alignment

        byte = huart_handles[UART_2].Instance->RDR;

        uint8_t rxBuffer = (uint8_t)byte;
#ifdef UART_2_SAFE_INSERT
        // If the buffer is full and the user defines UART_SAFE_INSERT, ignore new bytes.
        if (!rb_safe_insert(&rx_buffer[UART_2], rxBuffer))
            DEBUG_PR_ERROR("Rx buffer UART_%u full", UART_2 + 1);
#else
        // If the buffer is full overwrite data
        rb_push_insert(&rx_buffer[UART_2], rxBuffer);
#endif
    }
}

/**
* @brief This function handles USART3 and USART4 global interrupts
*/
void USART3_4_IRQHandler(void)
{
    // Did we receive data ?
    if (__HAL_UART_GET_IT(&huart_handles[UART_3], UART_IT_RXNE))
    {
        uint16_t byte; // Ensure correct alignment

        byte = huart_handles[UART_3].Instance->RDR;

        uint8_t rxBuffer = (uint8_t)byte;
#ifdef UART_3_SAFE_INSERT
        // If the buffer is full and the user defines UART_SAFE_INSERT, ignore new bytes.
        if (!rb_safe_insert(&rx_buffer[UART_3], rxBuffer))
            DEBUG_PR_ERROR("Rx buffer UART_%u full", UART_3 + 1);
#else
        // If the buffer is full overwrite data
        rb_push_insert(&rx_buffer[UART_3], rxBuffer);
#endif
    }
}

// Override _write function to enable printf use, but only if we have printf assigned to a uart
#ifndef DEBUG_DISABLED
#ifdef PRINTF_UART
int _write(int file, char * data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }

    // Wait for UART to be free
    if (UART_WaitOnFlagUntilTimeout(&huart_handles[PRINTF_UART], UART_FLAG_TXE, RESET, HAL_GetTick(), UART_TIMEOUT) != HAL_OK)
        return 0;

    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart_handles[PRINTF_UART], (uint8_t *)data, len, UART_TIMEOUT);

    // return # of bytes written - as best we can tell
    return (status == HAL_OK ? len : 0);
}
#endif
#endif