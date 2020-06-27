/* syshal_ble.h - HAL for ble device
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

#ifndef _SYSHAL_BLE_H_
#define _SYSHAL_BLE_H_

#include <stdint.h>
#include "sys_config.h"

/* Constants */

#define SYSHAL_BLE_MAX_BUFFER_SIZE         (512)
#define SYSHAL_BLE_UUID_SIZE               (16)
#define SYSHAL_BLE_ADVERTISING_SIZE        (31)

#define SYSHAL_BLE_NO_ERROR                (   0)
#define SYSHAL_BLE_ERROR_CRC               (  -1)
#define SYSHAL_BLE_ERROR_TIMEOUT           (  -2)
#define SYSHAL_BLE_ERROR_LENGTH            (  -3)
#define SYSHAL_BLE_ERROR_FW_TYPE           (  -4)
#define SYSHAL_BLE_ERROR_FORBIDDEN         (  -5)
#define SYSHAL_BLE_ERROR_BUSY              (  -6)
#define SYSHAL_BLE_ERROR_DISCONNECTED      (  -7)
#define SYSHAL_BLE_ERROR_FAIL              (  -8)
#define SYSHAL_BLE_ERROR_COMMS             (-100)
#define SYSHAL_BLE_ERROR_NOT_DETECTED      (-101)
#define SYSHAL_BLE_ERROR_RECEIVE_PENDING   (-102)
#define SYSHAL_BLE_ERROR_BUFFER_FULL       (-103)
#define SYSHAL_BLE_ERROR_DEVICE            (-104)
#define SYSHAL_BLE_ERROR_TRANSMIT_PENDING  (-105)

/* Macros */

/* Types */

typedef enum
{
    SYSHAL_BLE_EVENT_CONNECTED,
    SYSHAL_BLE_EVENT_DISCONNECTED,
    SYSHAL_BLE_EVENT_SEND_COMPLETE,
    SYSHAL_BLE_EVENT_RECEIVE_COMPLETE,
    SYSHAL_BLE_EVENT_FW_UPGRADE_COMPLETE,
    SYSHAL_BLE_EVENT_ERROR_INDICATION
} syshal_ble_event_id_t;

typedef enum
{
    SYSHAL_BLE_MODE_IDLE,
    SYSHAL_BLE_MODE_FW_UPGRADE,
    SYSHAL_BLE_MODE_BEACON,
    SYSHAL_BLE_MODE_GATT_SERVER,
    SYSHAL_BLE_MODE_GATT_CLIENT,
    SYSHAL_BLE_MODE_SCAN,
    SYSHAL_BLE_MODE_DEEP_SLEEP
} syshal_ble_mode_t;

typedef struct
{
    sys_config_tag_bluetooth_device_address_t        *tag_bluetooth_device_address;
    sys_config_tag_bluetooth_advertising_interval_t  *tag_bluetooth_advertising_interval;
    sys_config_tag_bluetooth_connection_interval_t   *tag_bluetooth_connection_interval;
    sys_config_tag_bluetooth_phy_mode_t              *tag_bluetooth_phy_mode;
    sys_config_tag_bluetooth_advertising_tags_t      *tag_bluetooth_advertising_tags;
} syshal_ble_init_t;

typedef struct
{
    syshal_ble_event_id_t id;
    int                   error;
    union
    {
        struct
        {
            uint16_t length;
        } send_complete;
        struct
        {
            uint16_t length;
        } receive_complete;
    };
} syshal_ble_event_t;

/* Functions */

int syshal_ble_init(syshal_ble_init_t ble_config);
int syshal_ble_term(void);
int syshal_ble_set_mode(syshal_ble_mode_t mode);
int syshal_ble_get_mode(syshal_ble_mode_t *mode);
int syshal_ble_get_version(uint32_t *version);
int syshal_ble_config_fw_upgrade(uint32_t size, uint32_t crc);
int syshal_ble_fw_send(uint8_t * data, uint32_t size);
int syshal_ble_reset(void);
int syshal_ble_send(uint8_t *buffer, uint32_t size);
int syshal_ble_receive(uint8_t *buffer, uint32_t size);
int syshal_ble_tick(void);
int syshal_ble_event_handler(syshal_ble_event_t *event);

#endif /* _SYSHAL_BLE_H_ */
