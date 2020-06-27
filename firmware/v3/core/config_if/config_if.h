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

#ifndef _CONFIG_IF_H_
#define _CONFIG_IF_H_

#include <stdint.h>
#include <stdbool.h>
#include "syshal_ble.h"
#include "fs.h"

// Constants
#define CONFIG_IF_NO_ERROR                  ( 0)
#define CONFIG_IF_ERROR_BUSY                (-1)
#define CONFIG_IF_ERROR_FAIL                (-2)
#define CONFIG_IF_ERROR_DISCONNECTED        (-3)
#define CONFIG_IF_ERROR_INVALID_SIZE        (-4)
#define CONFIG_IF_ERROR_INVALID_INSTANCE    (-5)
#define CONFIG_IF_ERROR_ALREADY_CONFIGURED  (-6)

typedef enum
{
    CONFIG_IF_EVENT_SEND_COMPLETE, //  BLE or USB send complete
    CONFIG_IF_EVENT_RECEIVE_COMPLETE, // BLE or USB receive complete, the length of data received should be passed back as part of config_if_event_t
    CONFIG_IF_EVENT_CONNECTED, // maps to BLE connection or USB enumeration complete
    CONFIG_IF_EVENT_DISCONNECTED, // maps to BLE disconnection or USB host disconnection
} config_if_event_id_t;

typedef enum
{
    CONFIG_IF_BACKEND_USB,
    CONFIG_IF_BACKEND_BLE,
    CONFIG_IF_BACKEND_FS_SCRIPT,
    CONFIG_IF_BACKEND_NOT_SET
} config_if_backend_id_t;

typedef struct
{
    config_if_backend_id_t id;
    union
    {
        struct
        {
            fs_t filesystem;
            uint8_t file_id;
        } fs_script;

        struct 
        {
            syshal_ble_init_t config;
        } ble;
    };
} config_if_backend_t;

typedef struct
{
    config_if_event_id_t id;
    config_if_backend_id_t backend;
    union
    {
        struct
        {
            uint32_t size;
        } send;
        struct
        {
            uint32_t size;
        } receive;
    };
} config_if_event_t;

int config_if_init(config_if_backend_t backend);
int config_if_term(void);
int config_if_send(uint8_t * data, uint32_t size);
int config_if_receive(uint8_t * data, uint32_t size);
int config_if_receive_byte_stream(uint8_t * data, uint32_t size);
config_if_backend_id_t config_if_current(void);
void config_if_tick(void);

int config_if_callback(config_if_event_t * event);

#endif /* _CONFIG_IF_H_ */