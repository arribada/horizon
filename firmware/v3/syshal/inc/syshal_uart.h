/* syshal_uart.h - HAL for UART
 *
 * Copyright (C) 2019 Arribada
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
 */

#ifndef _SYSHAL_UART_H_
#define _SYSHAL_UART_H_

#include <stdint.h>
#include <stdbool.h>

// Constants
#define SYSHAL_UART_NO_ERROR               ( 0)
#define SYSHAL_UART_ERROR_INVALID_SIZE     (-1)
#define SYSHAL_UART_ERROR_INVALID_INSTANCE (-2)
#define SYSHAL_UART_ERROR_BUSY             (-3)
#define SYSHAL_UART_ERROR_TIMEOUT          (-4)
#define SYSHAL_UART_ERROR_DEVICE           (-5)

int syshal_uart_init(uint32_t instance);
int syshal_uart_term(uint32_t instance);
int syshal_uart_change_baud(uint32_t instance, uint32_t baudrate);
int syshal_uart_get_baud(uint32_t instance, uint32_t * baudrate);
int syshal_uart_send(uint32_t instance, uint8_t * data, uint32_t size);
int syshal_uart_receive(uint32_t instance, uint8_t * data, uint32_t size);
bool syshal_uart_peek_at(uint32_t instance, uint8_t * byte, uint32_t location);
uint32_t syshal_uart_available(uint32_t instance);
int syshal_uart_flush(uint32_t instance);
int syshal_uart_read_timeout(uint32_t instance, uint8_t * buffer, uint32_t buf_size, uint32_t read_timeout_us, uint32_t last_char_timeout_us, uint32_t * bytes_received);
#ifdef GTEST
void inject_error(uint8_t b_inject_error ,uint8_t *data, uint32_t size);
#endif /* _SYSHAL_UART_H_ */
#endif