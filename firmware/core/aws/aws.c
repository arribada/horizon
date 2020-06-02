/* aws.c - Amazon Web Services abstraction layer
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

#include <inttypes.h>
#include <stdio.h>
#include "aws.h"
#include "json.h"

const char boilerplate_start[] = "{\"state\":{\"desired\":{\"device_status\":{";
const char boilerplate_end[] = "}}}}";

int aws_json_dumps_device_status(const iot_device_status_t * source, char * dest, size_t buffer_size)
{
    int written;
    size_t space_in_buffer = buffer_size;

    written = snprintf(dest, space_in_buffer, "%s", boilerplate_start);
    if ((size_t) written >= space_in_buffer)
        return AWS_ERROR_BUFFER_TOO_SMALL;
    if (written < 0)
        return AWS_ERROR_ENCODING;
    space_in_buffer -= written;
    dest += written;

    // last_log_file_read_pos
    if (source->presence_flags & IOT_PRESENCE_FLAG_LAST_LOG_FILE_READ_POS)
    {
        written = snprintf(dest, space_in_buffer, "\"last_log_file_read_pos\":%" PRIu32 ",", source->last_log_file_read_pos);
        if (written < 0)
            return AWS_ERROR_ENCODING;
        if ((size_t) written >= space_in_buffer)
            return AWS_ERROR_BUFFER_TOO_SMALL;
        space_in_buffer -= written;
        dest += written;
    }

    // last_gps_location
    if (source->presence_flags & IOT_PRESENCE_FLAG_LAST_GPS_LOCATION)
    {
        written = snprintf(dest, space_in_buffer, "\"last_gps_location\":{\"longitude\":%f,\"latitude\":%f,\"timestamp\":%" PRIu32 "},",
                           ((double)source->last_gps_location.longitude) / 10000000.0, // Convert from fixed point to floating
                           ((double)source->last_gps_location.latitude) / 10000000.0, // Convert from fixed point to floating
                           source->last_gps_location.timestamp);
        if (written < 0)
            return AWS_ERROR_ENCODING;
        if ((size_t) written >= space_in_buffer)
            return AWS_ERROR_BUFFER_TOO_SMALL;
        space_in_buffer -= written;
        dest += written;
    }

    // battery_level
    if (source->presence_flags & IOT_PRESENCE_FLAG_BATTERY_LEVEL)
    {
        // NOTE: We must use PRIu16 as PRIu8 is unsupported by newlib-nano
        written = snprintf(dest, space_in_buffer, "\"battery_level\":%" PRIu16 ",", (uint16_t) source->battery_level);
        if (written < 0)
            return AWS_ERROR_ENCODING;
        if ((size_t) written >= space_in_buffer)
            return AWS_ERROR_BUFFER_TOO_SMALL;
        space_in_buffer -= written;
        dest += written;
    }

    // battery_voltage
    if (source->presence_flags & IOT_PRESENCE_FLAG_BATTERY_VOLTAGE)
    {
        written = snprintf(dest, space_in_buffer, "\"battery_voltage\":%" PRIu16 ",", source->battery_voltage);
        if (written < 0)
            return AWS_ERROR_ENCODING;
        if ((size_t) written >= space_in_buffer)
            return AWS_ERROR_BUFFER_TOO_SMALL;
        space_in_buffer -= written;
        dest += written;
    }

    // last_cellular_connected_timestamp
    if (source->presence_flags & IOT_PRESENCE_FLAG_LAST_CELLULAR_CONNECTED_TIMESTAMP)
    {
        written = snprintf(dest, space_in_buffer, "\"last_cellular_connected_timestamp\":%" PRIu32 ",", source->last_cellular_connected_timestamp);
        if (written < 0)
            return AWS_ERROR_ENCODING;
        if ((size_t) written >= space_in_buffer)
            return AWS_ERROR_BUFFER_TOO_SMALL;
        space_in_buffer -= written;
        dest += written;
    }

    // last_sat_tx_timestamp
    if (source->presence_flags & IOT_PRESENCE_FLAG_LAST_SAT_TX_TIMESTAMP)
    {
        written = snprintf(dest, space_in_buffer, "\"last_sat_tx_timestamp\":%" PRIu32 ",", source->last_sat_tx_timestamp);
        if (written < 0)
            return AWS_ERROR_ENCODING;
        if ((size_t) written >= space_in_buffer)
            return AWS_ERROR_BUFFER_TOO_SMALL;
        space_in_buffer -= written;
        dest += written;
    }

    // next_sat_tx_timestamp
    if (source->presence_flags & IOT_PRESENCE_FLAG_NEXT_SAT_TX_TIMESTAMP)
    {
        written = snprintf(dest, space_in_buffer, "\"next_sat_tx_timestamp\":%" PRIu32 ",", source->next_sat_tx_timestamp);
        if (written < 0)
            return AWS_ERROR_ENCODING;
        if ((size_t) written >= space_in_buffer)
            return AWS_ERROR_BUFFER_TOO_SMALL;
        space_in_buffer -= written;
        dest += written;
    }

    // configuration_version
    if (source->presence_flags & IOT_PRESENCE_FLAG_CONFIGURATION_VERSION)
    {
        written = snprintf(dest, space_in_buffer, "\"configuration_version\":%" PRIu32 ",", source->configuration_version);
        if (written < 0)
            return AWS_ERROR_ENCODING;
        if ((size_t) written >= space_in_buffer)
            return AWS_ERROR_BUFFER_TOO_SMALL;
        space_in_buffer -= written;
        dest += written;
    }

    // firmware_version
    if (source->presence_flags & IOT_PRESENCE_FLAG_FIRMWARE_VERSION)
    {
        written = snprintf(dest, space_in_buffer, "\"firmware_version\":%" PRIu32 ",", source->firmware_version);
        if (written < 0)
            return AWS_ERROR_ENCODING;
        if ((size_t) written >= space_in_buffer)
            return AWS_ERROR_BUFFER_TOO_SMALL;
        space_in_buffer -= written;
        dest += written;
    }

    // If last character is a comma (,) then remove it to ensure this is valid JSON
    if (dest[-1] == ',')
    {
        dest[-1] = '\0';
        dest--;
    }

    written = snprintf(dest, space_in_buffer, "%s", boilerplate_end);
    if ((size_t) written >= space_in_buffer)
        return AWS_ERROR_BUFFER_TOO_SMALL;
    if (written < 0)
        return AWS_ERROR_ENCODING;
    space_in_buffer -= written;

    return buffer_size - space_in_buffer;
}

// Generate a custom format string with a fixed length to be used in scanf
static inline int scanf_format_str(char * buffer, size_t buffer_size, const char * type, uint32_t length)
{
    return snprintf(buffer, buffer_size, "%%%" PRIu32 "%s", length, type);
}

static void aws_json_parse_device_status(const char * source, iot_device_status_t * dest, size_t buffer_size)
{
    const char * device_status_ptr;
    const char * member_ptr;
    size_t device_status_len;
    size_t member_len;

    char format_str[24];
    int ret;

    // Assume nothing is present until proven otherwise
    dest->presence_flags = 0;

    // Work our way down the JSON tree until we are at the device_status
    device_status_ptr = json_parse("state", 0, source, buffer_size, &device_status_len);
    if (!device_status_ptr)
        return;

    device_status_ptr = json_parse("desired", 0, device_status_ptr, device_status_len, &device_status_len);
    if (!device_status_ptr)
        return;

    device_status_ptr = json_parse("device_status", 0, device_status_ptr, device_status_len, &device_status_len);
    if (!device_status_ptr)
        return;

    member_ptr = json_parse("last_log_file_read_pos", 0, device_status_ptr, device_status_len, &member_len);
    if (member_ptr)
    {
        scanf_format_str(format_str, sizeof(format_str), SCNu32, member_len);
        ret = sscanf(member_ptr, format_str, &dest->last_log_file_read_pos);
        if (ret >= 0)
            dest->presence_flags |= IOT_PRESENCE_FLAG_LAST_LOG_FILE_READ_POS;
    }

    member_ptr = json_parse("last_gps_location", 0, device_status_ptr, device_status_len, &member_len);
    if (member_ptr)
    {
        const char * sub_member;
        size_t sub_member_len;

        sub_member = json_parse("longitude", 0, member_ptr, member_len, &sub_member_len);
        if (!sub_member)
            goto invalid;
        scanf_format_str(format_str, sizeof(format_str), "f", sub_member_len);
        ret = sscanf(sub_member, format_str, &dest->last_gps_location.longitude);
        if (ret < 0)
            goto invalid;

        sub_member = json_parse("latitude", 0, member_ptr, member_len, &sub_member_len);
        if (!sub_member)
            goto invalid;
        scanf_format_str(format_str, sizeof(format_str), "f", sub_member_len);
        ret = sscanf(sub_member, format_str, &dest->last_gps_location.latitude);
        if (ret < 0)
            goto invalid;

        sub_member = json_parse("timestamp", 0, member_ptr, member_len, &sub_member_len);
        if (!sub_member)
            goto invalid;
        scanf_format_str(format_str, sizeof(format_str), SCNu32, sub_member_len);
        ret = sscanf(sub_member, format_str, &dest->last_gps_location.timestamp);
        if (ret < 0)
            goto invalid;

        dest->presence_flags |= IOT_PRESENCE_FLAG_LAST_GPS_LOCATION;
    }
invalid:

    member_ptr = json_parse("battery_level", 0, device_status_ptr, device_status_len, &member_len);
    if (member_ptr)
    {
        uint16_t temp_value; // NOTE: This is required as scanf of a SCNu8 is unsupported by newlib-nano
        scanf_format_str(format_str, sizeof(format_str), SCNu16, member_len);
        ret = sscanf(member_ptr, format_str, &temp_value);
        dest->battery_level = temp_value;
        if (ret >= 0)
            dest->presence_flags |= IOT_PRESENCE_FLAG_BATTERY_LEVEL;
    }

    member_ptr = json_parse("battery_voltage", 0, device_status_ptr, device_status_len, &member_len);
    if (member_ptr)
    {
        scanf_format_str(format_str, sizeof(format_str), SCNu16, member_len);
        ret = sscanf(member_ptr, format_str, &dest->battery_voltage);
        if (ret >= 0)
            dest->presence_flags |= IOT_PRESENCE_FLAG_BATTERY_VOLTAGE;
    }

    member_ptr = json_parse("last_cellular_connected_timestamp", 0, device_status_ptr, device_status_len, &member_len);
    if (member_ptr)
    {
        scanf_format_str(format_str, sizeof(format_str), SCNu32, member_len);
        ret = sscanf(member_ptr, format_str, &dest->last_cellular_connected_timestamp);
        if (ret >= 0)
            dest->presence_flags |= IOT_PRESENCE_FLAG_LAST_CELLULAR_CONNECTED_TIMESTAMP;
    }

    member_ptr = json_parse("last_sat_tx_timestamp", 0, device_status_ptr, device_status_len, &member_len);
    if (member_ptr)
    {
        scanf_format_str(format_str, sizeof(format_str), SCNu32, member_len);
        ret = sscanf(member_ptr, format_str, &dest->last_sat_tx_timestamp);
        if (ret >= 0)
            dest->presence_flags |= IOT_PRESENCE_FLAG_LAST_SAT_TX_TIMESTAMP;
    }

    member_ptr = json_parse("next_sat_tx_timestamp", 0, device_status_ptr, device_status_len, &member_len);
    if (member_ptr)
    {
        scanf_format_str(format_str, sizeof(format_str), SCNu32, member_len);
        ret = sscanf(member_ptr, format_str, &dest->next_sat_tx_timestamp);
        if (ret >= 0)
            dest->presence_flags |= IOT_PRESENCE_FLAG_NEXT_SAT_TX_TIMESTAMP;
    }

    member_ptr = json_parse("configuration_version", 0, device_status_ptr, device_status_len, &member_len);
    if (member_ptr)
    {
        scanf_format_str(format_str, sizeof(format_str), SCNu32, member_len);
        ret = sscanf(member_ptr, format_str, &dest->configuration_version);
        if (ret >= 0)
            dest->presence_flags |= IOT_PRESENCE_FLAG_CONFIGURATION_VERSION;
    }

    member_ptr = json_parse("firmware_version", 0, device_status_ptr, device_status_len, &member_len);
    if (member_ptr)
    {
        scanf_format_str(format_str, sizeof(format_str), SCNu32, member_len);
        ret = sscanf(member_ptr, format_str, &dest->firmware_version);
        if (ret >= 0)
            dest->presence_flags |= IOT_PRESENCE_FLAG_FIRMWARE_VERSION;
    }
}

static void aws_json_parse_device_update(const char * source, iot_device_update_t * dest, size_t buffer_size)
{
    const char * device_update_ptr;
    const char * url_ptr;
    const char * firmware_update_ptr;
    const char * config_update_ptr;
    const char * member_ptr;
    size_t device_update_len;
    size_t url_len;
    size_t firmware_update_len;
    size_t config_update_len;
    size_t member_len;

    char format_str[24];
    int ret;

    // Assume nothing is present until proven otherwise
    dest->presence_flags = 0;

    // Work our way down the JSON tree until we are at the device_update
    device_update_ptr = json_parse("state", 0, source, buffer_size, &device_update_len);
    if (!device_update_ptr)
        return;

    device_update_ptr = json_parse("desired", 0, device_update_ptr, device_update_len, &device_update_len);
    if (!device_update_ptr)
        return;

    device_update_ptr = json_parse("device_update", 0, device_update_ptr, device_update_len, &device_update_len);
    if (!device_update_ptr)
        return;

    // Configuration update
    config_update_ptr = json_parse("configuration_update", 0, device_update_ptr, device_update_len, &config_update_len);
    if (!config_update_ptr)
        goto invalid_config;

    member_ptr = json_parse("version", 0, config_update_ptr, config_update_len, &member_len);
    if (!member_ptr)
        goto invalid_config;

    scanf_format_str(format_str, sizeof(format_str), SCNu32, member_len);
    ret = sscanf(member_ptr, format_str, &dest->configuration_update.version);
    if (ret < 0)
        goto invalid_config;

    url_ptr = json_parse("url", 0, config_update_ptr, config_update_len, &url_len);
    if (!member_ptr)
        goto invalid_config;

    member_ptr = json_parse("domain", 0, url_ptr, url_len, &member_len);
    if (!member_ptr)
        goto invalid_config;
    scanf_format_str(format_str, sizeof(format_str), "s", member_len);
    ret = sscanf(member_ptr, format_str, &dest->configuration_update.url.domain);
    if (ret < 0)
        goto invalid_config;

    member_ptr = json_parse("path", 0, url_ptr, url_len, &member_len);
    if (!member_ptr)
        goto invalid_config;
    scanf_format_str(format_str, sizeof(format_str), "s", member_len);
    ret = sscanf(member_ptr, format_str, &dest->configuration_update.url.path);
    if (ret < 0)
        goto invalid_config;

    member_ptr = json_parse("port", 0, url_ptr, url_len, &member_len);
    if (!member_ptr)
        goto invalid_config;
    scanf_format_str(format_str, sizeof(format_str), SCNu16, member_len);
    ret = sscanf(member_ptr, format_str, &dest->configuration_update.url.port);
    if (ret < 0)
        goto invalid_config;

    dest->presence_flags |= IOT_DEVICE_UPDATE_PRESENCE_FLAG_CONFIGURATION_UPDATE;

invalid_config:

    // Firmware update
    firmware_update_ptr = json_parse("firmware_update", 0, device_update_ptr, device_update_len, &firmware_update_len);
    if (!firmware_update_ptr)
        goto invalid_firmware;

    member_ptr = json_parse("version", 0, firmware_update_ptr, firmware_update_len, &member_len);
    if (!member_ptr)
        goto invalid_firmware;

    scanf_format_str(format_str, sizeof(format_str), SCNu32, member_len);
    ret = sscanf(member_ptr, format_str, &dest->firmware_update.version);
    if (ret < 0)
        goto invalid_firmware;

    url_ptr = json_parse("url", 0, firmware_update_ptr, firmware_update_len, &url_len);
    if (!member_ptr)
        goto invalid_firmware;

    member_ptr = json_parse("domain", 0, url_ptr, url_len, &member_len);
    if (!member_ptr)
        goto invalid_firmware;
    scanf_format_str(format_str, sizeof(format_str), "s", member_len);
    ret = sscanf(member_ptr, format_str, &dest->firmware_update.url.domain);
    if (ret < 0)
        goto invalid_firmware;

    member_ptr = json_parse("path", 0, url_ptr, url_len, &member_len);
    if (!member_ptr)
        goto invalid_firmware;
    scanf_format_str(format_str, sizeof(format_str), "s", member_len);
    ret = sscanf(member_ptr, format_str, &dest->firmware_update.url.path);
    if (ret < 0)
        goto invalid_firmware;

    member_ptr = json_parse("port", 0, url_ptr, url_len, &member_len);
    if (!member_ptr)
        goto invalid_firmware;
    scanf_format_str(format_str, sizeof(format_str), SCNu16, member_len);
    ret = sscanf(member_ptr, format_str, &dest->firmware_update.url.port);
    if (ret < 0)
        goto invalid_firmware;

    dest->presence_flags |= IOT_DEVICE_UPDATE_PRESENCE_FLAG_FIRMWARE_UPDATE;

invalid_firmware:

    return;
}

int aws_json_gets_device_shadow(const char * source, iot_device_shadow_t * dest, size_t buffer_size)
{
    aws_json_parse_device_status(source, &dest->device_status, buffer_size);
    aws_json_parse_device_update(source, &dest->device_update, buffer_size);

    return AWS_NO_ERROR;
}