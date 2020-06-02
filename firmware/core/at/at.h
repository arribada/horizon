/* at.h - AT library for handling AT command thought UART Interface
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

#ifndef _AT_H
#define _AT_H

#include "fs.h"
#include <stdint.h>

/* Constants */
#ifndef MAX_AT_COMMAND_SIZE
#define MAX_AT_COMMAND_SIZE 256
#endif

#ifndef MAX_AT_BUFFER_SIZE
#define MAX_AT_BUFFER_SIZE 4096
#endif

#define TIMES_CHAR_TIMEOUT          (100)

#define AT_NO_ERROR                     ( 0)
#define AT_ERROR_TIMEOUT                (-1)
#define AT_ERROR_UNEXPECTED_RESPONSE    (-2)
#define AT_ERROR_DEVICE                 (-3)
#define AT_ERROR_FORMAT_NOT_SUPPORTED   (-4)
#define AT_ERROR_BUFFER_OVERFLOW        (-5)
#define AT_ERROR_HTTP                   (-6)

#define HTTP_CODE_SUCCESS_NO_ERROR      (200)
#define HTTP_CODE_SUCCESS_NO_RESPONSE   (204)
#define HTTP_CODE_ERROR_BAD_REQUEST     (400)
#define HTTP_CODE_ERROR_UNAUTHORIZED    (401)
#define HTTP_CODE_ERROR_NOT_FOUND       (404)
#define HTTP_CODE_ERROR_INTERNAL_ERROR  (500)

/* Macros */

/* Types */

/* Functions */
int at_init(uint32_t uart_device);
int at_send(const uint8_t *format, ...);
int at_expect(const uint8_t *format, uint32_t timeout_ms, uint32_t *bytes_read, ...);
int at_expect_last_line(const uint8_t *format, ...);
int at_send_raw(const uint8_t *buffer, uint32_t length);
int at_send_raw_with_cr(const uint8_t *buffer, uint32_t length);
int at_send_raw_fs(fs_handle_t handle, uint32_t length);
int at_read_raw_to_fs(uint32_t timeout_ms, uint32_t length, fs_handle_t handle);
int at_read_raw_to_buffer(uint32_t timeout_ms, uint32_t length, uint8_t *buffer);
int at_expect_http_header(uint32_t *length, uint16_t *http_code);
int at_flush(void);
int at_discard(uint32_t length);
__attribute__((weak)) void at_busy_handler(void);

#endif /* _AT_H */