/* config_if.h - Configuration interface abstraction layer. This is used
 * to homogenise the USB and BLE syshals for seamless switching between them
 *
 * Copyright (C) 2018 Arribada
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

#ifndef _FS_SCRIPT_H_
#define _FS_SCRIPT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fs.h"

// Constants
#define FS_SCRIPT_NO_ERROR                   ( 0)
#define FS_SCRIPT_ERROR_END_OF_FILE          (-1)
#define FS_SCRIPT_ERROR_FS                   (-2)
#define FS_SCRIPT_ERROR_FILE_NOT_OPEN        (-3)
#define FS_SCRIPT_ERROR_INVALID_FILE_FORMAT  (-4)
#define FS_SCRIPT_ERROR_BUFFER_TOO_SMALL     (-5)
#define FS_SCRIPT_ERROR_FILE_NOT_FOUND       (-6)

typedef enum
{
    FS_SCRIPT_FILE_OPENED,
    FS_SCRIPT_FILE_CLOSED,
    FS_SCRIPT_RECEIVE_COMPLETE,
    FS_SCRIPT_SEND_COMPLETE,
} fs_script_event_id_t;

typedef struct
{
    fs_script_event_id_t id;
    union
    {
        struct
        {
            uint8_t * buffer;
            uint32_t size;
        } send;
        struct
        {
            uint8_t * buffer;
            uint32_t size;
        } receive;
    };
} fs_script_event_t;

int fs_script_init(fs_t fs, uint8_t file_id);
int fs_script_term(void);
int fs_script_send(uint8_t * buffer, uint32_t length);
int fs_script_receive(uint8_t * buffer, uint32_t length);
int fs_script_receive_byte_stream(uint8_t * buffer, uint32_t length);

void fs_script_event_handler(fs_script_event_t * event);

#endif /* _FS_SCRIPT_H_ */