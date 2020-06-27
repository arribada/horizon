/* iot.h - Internet of things abstraction layer
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

#ifndef _IOT_H_
#define _IOT_H_

#include <stdint.h>
#include "sys_config.h"
#include "fs.h"

#define IOT_NO_ERROR                        (   0)
#define IOT_INVALID_PARAM                   (  -1)
#define IOT_INVALID_RADIO_TYPE              (  -2)
#define IOT_ERROR_JSON                      (  -3)
#define IOT_ERROR_FS                        (  -4)
#define IOT_ERROR_NOT_ENABLED               (  -5)
#define IOT_ERROR_RADIO_ALREADY_ON          (  -6)
#define IOT_ERROR_RADIO_NOT_ON              (  -7)
#define IOT_ERROR_FILE_NOT_FOUND            (  -8)
#define IOT_ERROR_NOT_SUPPORTED             (  -9)
#define IOT_ERROR_NOT_CONNECTED             ( -10)
#define IOT_ERROR_HTTP                      (-100)
#define IOT_ERROR_SCAN_FAIL                 (-101)
#define IOT_ERROR_ATTACH_FAIL               (-102)
#define IOT_ERROR_ACTIVATE_PDP_FAIL         (-103)
#define IOT_ERROR_CREATE_PDP_FAIL           (-104)
#define IOT_ERROR_SET_RAT_FAIL              (-105)
#define IOT_ERROR_CHECK_SIM_FAIL            (-106)
#define IOT_ERROR_SYNC_COMS_FAIL            (-107)
#define IOT_ERROR_POWER_ON_FAIL             (-108)
#define IOT_ERROR_GET_SHADOW_FAIL           (-109)
#define IOT_ERROR_READ_SHADOW_FAIL          (-110)
#define IOT_ERROR_WRITE_SHADOW_FAIL         (-111)
#define IOT_ERROR_POST_SHADOW_FAIL          (-112)
#define IOT_ERROR_FILE_LOGGING_FAIL         (-113)
#define IOT_ERROR_WRITE_LOGGING_FAIL        (-114)
#define IOT_ERROR_READ_LOGGING_FAIL         (-115)
#define IOT_ERROR_GET_DOWNLOAD_FILE_FAIL    (-116)
#define IOT_ERROR_FS_DOWNLOAD_FILE_FAIL     (-117)
#define IOT_ERROR_READ_DOWNLOAD_FILE_FAIL   (-118)
#define IOT_ERROR_LOG_FILE_IS_CORRUPT       (-119)
#define IOT_ERROR_POST_LOGGING_FAIL         (-120)
#define IOT_ERROR_NETWORK_INFO_FAIL         (-121)
#define IOT_ERROR_PREPAS_FAIL               (-122)
#define IOT_ERROR_TRANSMISSION_FAIL         (-123)

#define IOT_AWS_ABSOLUTE_MAX_FILE_SIZE (1024 * 128) // Max file size the AWS accepts (128k)
#define IOT_AWS_MAX_FILE_SIZE          (1024 * 32)  // Max file size to send to AWS in one go. WARN: this will be placed on the stack

#define IOT_PRESENCE_FLAG_LAST_LOG_FILE_READ_POS              (1 << 0)
#define IOT_PRESENCE_FLAG_LAST_GPS_LOCATION                   (1 << 1)
#define IOT_PRESENCE_FLAG_BATTERY_LEVEL                       (1 << 2)
#define IOT_PRESENCE_FLAG_BATTERY_VOLTAGE                     (1 << 3)
#define IOT_PRESENCE_FLAG_LAST_CELLULAR_CONNECTED_TIMESTAMP   (1 << 4)
#define IOT_PRESENCE_FLAG_LAST_SAT_TX_TIMESTAMP               (1 << 5)
#define IOT_PRESENCE_FLAG_NEXT_SAT_TX_TIMESTAMP               (1 << 6)
#define IOT_PRESENCE_FLAG_CONFIGURATION_VERSION               (1 << 7)
#define IOT_PRESENCE_FLAG_FIRMWARE_VERSION                    (1 << 8)

#define IOT_DEVICE_UPDATE_PRESENCE_FLAG_CONFIGURATION_UPDATE  (1 << 0)
#define IOT_DEVICE_UPDATE_PRESENCE_FLAG_FIRMWARE_UPDATE       (1 << 1)

#define IOT_SAT_ZERO_PACKET (0)

typedef struct
{
    sys_config_iot_general_settings_t *iot_config;                     // IoT generic configuration parameters
    sys_config_iot_cellular_settings_t *iot_cellular_config;           // IoT cellular configuration parameters
    sys_config_iot_cellular_aws_settings_t *iot_cellular_aws_config;   // IoT cellular AWS configuration parameters
    sys_config_iot_cellular_apn_t *iot_cellular_apn;                   // IoT cellular AWS configuration parameters
    sys_config_iot_sat_settings_t *iot_sat_config;                     // IoT satellite configuration parameters
    sys_config_iot_sat_artic_settings_t *iot_sat_artic_config;         // IoT satellite Artic configuration parameters
    sys_config_system_device_identifier_t *system_device_identifier;
    fs_t fs;
    uint8_t satellite_firmware_file_id;
} iot_init_t;

typedef struct
{
    char domain[256];
    uint16_t port;
    char path[256];
} iot_url_t;

typedef struct
{
    iot_url_t url;
    uint32_t version;
} iot_versioned_url_t;

typedef struct
{
    uint32_t presence_flags;
    iot_versioned_url_t configuration_update;
    iot_versioned_url_t firmware_update;
} iot_device_update_t;

typedef struct
{
    float longitude;
    float latitude;
    uint32_t timestamp;
} iot_last_gps_location_t;

typedef struct
{
    uint32_t presence_flags;
    uint32_t last_log_file_read_pos;
    iot_last_gps_location_t last_gps_location;
    uint8_t battery_level;
    uint16_t battery_voltage;
    uint32_t last_cellular_connected_timestamp;
    uint32_t last_sat_tx_timestamp;
    uint32_t next_sat_tx_timestamp;
    uint32_t configuration_version;
    uint32_t firmware_version;
} iot_device_status_t;

typedef struct
{
    iot_device_update_t device_update;
    iot_device_status_t device_status;
} iot_device_shadow_t;

typedef uint32_t iot_prepass_result_t;

typedef enum
{
    IOT_RADIO_CELLULAR,
    IOT_RADIO_SATELLITE
} iot_radio_type_t;

typedef struct
{
    int16_t iot_error_code;
    int16_t hal_error_code;
    uint16_t hal_line_number;
    uint16_t vendor_error_code;
} iot_error_report_t;

typedef struct
{
    syshal_cellular_info_t cellular_info;
} iot_network_info_t;

typedef uint8_t iot_imsi_t[64];

int iot_init(iot_init_t init);
int iot_connect(uint32_t timeout_ms);
int iot_power_on(iot_radio_type_t radio_type);
int iot_check_sim(iot_imsi_t *imsi);
int iot_power_off(void);
int iot_calc_prepass(const iot_last_gps_location_t *gps, iot_prepass_result_t *result);
int iot_fetch_device_shadow(uint32_t timeout_ms, iot_device_shadow_t *shadow);
int iot_send_device_status(uint32_t timeout_ms, const iot_device_status_t *device_status);
int iot_send_logging(uint32_t timeout_ms, fs_t fs, uint32_t local_file_id, uint32_t log_file_read_pos);
int iot_download_file(uint32_t timeout_ms, const iot_url_t *url, fs_t fs, uint32_t local_file_id, uint32_t *file_size);
void iot_get_error_report(iot_error_report_t *error_report);
void iot_get_network_info(iot_network_info_t *network_info);

__attribute__((weak)) void iot_busy_handler(void);

#endif /* _IOT_H_ */
