/* iot.c - Internet of things abstraction layer
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

#include <stdbool.h>
#include <string.h>
#include "syshal_cellular.h"
#include "syshal_sat.h"
#include "syshal_pmu.h"
#include "logging.h"
#include "aws.h"
#include "iot.h"
#include "debug.h"

#define JSON_WORKING_BUF_SIZE (2048)

#define CELLULAR_SET_RAT_TIMEOUT_MS (10000)

#define IOT_SAT_MAX_PAYLOAD_SIZE (SYSHAL_SAT_MAX_PACKET_SIZE - sizeof(satellite_hdr_t)) 
#define IOT_SAT_TYPE_SEND_STATUS (0x00)

typedef struct __attribute__((__packed__))
{
    uint8_t type;
    uint32_t presence_flags;
} satellite_hdr_t;

typedef struct __attribute__((__packed__))
{
    satellite_hdr_t hdr;
    uint8_t payload[IOT_SAT_MAX_PAYLOAD_SIZE];
} satellite_packet_t;

static iot_init_t config;

static char logging_topic_path_full[sizeof(config.iot_cellular_aws_config->contents.thing_name) + sizeof(config.iot_cellular_aws_config->contents.logging_topic_path)];
static char device_shadow_path_full[sizeof(config.iot_cellular_aws_config->contents.thing_name) + sizeof(config.iot_cellular_aws_config->contents.device_shadow_path)];

static iot_error_report_t iot_report_error;
static iot_network_info_t iot_network_info;

static void generate_cellular_error_report(int16_t iot_error_code, int16_t hal_error, uint16_t vendor_error_code)
{
    iot_report_error.iot_error_code = iot_error_code;
    iot_report_error.hal_error_code = hal_error;
    iot_report_error.hal_line_number = syshal_cellular_get_line_error();
    iot_report_error.vendor_error_code = vendor_error_code;
}

static void generate_sat_error_report(int16_t iot_error_code, int16_t hal_error)
{
    uint32_t status_register = 0;
    iot_report_error.iot_error_code = iot_error_code;
    iot_report_error.hal_error_code = hal_error;
    iot_report_error.hal_line_number = 0;

    if (syshal_sat_get_status_register(&status_register) == 0)
    {
        iot_report_error.vendor_error_code = status_register;
    }
    else
    {
        iot_report_error.vendor_error_code = 0;
    }
}

static void generate_iot_error_report(int16_t iot_error_code, int16_t hal_error, uint16_t line_number)
{
    iot_report_error.iot_error_code = iot_error_code;
    iot_report_error.hal_error_code = hal_error;
    iot_report_error.hal_line_number = line_number;
    iot_report_error.vendor_error_code = 0;
}

static void clear_error_report()
{
    iot_report_error.iot_error_code = IOT_NO_ERROR;
    iot_report_error.hal_error_code = 0;
    iot_report_error.hal_line_number = 0;
    iot_report_error.vendor_error_code = 0;
}

static struct
{
    iot_radio_type_t radio_type;
    bool radio_on;
    bool connected;
} internal_status;

static scan_mode_t scan_mode_mapping(uint32_t mode)
{
    switch (mode)
    {
        default:
        case SYS_CONFIG_TAG_IOT_CELLULAR_CONNECTION_MODE_AUTO:
            return SCAN_MODE_AUTO;
        case SYS_CONFIG_TAG_IOT_CELLULAR_CONNECTION_MODE_2_G:
            return SCAN_MODE_2G;
        case SYS_CONFIG_TAG_IOT_CELLULAR_CONNECTION_MODE_3_G:
            return SCAN_MODE_3G;
    }
}

static bool replace_hash(char *dest, const char *source, const char *replacement)
{
    size_t source_len = strlen(source);
    size_t replacement_len = strlen(replacement);

    char *hash_location = strchr(source, '#');
    if (!hash_location)
    {
        // No hash found so just use original string
        memcpy(dest, source, source_len);
        return false;
    }

    // Copy up to '#' from source
    size_t first_size = hash_location - source;
    memcpy(dest, source, first_size);
    dest += first_size;

    // Append replacement string
    memcpy(dest, replacement, replacement_len);
    dest += replacement_len;

    // Append string after '#' from source
    size_t remaining_size = source_len - first_size - 1;
    memcpy(dest, source + first_size + 1, remaining_size);
    dest += remaining_size;

    // Null terminate the string
    *(dest) = '\0';

    return true;
}

int iot_init(iot_init_t init)
{
    // Make a local copy of the init params
    config = init;

    // Replace character '#' with thing_name in aws paths if one exists, and IoT cellular AWS is enabled
    if (config.iot_config->contents.enable && config.iot_config->hdr.set &&
        config.iot_cellular_config->contents.enable && config.iot_cellular_config->hdr.set)
    {
        if (config.iot_cellular_aws_config->contents.thing_name[0] == '\0')
        {
            // Use the system device name if a thing_name is not given
            replace_hash(device_shadow_path_full, config.iot_cellular_aws_config->contents.device_shadow_path, config.system_device_identifier->contents.name);
            replace_hash(logging_topic_path_full, config.iot_cellular_aws_config->contents.logging_topic_path, config.system_device_identifier->contents.name);
        }
        else
        {
            replace_hash(device_shadow_path_full, config.iot_cellular_aws_config->contents.device_shadow_path, config.iot_cellular_aws_config->contents.thing_name);
            replace_hash(logging_topic_path_full, config.iot_cellular_aws_config->contents.logging_topic_path, config.iot_cellular_aws_config->contents.thing_name);
        }
    }

    internal_status.radio_on = false;
    internal_status.connected = false;

    return IOT_NO_ERROR;
}

int iot_power_on(iot_radio_type_t radio_type)
{
    iot_imsi_t imsi;
    int ret;
    uint16_t error_code;

    clear_error_report();

    if (!config.iot_config->hdr.set || !config.iot_config->contents.enable)
    {
        generate_iot_error_report(IOT_ERROR_NOT_ENABLED, 0, __LINE__);
        return IOT_ERROR_NOT_ENABLED;
    }

    if (internal_status.radio_on)
    {
        generate_iot_error_report(IOT_ERROR_RADIO_ALREADY_ON, 0, __LINE__);
        return IOT_ERROR_RADIO_ALREADY_ON;
    }

    switch (radio_type)
    {
        case IOT_RADIO_CELLULAR:
            if (!config.iot_cellular_config->hdr.set || !config.iot_cellular_config->contents.enable)
            {
                generate_iot_error_report(IOT_ERROR_NOT_ENABLED, 0, __LINE__);
                return IOT_ERROR_NOT_ENABLED;
            }

            internal_status.radio_type = radio_type;
            DEBUG_PR_TRACE("Powering cellular on");
            ret = syshal_cellular_power_on();
            if (ret)
            {
                generate_iot_error_report(IOT_ERROR_POWER_ON_FAIL, ret, __LINE__);
                return IOT_ERROR_POWER_ON_FAIL;
            }

            DEBUG_PR_TRACE("Syncing cellular comms");
            ret = syshal_cellular_sync_comms(&error_code);
            if (ret)
            {
                generate_cellular_error_report(IOT_ERROR_SYNC_COMS_FAIL, ret, error_code);
                return IOT_ERROR_SYNC_COMS_FAIL;
            }

            DEBUG_PR_TRACE("Checking cellular sim");
            ret = syshal_cellular_check_sim((uint8_t *) &imsi, &error_code);
            if (ret)
            {
                generate_cellular_error_report(IOT_ERROR_CHECK_SIM_FAIL, ret, error_code);
                return IOT_ERROR_CHECK_SIM_FAIL;
            }

            DEBUG_PR_TRACE("Creating secure profile");
            ret = syshal_cellular_create_secure_profile(&error_code);
            if (ret)
            {
                generate_cellular_error_report(IOT_ERROR_CREATE_PDP_FAIL, ret, error_code);
                return IOT_ERROR_CREATE_PDP_FAIL;
            }

            DEBUG_PR_TRACE("Setting connection preferences");
            ret = syshal_cellular_set_rat(CELLULAR_SET_RAT_TIMEOUT_MS, scan_mode_mapping(config.iot_cellular_config->contents.connection_mode), &error_code);
            if (ret)
            {
                generate_cellular_error_report(IOT_ERROR_SET_RAT_FAIL, ret, error_code);
                return IOT_ERROR_SET_RAT_FAIL;
            }

            internal_status.radio_on = true;
            break;

        case IOT_RADIO_SATELLITE:
            {
                uint32_t num_attempts = 3;

                while (num_attempts) {
                    DEBUG_PR_TRACE("Powering satellite on");
                    internal_status.radio_type = radio_type;
                    ret = syshal_sat_power_on();
                    if (ret)
                    {
                        num_attempts--;
                        syshal_time_delay_ms(100);
                        continue;
                    }

                    DEBUG_PR_TRACE("Programming satellite");
                    ret = syshal_sat_program_firmware(config.fs, config.satellite_firmware_file_id);
                    if (ret)
                    {
                        num_attempts--;
                        syshal_time_delay_ms(100);
                        continue;
                    }
                    else
                    {
                        internal_status.radio_on = true;
                        break;
                    }
                };

                if (num_attempts == 0)
                {
                    generate_sat_error_report(IOT_ERROR_POWER_ON_FAIL, ret);
                    return IOT_ERROR_POWER_ON_FAIL;
                }
                break;
            }

        default:
            return IOT_INVALID_PARAM;
            break;
    }

    return IOT_NO_ERROR;
}

int iot_power_off(void)
{
    clear_error_report();
    // We don't check if the Iot is enabled here as we want to be 100% sure the radio is powered off when this is called
    switch (internal_status.radio_type)
    {
        case IOT_RADIO_CELLULAR:
            DEBUG_PR_TRACE("Powering cellular off");
            syshal_cellular_power_off();
            break;
        case IOT_RADIO_SATELLITE:
            DEBUG_PR_TRACE("Powering satellite off");
            syshal_sat_power_off();
            break;
        default:
            return IOT_INVALID_PARAM; // Shouldn't be possible
            break;
    }

    internal_status.radio_on = false;
    internal_status.connected = false;

    return IOT_NO_ERROR;
}

int iot_connect(uint32_t timeout_ms)
{
    int return_code;
    uint16_t error_code;

    clear_error_report();

    if (!config.iot_config->hdr.set || !config.iot_config->contents.enable)
    {
        generate_iot_error_report(IOT_ERROR_NOT_ENABLED, 0, __LINE__);
        return IOT_ERROR_NOT_ENABLED;
    }

    if (!internal_status.radio_on)
    {
        generate_iot_error_report(IOT_ERROR_RADIO_NOT_ON, 0, __LINE__);
        return IOT_ERROR_RADIO_NOT_ON;
    }

    switch (internal_status.radio_type)
    {
        case IOT_RADIO_CELLULAR:
            if (!config.iot_cellular_config->hdr.set || !config.iot_cellular_config->contents.enable)
            {
                generate_iot_error_report(IOT_ERROR_NOT_ENABLED, 0, __LINE__);
                return IOT_ERROR_NOT_ENABLED;
            }

            DEBUG_PR_TRACE("Scanning for cellular network");
            return_code = syshal_cellular_scan(timeout_ms, &error_code);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_SCAN_FAIL, return_code, error_code);
                return IOT_ERROR_SCAN_FAIL;
            }

            DEBUG_PR_TRACE("Attaching to cellular network");
            return_code = syshal_cellular_attach(timeout_ms, &error_code);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_ATTACH_FAIL, return_code, error_code);
                return IOT_ERROR_ATTACH_FAIL;
            }

            DEBUG_PR_TRACE("Getting network information");
            return_code = syshal_cellular_network_info(&(iot_network_info.cellular_info), &error_code);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_NETWORK_INFO_FAIL, return_code, error_code);
                return IOT_ERROR_NETWORK_INFO_FAIL;
            }

            DEBUG_PR_TRACE("Activating pdp");
            return_code = syshal_cellular_activate_pdp(&config.iot_cellular_apn->contents.apn, timeout_ms, &error_code);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_ACTIVATE_PDP_FAIL, return_code, error_code);
                return IOT_ERROR_ACTIVATE_PDP_FAIL;
            }

            break;
        case IOT_RADIO_SATELLITE:
            generate_iot_error_report(IOT_ERROR_NOT_SUPPORTED, 0, __LINE__);
            return IOT_ERROR_NOT_SUPPORTED;
            break;
        default:
            return IOT_INVALID_PARAM; // Shouldn't be possible
            break;
    }

    internal_status.connected = true;
    return IOT_NO_ERROR;
}

int iot_check_sim(iot_imsi_t *imsi)
{
    int return_code;
    uint16_t error_code;

    clear_error_report();

    if (!internal_status.radio_on)
    {
        generate_iot_error_report(IOT_ERROR_RADIO_NOT_ON, 0, __LINE__);
        return IOT_ERROR_RADIO_NOT_ON;
    }

    if (IOT_RADIO_CELLULAR != internal_status.radio_type)
    {
        generate_iot_error_report(IOT_ERROR_NOT_SUPPORTED, 0, __LINE__);
        return IOT_ERROR_NOT_SUPPORTED;
    }

    return_code = syshal_cellular_check_sim((uint8_t *) imsi, &error_code);
    if (return_code)
    {
        generate_cellular_error_report(IOT_ERROR_CHECK_SIM_FAIL, return_code, error_code);
        return IOT_ERROR_CHECK_SIM_FAIL;
    }

    return IOT_NO_ERROR;
}

int iot_calc_prepass(const iot_last_gps_location_t *gps, iot_prepass_result_t *result)
{
    int return_code;

    return_code = syshal_sat_calc_prepass(gps, result);
    if (return_code)
    {
        generate_iot_error_report(IOT_ERROR_PREPAS_FAIL, 0, __LINE__);
        return IOT_ERROR_PREPAS_FAIL;
    }

    return IOT_NO_ERROR;
}

int iot_fetch_device_shadow(uint32_t timeout_ms, iot_device_shadow_t *shadow)
{
    int return_code;
    uint32_t bytes_written;
    uint16_t http_return_code;
    uint16_t error_code;

    clear_error_report();

    if (!internal_status.radio_on)
    {
        generate_iot_error_report(IOT_ERROR_RADIO_NOT_ON, 0, __LINE__);
        return IOT_ERROR_RADIO_NOT_ON;
    }

    if (IOT_RADIO_CELLULAR != internal_status.radio_type)
    {
        generate_iot_error_report(IOT_ERROR_NOT_SUPPORTED, 0, __LINE__);
        return IOT_ERROR_NOT_SUPPORTED;
    }

    if (!internal_status.connected)
    {
        generate_iot_error_report(IOT_ERROR_NOT_CONNECTED, 0, __LINE__);
        return IOT_ERROR_NOT_CONNECTED;
    }

    DEBUG_PR_TRACE("Fetching device shadow from: %s:%u%s", config.iot_cellular_aws_config->contents.arn, config.iot_cellular_aws_config->contents.port, device_shadow_path_full);

    return_code = syshal_cellular_https_get(timeout_ms, config.iot_cellular_aws_config->contents.arn, config.iot_cellular_aws_config->contents.port, device_shadow_path_full, &error_code);

    if (return_code)
    {
        generate_cellular_error_report(IOT_ERROR_GET_SHADOW_FAIL, return_code, error_code);
        return IOT_ERROR_GET_SHADOW_FAIL;
    }

    uint8_t file_working_buffer[JSON_WORKING_BUF_SIZE] = {0};
    return_code = syshal_cellular_read_from_file_to_buffer(file_working_buffer, sizeof(file_working_buffer), &bytes_written, &http_return_code);
    if (return_code)
    {
        generate_cellular_error_report(IOT_ERROR_READ_SHADOW_FAIL, return_code, http_return_code);
        if (SYSHAL_CELLULAR_ERROR_HTTP == return_code)
            DEBUG_PR_ERROR("HTTP connection failed with %d", http_return_code);
        return IOT_ERROR_READ_SHADOW_FAIL;
    }

    DEBUG_PR_TRACE("Remote device shadow: %.*s", JSON_WORKING_BUF_SIZE, file_working_buffer);

    return_code = aws_json_gets_device_shadow((char *) file_working_buffer, shadow, bytes_written);
    if (return_code)
    {
        generate_iot_error_report(IOT_ERROR_JSON, return_code, __LINE__);
        return IOT_ERROR_JSON;
    }

    return IOT_NO_ERROR;
}

int iot_send_device_status(uint32_t timeout_ms, const iot_device_status_t *device_status)
{
    int return_code;
    int json_length;
    uint16_t error_code;
    uint32_t bytes_to_send;

    clear_error_report();

    if (!internal_status.radio_on)
    {
        generate_iot_error_report(IOT_ERROR_RADIO_NOT_ON, 0, __LINE__);
        return IOT_ERROR_RADIO_NOT_ON;
    }

    if (internal_status.radio_type == IOT_RADIO_CELLULAR && !internal_status.connected)
    {
        generate_iot_error_report(IOT_ERROR_NOT_CONNECTED, 0, __LINE__);
        return IOT_ERROR_NOT_CONNECTED;
    }


    if (!device_status->presence_flags)
        return IOT_NO_ERROR; // No data to send

    switch (internal_status.radio_type)
    {
        case IOT_RADIO_CELLULAR:
        {
            DEBUG_PR_TRACE("Sending device status to: %s:%u%s", config.iot_cellular_aws_config->contents.arn, config.iot_cellular_aws_config->contents.port, device_shadow_path_full);

            uint8_t file_working_buffer[JSON_WORKING_BUF_SIZE] = {0};
            json_length = aws_json_dumps_device_status(device_status, (char *) file_working_buffer, sizeof(file_working_buffer));
            if (json_length < 0)
            {
                generate_iot_error_report(IOT_ERROR_JSON, json_length, __LINE__);
                return IOT_ERROR_JSON;
            }

            DEBUG_PR_TRACE("Sent device status: %.*s", JSON_WORKING_BUF_SIZE, file_working_buffer);

            return_code = syshal_cellular_write_from_buffer_to_file(file_working_buffer, json_length);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_WRITE_SHADOW_FAIL, return_code, 0);
                return IOT_ERROR_WRITE_SHADOW_FAIL;
            }

            return_code = syshal_cellular_https_post(timeout_ms, config.iot_cellular_aws_config->contents.arn, config.iot_cellular_aws_config->contents.port, device_shadow_path_full, &error_code);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_POST_SHADOW_FAIL, return_code, error_code);
                return IOT_ERROR_POST_SHADOW_FAIL;
            }
            break;
        }

        case IOT_RADIO_SATELLITE:
        {
            satellite_packet_t satellite_packet;
            uint32_t bytes_left = IOT_SAT_MAX_PAYLOAD_SIZE;
            uint8_t * data_ptr = &satellite_packet.payload[0];

            satellite_packet.hdr.type = IOT_SAT_TYPE_SEND_STATUS;
            satellite_packet.hdr.presence_flags = device_status->presence_flags;

            memset(satellite_packet.payload, 0, IOT_SAT_MAX_PAYLOAD_SIZE);

            // WARN: Do NOT change the order of the data packing unless you are modifying the protocol definition

            if (IOT_PRESENCE_FLAG_LAST_LOG_FILE_READ_POS & satellite_packet.hdr.presence_flags)
            {
                size_t member_size = sizeof(device_status->last_log_file_read_pos);
                if (member_size >= bytes_left)
                    goto packet_full;

                memcpy(data_ptr, &device_status->last_log_file_read_pos, member_size);
                data_ptr += member_size;
                bytes_left -= member_size;
            }

            if (IOT_PRESENCE_FLAG_LAST_GPS_LOCATION & satellite_packet.hdr.presence_flags)
            {
                size_t member_size = sizeof(device_status->last_gps_location);
                if (member_size >= bytes_left)
                    goto packet_full;

                memcpy(data_ptr, &device_status->last_gps_location, member_size);
                data_ptr += member_size;
                bytes_left -= member_size;
            }

            if (IOT_PRESENCE_FLAG_BATTERY_LEVEL & satellite_packet.hdr.presence_flags)
            {
                size_t member_size = sizeof(device_status->battery_level);
                if (member_size >= bytes_left)
                    goto packet_full;

                memcpy(data_ptr, &device_status->battery_level, member_size);
                data_ptr += member_size;
                bytes_left -= member_size;
            }

            if (IOT_PRESENCE_FLAG_BATTERY_VOLTAGE & satellite_packet.hdr.presence_flags)
            {
                size_t member_size = sizeof(device_status->battery_voltage);
                if (member_size >= bytes_left)
                    goto packet_full;

                memcpy(data_ptr, &device_status->battery_voltage, member_size);
                data_ptr += member_size;
                bytes_left -= member_size;
            }

            if (IOT_PRESENCE_FLAG_LAST_CELLULAR_CONNECTED_TIMESTAMP & satellite_packet.hdr.presence_flags)
            {
                size_t member_size = sizeof(device_status->last_cellular_connected_timestamp);
                if (member_size >= bytes_left)
                    goto packet_full;

                memcpy(data_ptr, &device_status->last_cellular_connected_timestamp, member_size);
                data_ptr += member_size;
                bytes_left -= member_size;
            }

            if (IOT_PRESENCE_FLAG_LAST_SAT_TX_TIMESTAMP & satellite_packet.hdr.presence_flags)
            {
                size_t member_size = sizeof(device_status->last_sat_tx_timestamp);
                if (member_size >= bytes_left)
                    goto packet_full;

                memcpy(data_ptr, &device_status->last_sat_tx_timestamp, member_size);
                data_ptr += member_size;
                bytes_left -= member_size;
            }

            if (IOT_PRESENCE_FLAG_NEXT_SAT_TX_TIMESTAMP & satellite_packet.hdr.presence_flags)
            {
                size_t member_size = sizeof(device_status->next_sat_tx_timestamp);
                if (member_size >= bytes_left)
                    goto packet_full;

                memcpy(data_ptr, &device_status->next_sat_tx_timestamp, member_size);
                data_ptr += member_size;
                bytes_left -= member_size;
            }

            if (IOT_PRESENCE_FLAG_CONFIGURATION_VERSION & satellite_packet.hdr.presence_flags)
            {
                size_t member_size = sizeof(device_status->configuration_version);
                if (member_size >= bytes_left)
                    goto packet_full;

                memcpy(data_ptr, &device_status->configuration_version, member_size);
                data_ptr += member_size;
                bytes_left -= member_size;
            }

            if (IOT_PRESENCE_FLAG_FIRMWARE_VERSION & satellite_packet.hdr.presence_flags)
            {
                size_t member_size = sizeof(device_status->firmware_version);
                if (member_size >= bytes_left)
                    goto packet_full;

                memcpy(data_ptr, &device_status->firmware_version, member_size);
                data_ptr += member_size;
                bytes_left -= member_size;
            }

packet_full:
            if(IOT_SAT_MAX_PAYLOAD_SIZE == bytes_left)
            {
                bytes_to_send = IOT_SAT_ZERO_PACKET;
            }
            else
            {
                bytes_to_send = sizeof(satellite_hdr_t) + IOT_SAT_MAX_PAYLOAD_SIZE - bytes_left;
            }
            DEBUG_PR_TRACE("Sending device status of length %lu to Argos using id: 0x%08lX", bytes_to_send, config.iot_sat_artic_config->contents.device_identifier);
            return_code = syshal_sat_send_message((uint8_t *) &satellite_packet, bytes_to_send);
            if(return_code)
            {
                generate_sat_error_report(IOT_ERROR_TRANSMISSION_FAIL, return_code);
                return IOT_ERROR_TRANSMISSION_FAIL;
            }
        break;
        }
        default:
            return IOT_INVALID_RADIO_TYPE;
            break;
    }

    return IOT_NO_ERROR;
}

/**
 * @brief Determines if a given log file contains valid log entries
 *
 * @param fs                        The file system
 * @param local_file_id             The log file id
 * @param log_file_read_pos         The start position to begin examining
 * @param length                    The amount of bytes to process
 * @param last_valid_entry_pos[out] Returns the position of the last valid tag found, NULL is permitted
 *
 * @return IOT_NO_ERROR on success
 * @return IOT_ERROR_LOG_FILE_IS_CORRUPT if the log file is corrupt
 */
static int is_log_file_corrupt(fs_t fs, uint32_t local_file_id, uint32_t log_file_read_pos, uint32_t length, uint32_t *last_valid_entry_pos)
{
    fs_handle_t handle;
    int return_code;
    uint32_t bytes_actually_read;

    clear_error_report();

    return_code = fs_open(fs, &handle, local_file_id, FS_MODE_READONLY, NULL);
    if (return_code)
    {
        generate_iot_error_report(IOT_ERROR_LOG_FILE_IS_CORRUPT, return_code, __LINE__);
        return IOT_ERROR_LOG_FILE_IS_CORRUPT;
    }

    return_code = fs_seek(handle, log_file_read_pos);
    if (return_code)
    {
        generate_iot_error_report(IOT_ERROR_LOG_FILE_IS_CORRUPT, return_code, __LINE__);
        fs_close(handle);
        return IOT_ERROR_LOG_FILE_IS_CORRUPT;
    }

    if (last_valid_entry_pos)
        *last_valid_entry_pos = log_file_read_pos;

    while (length)
    {
        // Read the tag value
        uint8_t tag_id;
        return_code = fs_read(handle, &tag_id, sizeof(tag_id), &bytes_actually_read);
        if (return_code || !bytes_actually_read)
        {
            generate_iot_error_report(IOT_ERROR_LOG_FILE_IS_CORRUPT, return_code, __LINE__);
            fs_close(handle);
            return IOT_ERROR_LOG_FILE_IS_CORRUPT;
        }

        // Is this tag valid?
        size_t tag_size;
        if (logging_tag_size(tag_id, &tag_size))
        {
            generate_iot_error_report(IOT_ERROR_LOG_FILE_IS_CORRUPT, return_code, __LINE__);
            fs_close(handle);
            return IOT_ERROR_LOG_FILE_IS_CORRUPT;
        }

        // Tag is valid but is all its data present?
        if (tag_size > length)
        {
            generate_iot_error_report(IOT_ERROR_LOG_FILE_IS_CORRUPT, return_code, __LINE__);
            fs_close(handle);
            return IOT_ERROR_LOG_FILE_IS_CORRUPT;
        }

        length -= tag_size;

        if (length)
        {
            return_code = fs_seek(handle, tag_size - sizeof(tag_id));
            if (return_code)
            {
                generate_iot_error_report(IOT_ERROR_LOG_FILE_IS_CORRUPT, return_code, __LINE__);
                fs_close(handle);
                return IOT_ERROR_LOG_FILE_IS_CORRUPT;
            }
        }

        if (last_valid_entry_pos)
            *last_valid_entry_pos += tag_size;
    }

    fs_close(handle);
    return IOT_NO_ERROR;
}

/**
 * @brief      Send a log file using IoT
 *
 * @param[in]  timeout_ms         The timeout_ms
 * @param[in]  fs                 The file system
 * @param[in]  local_file_id      The local file identifier
 * @param[in]  log_file_read_pos  The log file read position
 *
 * @return     The end log file position
 * @return     < 0 on failure
 */
int iot_send_logging(uint32_t timeout_ms, fs_t fs, uint32_t local_file_id, uint32_t log_file_read_pos)
{
    uint32_t total_bytes_to_send, bytes_to_send;
    uint8_t buffer[IOT_AWS_MAX_FILE_SIZE]; // WARN: This is big. Make sure the stack can handle it
    fs_handle_t handle;
    fs_stat_t stat;
    int return_code;
    uint16_t error_code;

    clear_error_report();

    if (!internal_status.radio_on)
    {
        generate_iot_error_report(IOT_ERROR_RADIO_NOT_ON, 0, __LINE__);
        return IOT_ERROR_RADIO_NOT_ON;
    }

    if (IOT_RADIO_CELLULAR != internal_status.radio_type)
    {
        generate_iot_error_report(IOT_ERROR_NOT_SUPPORTED, 0, __LINE__);
        return IOT_ERROR_NOT_SUPPORTED;
    }

    if (!internal_status.connected)
    {
        generate_iot_error_report(IOT_ERROR_NOT_CONNECTED, 0, __LINE__);
        return IOT_ERROR_NOT_CONNECTED;
    }

    return_code = fs_stat(fs, local_file_id, &stat);
    if (return_code)
    {
        generate_iot_error_report(IOT_ERROR_FILE_LOGGING_FAIL, return_code, __LINE__);
        return IOT_ERROR_FILE_LOGGING_FAIL;
    }

    if (stat.size < log_file_read_pos)
    {
        DEBUG_PR_WARN("Shadow log file position (%lu) is greater than log file size (%lu), starting from zero", log_file_read_pos, stat.size);
        log_file_read_pos = 0;
    }
    else if (stat.size == log_file_read_pos)
    {
        return log_file_read_pos; // There is no new data to send so return
    }

    total_bytes_to_send = stat.size - log_file_read_pos;

    return_code = is_log_file_corrupt(fs, local_file_id, log_file_read_pos, total_bytes_to_send, NULL);
    if (return_code)
    {
        DEBUG_PR_ERROR("Log file position %lu and length of %lu generates a log file corrupt error", log_file_read_pos, total_bytes_to_send);

        if (log_file_read_pos == 0)
        {
            // If our first byte is invalid then we can't recover
            generate_iot_error_report(IOT_ERROR_FILE_LOGGING_FAIL, return_code, __LINE__);
            return IOT_ERROR_FILE_LOGGING_FAIL;
        }

        // [NGPT-368] If this log file position is invalid then scan from the start to the nearest valid tag preceeding it and use that
        uint32_t last_valid_tag_pos = 0;
        return_code = is_log_file_corrupt(fs, local_file_id, 0, log_file_read_pos, &last_valid_tag_pos);
        if (return_code &&
            last_valid_tag_pos == 0)
        {
            // Error report is generated by is_log_file_corrupt()
            return IOT_ERROR_FILE_LOGGING_FAIL;
        }

        DEBUG_PR_WARN("Log file position recovery attempt. Moving from %lu to %lu", log_file_read_pos, last_valid_tag_pos);

        log_file_read_pos = last_valid_tag_pos;
        total_bytes_to_send = stat.size - log_file_read_pos;
    }

    DEBUG_PR_TRACE("Sending log file data of length %lu starting at position %lu", total_bytes_to_send, log_file_read_pos);

    return_code = fs_open(fs, &handle, local_file_id, FS_MODE_READONLY, NULL);
    if (return_code)
    {
        generate_iot_error_report(IOT_ERROR_FILE_LOGGING_FAIL, return_code, __LINE__);
        return IOT_ERROR_FILE_LOGGING_FAIL;
    }

    return_code = fs_seek(handle, log_file_read_pos);
    if (return_code)
    {
        generate_iot_error_report(IOT_ERROR_FILE_LOGGING_FAIL, return_code, __LINE__);
        fs_close(handle);
        return IOT_ERROR_FILE_LOGGING_FAIL;
    }

    uint32_t bytes_in_buffer = 0;

    bytes_to_send = total_bytes_to_send;
    while (bytes_to_send)
    {
        // Attempt to read the maximum amount we can to fill our working buffer
        uint32_t bytes_actually_read;
        return_code = fs_read(handle, &buffer[bytes_in_buffer], sizeof(buffer) - bytes_in_buffer, &bytes_actually_read);
        if (return_code)
        {
            generate_iot_error_report(IOT_ERROR_FILE_LOGGING_FAIL, return_code, __LINE__);
            fs_close(handle);
            return IOT_ERROR_FILE_LOGGING_FAIL;
        }

        bytes_in_buffer += bytes_actually_read;

        if (bytes_in_buffer >= bytes_to_send)
        {
            // We have reached the end of the file so write whatever data we have to the AWS file
            return_code = syshal_cellular_write_from_buffer_to_file(buffer, bytes_in_buffer);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_WRITE_LOGGING_FAIL, return_code, 0);
                fs_close(handle);
                return IOT_ERROR_WRITE_LOGGING_FAIL;
            }

            return_code = syshal_cellular_https_post(timeout_ms, config.iot_cellular_aws_config->contents.arn, config.iot_cellular_aws_config->contents.port, logging_topic_path_full, &error_code);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_POST_LOGGING_FAIL, return_code, error_code);
                fs_close(handle);
                return IOT_ERROR_POST_LOGGING_FAIL;
            }

            bytes_to_send -= bytes_in_buffer;
        }
        else
        {
            // If there is still data remaining then we need to ensure we don't send a partial log entry
            // if we did this we would break the AWS lamda functions causing data loss/corruption

            // To do this we must scan through all the log entries and find the last one that will fit

            uint32_t byte_idx = 0;
            while (byte_idx < sizeof(buffer))
            {
                size_t tag_size;
                if (logging_tag_size(buffer[byte_idx], &tag_size))
                {
                    generate_iot_error_report(IOT_ERROR_READ_LOGGING_FAIL, return_code, __LINE__);
                    fs_close(handle);
                    return IOT_ERROR_READ_LOGGING_FAIL;
                }

                if (byte_idx + tag_size > sizeof(buffer))
                {
                    // If this is a partial tag then exit
                    break;
                }

                byte_idx += tag_size;
            }

            // Send data up to and including the last full tag
            return_code = syshal_cellular_write_from_buffer_to_file(buffer, byte_idx);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_WRITE_LOGGING_FAIL, return_code, 0);
                fs_close(handle);
                return IOT_ERROR_WRITE_LOGGING_FAIL;
            }

            return_code = syshal_cellular_https_post(timeout_ms, config.iot_cellular_aws_config->contents.arn, config.iot_cellular_aws_config->contents.port, logging_topic_path_full, &error_code);
            if (return_code)
            {
                generate_cellular_error_report(IOT_ERROR_POST_LOGGING_FAIL, return_code, error_code);
                fs_close(handle);
                return IOT_ERROR_POST_LOGGING_FAIL;
            }

            // Move any partial tags to the start of the buffer
            bytes_to_send -= byte_idx;
            bytes_in_buffer = sizeof(buffer) - byte_idx;
            memcpy(buffer, &buffer[byte_idx], bytes_in_buffer);
        }

        iot_busy_handler();

    }

    fs_close(handle);
    return log_file_read_pos + total_bytes_to_send;
}

int iot_download_file(uint32_t timeout_ms, const iot_url_t *url, fs_t fs, uint32_t local_file_id, uint32_t *file_size)
{
    fs_handle_t handle;
    int return_code;
    uint16_t http_return_code;
    uint16_t error_code;

    DEBUG_PR_TRACE("%s()", __FUNCTION__);

    clear_error_report();

    if (!internal_status.radio_on)
    {
        generate_iot_error_report(IOT_ERROR_RADIO_NOT_ON, 0, __LINE__);
        return IOT_ERROR_RADIO_NOT_ON;
    }

    if (IOT_RADIO_CELLULAR != internal_status.radio_type)
    {
        generate_iot_error_report(IOT_ERROR_NOT_SUPPORTED, 0, __LINE__);
        return IOT_ERROR_NOT_SUPPORTED;
    }

    if (!internal_status.connected)
    {
        generate_iot_error_report(IOT_ERROR_NOT_CONNECTED, 0, __LINE__);
        return IOT_ERROR_NOT_CONNECTED;
    }

    return_code = syshal_cellular_https_get(timeout_ms, url->domain, url->port, url->path, &error_code);
    if (return_code)
    {
        generate_cellular_error_report(IOT_ERROR_GET_DOWNLOAD_FILE_FAIL, return_code, error_code);
        return IOT_ERROR_GET_DOWNLOAD_FILE_FAIL;
    }

    return_code = fs_delete(fs, local_file_id);
    if (FS_ERROR_FILE_NOT_FOUND != return_code)
    {
        generate_cellular_error_report(IOT_ERROR_FS_DOWNLOAD_FILE_FAIL, return_code, 0);
        return IOT_ERROR_FS_DOWNLOAD_FILE_FAIL;
    }

    if (fs_open(fs, &handle, local_file_id, FS_MODE_CREATE, NULL))
    {
        generate_cellular_error_report(IOT_ERROR_FS_DOWNLOAD_FILE_FAIL, return_code, 0);
        return IOT_ERROR_FS_DOWNLOAD_FILE_FAIL;
    }

    return_code = syshal_cellular_read_from_file_to_fs(handle, &http_return_code, file_size);
    if (return_code)
    {
        if (SYSHAL_CELLULAR_ERROR_HTTP == return_code)
        {
            DEBUG_PR_ERROR("HTTP connection failed with %d", http_return_code);
        }

        generate_cellular_error_report(IOT_ERROR_READ_DOWNLOAD_FILE_FAIL, return_code, http_return_code);

        fs_close(handle);
        return IOT_ERROR_READ_DOWNLOAD_FILE_FAIL;
    }

    fs_close(handle);
    return IOT_NO_ERROR;
}

inline void iot_get_error_report(iot_error_report_t *report_error)
{
    memcpy(report_error, &iot_report_error, sizeof(iot_error_report_t));
}

inline void iot_get_network_info(iot_network_info_t *network_info)
{
    memcpy(network_info, &iot_network_info, sizeof(iot_network_info_t));
}

/*! \brief IoT busy handler
 *
 * This handler function can be used to perform useful work
 * whilst the IoT layer is busy doing a potentially long task
 */
__attribute__((weak)) void iot_busy_handler(void)
{
    /* Do not modify -- override with your own handler function */
}