/* sm_iot.h - IOT state machine
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

#ifndef _SM_IOT_H_
#define _SM_IOT_H_

#include "iot.h"
#include "logging.h"

#define SM_IOT_NO_ERROR                 ( 0)
#define SM_IOT_ERROR_INVALID_PARAM      (-1)
#define SM_IOT_ERROR_CONNECTION_FAILED  (-2)
#define SM_IOT_ERROR_INIT               (-3)
#define SM_IOT_ERROR_PREPAS_CALC_FAILED (-4)

typedef enum
{
    SM_IOT_ABOUT_TO_POWER_ON,
    SM_IOT_CELLULAR_POWER_ON,
    SM_IOT_CELLULAR_POWER_OFF,
    SM_IOT_CELLULAR_CONNECT,
    SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW,
    SM_IOT_CELLULAR_SEND_LOGGING,
    SM_IOT_CELLULAR_SEND_DEVICE_STATUS,
    SM_IOT_CELLULAR_DOWNLOAD_FIRMWARE_FILE,
    SM_IOT_CELLULAR_DOWNLOAD_CONFIG_FILE,
    SM_IOT_CELLULAR_MAX_BACKOFF_REACHED,
    SM_IOT_CELLULAR_NETWORK_INFO,
    SM_IOT_SATELLITE_POWER_ON,
    SM_IOT_SATELLITE_POWER_OFF,
    SM_IOT_SATELLITE_SEND_DEVICE_STATUS,
    SM_IOT_APPLY_FIRMWARE_UPDATE,
    SM_IOT_APPLY_CONFIG_UPDATE,
    SM_IOT_NEXT_PREPAS
} sm_iot_event_id_t;

typedef struct
{
    sm_iot_event_id_t id;
    iot_error_report_t iot_report_error;
    union
    {
        iot_network_info_t network_info;
        struct
        {
            uint32_t version;
            uint32_t length;
        } firmware_update;
        struct
        {
            uint32_t version;
            uint32_t length;
        } config_update;
        struct
        {
            uint32_t gps_timestamp;
            iot_prepass_result_t prepas_result;
        } next_prepas;
    };
} sm_iot_event_t;

typedef struct
{
    fs_t file_system;
    iot_init_t iot_init;
    sys_config_gps_last_known_position_t * gps_last_known_position; // NULL permitted
    sys_config_version_t * configuration_version; // NULL permitted
} sm_iot_init_t;

typedef struct
{
    float longitude;
    float latitude;
    uint32_t timestamp;
} sm_iot_timestamped_position_t;

int sm_iot_init(sm_iot_init_t init);
int sm_iot_term(void);
int sm_iot_trigger_force(iot_radio_type_t radio_type);
int sm_iot_new_position(sm_iot_timestamped_position_t timestamp_pos);
int sm_iot_tick(void);

void sm_iot_callback(sm_iot_event_t * event);

#endif /* _SM_IOT_H_ */
