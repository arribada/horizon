/* sys_config.c - Configuration data and tag handling
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

#include <string.h>
#include "sys_config.h"
#include "sys_config_priv.h"
#include "syshal_rtc.h"
#include "debug.h"
#include "sm_main.h"
#include "crc_32.h"

// Exposed variables
sys_config_t sys_config; // Configuration data stored in RAM

static void rtc_date_time_getter(void);
static void rtc_date_time_setter(void);
static void logging_file_size_getter(void);
static void logging_file_type_getter(void);

static const sys_config_lookup_table_t sys_config_lookup_table[] =
{
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_SYSTEM_DEVICE_IDENTIFIER,                  sys_config.system_device_identifier,                     true),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_VERSION,                                   sys_config.version,                                      false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE,                   sys_config.gps_log_position_enable,                      false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_LOG_TTFF_ENABLE,                       sys_config.gps_log_ttff_enable,                          false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_TRIGGER_MODE,                          sys_config.gps_trigger_mode,                             false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_SCHEDULED_ACQUISITION_INTERVAL,        sys_config.gps_scheduled_acquisition_interval,           false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_MAXIMUM_ACQUISITION_TIME,              sys_config.gps_maximum_acquisition_time,                 false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_SCHEDULED_ACQUISITION_NO_FIX_TIMEOUT,  sys_config.gps_scheduled_acquisition_no_fix_timeout,     false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_LAST_KNOWN_POSITION,                   sys_config.gps_last_known_position,                      false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_TEST_FIX_HOLD_TIME,                    sys_config.gps_test_fix_hold_time,                       false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_DEBUG_LOGGING_ENABLE,                  sys_config.gps_debug_logging_enable,                     false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_GPS_MAX_FIXES,                             sys_config.gps_max_fixes,                                false),
    SYS_CONFIG_TAG(SYS_CONFIG_SALTWATER_SWITCH_LOG_ENABLE,                   sys_config.saltwater_switch_log_enable,                  false),
    SYS_CONFIG_TAG(SYS_CONFIG_SALTWATER_SWITCH_HYSTERESIS_PERIOD,            sys_config.saltwater_switch_hysteresis_period,           false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_RTC_SYNC_TO_GPS_ENABLE,                    sys_config.rtc_sync_to_gps_enable,                       false),
    SYS_CONFIG_TAG_GET_SET(SYS_CONFIG_TAG_RTC_CURRENT_DATE_AND_TIME,         sys_config.rtc_current_date_and_time,                    false, rtc_date_time_getter, rtc_date_time_setter),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_LOGGING_ENABLE,                            sys_config.logging_enable,                               false),
    SYS_CONFIG_TAG_GET_SET(SYS_CONFIG_TAG_LOGGING_FILE_SIZE,                 sys_config.logging_file_size,                            false, logging_file_size_getter, NULL),
    SYS_CONFIG_TAG_GET_SET(SYS_CONFIG_TAG_LOGGING_FILE_TYPE,                 sys_config.logging_file_type,                            false, logging_file_type_getter, NULL),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_LOGGING_GROUP_SENSOR_READINGS_ENABLE,      sys_config.logging_group_sensor_readings_enable,         false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_LOGGING_START_END_SYNC_ENABLE,             sys_config.logging_start_end_sync_enable,                false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_LOGGING_DATE_TIME_STAMP_ENABLE,            sys_config.logging_date_time_stamp_enable,               false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_LOGGING_HIGH_RESOLUTION_TIMER_ENABLE,      sys_config.logging_high_resolution_timer_enable,         false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_AXL_LOG_ENABLE,                            sys_config.axl_log_enable,                               false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_AXL_CONFIG,                                sys_config.axl_config,                                   false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_AXL_G_FORCE_HIGH_THRESHOLD,                sys_config.axl_g_force_high_threshold,                   false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_AXL_SAMPLE_RATE,                           sys_config.axl_sample_rate,                              false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_AXL_MODE,                                  sys_config.axl_mode,                                     false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_AXL_SCHEDULED_ACQUISITION_INTERVAL,        sys_config.axl_scheduled_acquisition_interval,           false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_AXL_MAXIMUM_ACQUISITION_TIME,              sys_config.axl_maximum_acquisition_time,                 false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_PRESSURE_SENSOR_LOG_ENABLE,                sys_config.pressure_sensor_log_enable,                   false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_PRESSURE_SAMPLE_RATE,                      sys_config.pressure_sample_rate,                         false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_PRESSURE_LOW_THRESHOLD,                    sys_config.pressure_low_threshold,                       false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_PRESSURE_HIGH_THRESHOLD,                   sys_config.pressure_high_threshold,                      false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_PRESSURE_MODE,                             sys_config.pressure_mode,                                false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_PRESSURE_SCHEDULED_ACQUISITION_INTERVAL,   sys_config.pressure_scheduled_acquisition_interval,      false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_PRESSURE_MAXIMUM_ACQUISITION_TIME,         sys_config.pressure_maximum_acquisition_time,            false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_TEMP_SENSOR_LOG_ENABLE,                    sys_config.temp_sensor_log_enable,                       false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_TEMP_SENSOR_SAMPLE_RATE,                   sys_config.temp_sensor_sample_rate,                      false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_TEMP_SENSOR_LOW_THRESHOLD,                 sys_config.temp_sensor_low_threshold,                    false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_TEMP_SENSOR_HIGH_THRESHOLD,                sys_config.temp_sensor_high_threshold,                   false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_TEMP_SENSOR_MODE,                          sys_config.temp_sensor_mode,                             false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_DEVICE_ADDRESS,                  sys_config.tag_bluetooth_device_address,                 false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_TRIGGER_CONTROL,                 sys_config.tag_bluetooth_trigger_control,                false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_SCHEDULED_INTERVAL,              sys_config.tag_bluetooth_scheduled_interval,             false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_SCHEDULED_DURATION,              sys_config.tag_bluetooth_scheduled_duration,             false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_ADVERTISING_INTERVAL,            sys_config.tag_bluetooth_advertising_interval,           false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_CONNECTION_INTERVAL,             sys_config.tag_bluetooth_connection_interval,            false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_CONNECTION_INACTIVITY_TIMEOUT,   sys_config.tag_bluetooth_connection_inactivity_timeout,  false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_PHY_MODE,                        sys_config.tag_bluetooth_phy_mode,                       false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_LOG_ENABLE,                      sys_config.tag_bluetooth_log_enable,                     false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BLUETOOTH_ADVERTISING_TAGS,                sys_config.tag_bluetooth_advertising_tags,               false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_IOT_GENERAL_SETTINGS,                      sys_config.iot_general_settings,                         false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_IOT_CELLULAR_SETTINGS,                     sys_config.iot_cellular_settings,                        false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_IOT_CELLULAR_AWS_SETTINGS,                 sys_config.iot_cellular_aws_settings,                    false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_IOT_CELLULAR_APN,                          sys_config.iot_cellular_apn,                             false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_IOT_SATELLITE_SETTINGS,                    sys_config.iot_sat_settings,                             false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_IOT_SATELLITE_ARTIC_SETTINGS,              sys_config.iot_sat_artic_settings,                       false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BATTERY_LOG_ENABLE,                        sys_config.battery_log_enable,                           false),
    SYS_CONFIG_TAG(SYS_CONFIG_TAG_BATTERY_LOW_THRESHOLD,                     sys_config.battery_low_threshold,                        false),
};

static const dependancy_lookup_table_t dependancy_lookup_table[] =
{
    // If GPS is enabled
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_GPS_TRIGGER_MODE,                          SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, sys_config.gps_log_position_enable.contents.enable, 1),
    // If GPS is in scheduled mode
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_GPS_SCHEDULED_ACQUISITION_INTERVAL,        SYS_CONFIG_TAG_GPS_TRIGGER_MODE, sys_config.gps_trigger_mode.contents.mode, SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_GPS_MAXIMUM_ACQUISITION_TIME,              SYS_CONFIG_TAG_GPS_TRIGGER_MODE, sys_config.gps_trigger_mode.contents.mode, SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED),
    // If GPS is in hybrid mode
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_GPS_SCHEDULED_ACQUISITION_INTERVAL,        SYS_CONFIG_TAG_GPS_TRIGGER_MODE, sys_config.gps_trigger_mode.contents.mode, SYS_CONFIG_GPS_TRIGGER_MODE_HYBRID),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_GPS_MAXIMUM_ACQUISITION_TIME,              SYS_CONFIG_TAG_GPS_TRIGGER_MODE, sys_config.gps_trigger_mode.contents.mode, SYS_CONFIG_GPS_TRIGGER_MODE_HYBRID),

    // If Temperature is enabled
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_TEMP_SENSOR_MODE,         SYS_CONFIG_TAG_TEMP_SENSOR_LOG_ENABLE, sys_config.temp_sensor_log_enable.contents.enable, 1),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_TEMP_SENSOR_SAMPLE_RATE,  SYS_CONFIG_TAG_TEMP_SENSOR_LOG_ENABLE, sys_config.temp_sensor_log_enable.contents.enable, 1),
    // If Temperature mode is trigger below
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_TEMP_SENSOR_LOW_THRESHOLD, SYS_CONFIG_TAG_TEMP_SENSOR_MODE, sys_config.temp_sensor_mode.contents.mode, SYS_CONFIG_TEMP_MODE_TRIGGER_BELOW),
    // If Temperature mode is trigger above
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_TEMP_SENSOR_HIGH_THRESHOLD, SYS_CONFIG_TAG_TEMP_SENSOR_MODE, sys_config.temp_sensor_mode.contents.mode, SYS_CONFIG_TEMP_MODE_TRIGGER_ABOVE),
    // If Temperature mode is trigger between
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_TEMP_SENSOR_LOW_THRESHOLD,  SYS_CONFIG_TAG_TEMP_SENSOR_MODE, sys_config.temp_sensor_mode.contents.mode, SYS_CONFIG_TEMP_MODE_TRIGGER_BETWEEN),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_TEMP_SENSOR_HIGH_THRESHOLD, SYS_CONFIG_TAG_TEMP_SENSOR_MODE, sys_config.temp_sensor_mode.contents.mode, SYS_CONFIG_TEMP_MODE_TRIGGER_BETWEEN),

    // If Pressure is enabled
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_PRESSURE_SAMPLE_RATE,  SYS_CONFIG_TAG_PRESSURE_SENSOR_LOG_ENABLE, sys_config.pressure_sensor_log_enable.contents.enable, 1),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_PRESSURE_MODE,         SYS_CONFIG_TAG_PRESSURE_SENSOR_LOG_ENABLE, sys_config.pressure_sensor_log_enable.contents.enable, 1),
    // If Pressure mode is periodic
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_PRESSURE_SCHEDULED_ACQUISITION_INTERVAL, SYS_CONFIG_TAG_PRESSURE_MODE, sys_config.pressure_mode.contents.mode, SYS_CONFIG_PRESSURE_MODE_PERIODIC),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_PRESSURE_MAXIMUM_ACQUISITION_TIME,       SYS_CONFIG_TAG_PRESSURE_MODE, sys_config.pressure_mode.contents.mode, SYS_CONFIG_PRESSURE_MODE_PERIODIC),
    // If Pressure mode is trigger below
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_PRESSURE_LOW_THRESHOLD,  SYS_CONFIG_TAG_TEMP_SENSOR_MODE, sys_config.temp_sensor_mode.contents.mode, SYS_CONFIG_PRESSURE_MODE_TRIGGER_BELOW),
    // If Pressure mode is trigger above
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_PRESSURE_HIGH_THRESHOLD, SYS_CONFIG_TAG_TEMP_SENSOR_MODE, sys_config.temp_sensor_mode.contents.mode, SYS_CONFIG_PRESSURE_MODE_TRIGGER_ABOVE),
    // If Pressure mode is trigger between
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_PRESSURE_LOW_THRESHOLD,  SYS_CONFIG_TAG_TEMP_SENSOR_MODE, sys_config.temp_sensor_mode.contents.mode, SYS_CONFIG_PRESSURE_MODE_TRIGGER_BETWEEN),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_PRESSURE_HIGH_THRESHOLD, SYS_CONFIG_TAG_TEMP_SENSOR_MODE, sys_config.temp_sensor_mode.contents.mode, SYS_CONFIG_PRESSURE_MODE_TRIGGER_BETWEEN),

    // If AXL is enabled
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_AXL_MODE,        SYS_CONFIG_TAG_AXL_LOG_ENABLE, sys_config.axl_log_enable.contents.enable, 1),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_AXL_SAMPLE_RATE, SYS_CONFIG_TAG_AXL_LOG_ENABLE, sys_config.axl_log_enable.contents.enable, 1),
    // If AXL mode is periodic
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_AXL_SCHEDULED_ACQUISITION_INTERVAL, SYS_CONFIG_TAG_AXL_MODE, sys_config.axl_mode.contents.mode, SYS_CONFIG_AXL_MODE_PERIODIC),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_AXL_MAXIMUM_ACQUISITION_TIME,       SYS_CONFIG_TAG_AXL_MODE, sys_config.axl_mode.contents.mode, SYS_CONFIG_AXL_MODE_PERIODIC),
    // If AXL mode is trigger above
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_AXL_G_FORCE_HIGH_THRESHOLD,  SYS_CONFIG_TAG_AXL_MODE, sys_config.axl_mode.contents.mode, SYS_CONFIG_AXL_MODE_TRIGGER_ABOVE),

    // If IOT Cellular is enabled
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_IOT_CELLULAR_AWS_SETTINGS, SYS_CONFIG_TAG_IOT_CELLULAR_SETTINGS, sys_config.iot_cellular_settings.contents.enable, 1),
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_IOT_CELLULAR_APN, SYS_CONFIG_TAG_IOT_CELLULAR_SETTINGS, sys_config.iot_general_settings.contents.enable, 1),
    // If IOT Satellite is enabled
    SYS_CONFIG_REQUIRED_IF_MATCH(SYS_CONFIG_TAG_IOT_SATELLITE_ARTIC_SETTINGS, SYS_CONFIG_TAG_IOT_SATELLITE_SETTINGS, sys_config.iot_sat_settings.contents.enable, 1),
};

static void rtc_date_time_getter(void)
{
    // Populate the sys_config values with the current date/time
    syshal_rtc_data_and_time_t date_time;
    syshal_rtc_get_date_and_time(&date_time);

    sys_config.rtc_current_date_and_time.contents.day = date_time.day;
    sys_config.rtc_current_date_and_time.contents.month = date_time.month;
    sys_config.rtc_current_date_and_time.contents.year = date_time.year;
    sys_config.rtc_current_date_and_time.contents.hours = date_time.hours;
    sys_config.rtc_current_date_and_time.contents.minutes = date_time.minutes;
    sys_config.rtc_current_date_and_time.contents.seconds = date_time.seconds;

    sys_config.rtc_current_date_and_time.hdr.set = true;
}

static void rtc_date_time_setter(void)
{
    // Set the date time based on tag settings
    syshal_rtc_data_and_time_t date_time;

    date_time.day = sys_config.rtc_current_date_and_time.contents.day;
    date_time.month = sys_config.rtc_current_date_and_time.contents.month;
    date_time.year = sys_config.rtc_current_date_and_time.contents.year;
    date_time.hours = sys_config.rtc_current_date_and_time.contents.hours;
    date_time.minutes = sys_config.rtc_current_date_and_time.contents.minutes;
    date_time.seconds = sys_config.rtc_current_date_and_time.contents.seconds;
    date_time.milliseconds = 0;

    syshal_rtc_set_date_and_time(date_time);
}

static void logging_file_size_getter(void)
{
    fs_stat_t stat;

    if (fs_stat(file_system, FILE_ID_LOG, &stat) == FS_NO_ERROR)
    {
        sys_config.logging_file_size.contents.file_size = stat.size;
        sys_config.logging_file_size.hdr.set = true;
    }
    else
    {
        sys_config.logging_file_size.hdr.set = false;
    }
}

static void logging_file_type_getter(void)
{
    fs_stat_t stat;

    if (fs_stat(file_system, FILE_ID_LOG, &stat) == FS_NO_ERROR)
    {
        sys_config.logging_file_type.contents.file_type = stat.is_circular ? SYS_CONFIG_TAG_LOGGING_FILE_TYPE_CIRCULAR : SYS_CONFIG_TAG_LOGGING_FILE_TYPE_LINEAR;
        sys_config.logging_file_type.hdr.set = true;
    }
    else
    {
        sys_config.logging_file_type.hdr.set = false;
    }
}

static int sys_config_get_index(uint16_t tag, uint32_t * index)
{
    for (uint32_t i = 0; i < NUM_OF_TAGS; ++i)
    {
        if (sys_config_lookup_table[i].tag == tag)
        {
            *index = i;
            return SYS_CONFIG_NO_ERROR;
        }
    }

    return SYS_CONFIG_ERROR_INVALID_TAG;
}

/**
 * @brief      Checks to see if the given tag exists
 *
 * @param[in]  tag   The configuration tag
 *
 * @return     true if exists
 * @return     false if does not exist
 */
bool sys_config_exists(uint16_t tag)
{
    uint32_t index;
    int ret = sys_config_get_index(tag, &index);

    if (SYS_CONFIG_NO_ERROR == ret)
        return true;
    else
        return false;
}

/**
 * @brief      Checks to see if the given tag is set
 *
 * @param[in]  tag   The configuration tag
 * @param[out] set   True or False if tag is set
 *
 * @return     SYS_CONFIG_NO_ERROR on success
 * @return     SYS_CONFIG_ERROR_INVALID_TAG if the given tag is invalid
 */
int sys_config_is_set(uint16_t tag, bool * set)
{
    uint32_t index;
    int ret = sys_config_get_index(tag, &index);

    if (ret)
        return ret;

    *set = TAG_SET_FLAG(sys_config_lookup_table[index]);
    return SYS_CONFIG_NO_ERROR;
}

/**
 * @brief      Clear the given configuration tag
 *
 * @param[in]  tag   The configuration tag
 *
 * @return     SYS_CONFIG_NO_ERROR on success
 * @return     SYS_CONFIG_ERROR_INVALID_TAG if the given tag is invalid
 */
int sys_config_unset(uint16_t tag)
{
    uint32_t index;
    int ret = sys_config_get_index(tag, &index);

    if (ret)
        return ret;

    TAG_SET_FLAG(sys_config_lookup_table[index]) = false;
    return SYS_CONFIG_NO_ERROR;
}

/**
 * @brief      Get data size of the given configuration tag
 *
 * @param[in]  tag   The configuration tag
 * @param      size  The configuration tag's size
 *
 * @return     SYS_CONFIG_NO_ERROR on success
 * @return     SYS_CONFIG_ERROR_INVALID_TAG if the given tag is invalid
 */
int sys_config_size(uint16_t tag, size_t * size)
{
    uint32_t index;
    int ret = sys_config_get_index(tag, &index);

    if (ret)
        return ret;

    *size = sys_config_lookup_table[index].length;
    return SYS_CONFIG_NO_ERROR;
}

/**
 * @brief      Determine if a configuration tag needs to be set or not to
 *             enter operational state
 *
 * @param[in]  tag       The configuration tag
 * @param[out] required  True if the given tag needs to be set, False otherwise
 *
 * @return     SYS_CONFIG_NO_ERROR on success
 * @return     SYS_CONFIG_ERROR_INVALID_TAG if the given tag is invalid
 */
int sys_config_is_required(uint16_t tag, bool * required)
{
    uint32_t index;
    int ret = sys_config_get_index(tag, &index);

    // Is this tag valid?
    if (ret)
        return ret;

    // Is this tag compulsory?
    if (sys_config_lookup_table[index].compulsory && !TAG_SET_FLAG(sys_config_lookup_table[index]))
    {
        // If it's not set then it is required
        *required = true;
        return SYS_CONFIG_NO_ERROR;
    }

    *required = false;

    // Scan through all the dependencies to see if another tag requires this one
    for (uint32_t i = 0; i < NUM_OF_DEPENDENCIES; ++i)
    {
        // If there's a mention of this tag in a dependency
        if (dependancy_lookup_table[i].tag == tag)
        {
            // Is this other tag valid?
            bool is_set;
            if (SYS_CONFIG_NO_ERROR == sys_config_is_set(dependancy_lookup_table[i].tag_dependant, &is_set))
            {
                // Is this other tag set?
                if (!is_set)
                    continue;

                // Does this tag value match that which is required to trigger this dependancy?
                if ((*(uint32_t *)dependancy_lookup_table[i].address & dependancy_lookup_table[i].bitmask) == dependancy_lookup_table[i].value)
                {
                    // Is the tag it relies on set and thus it's dependency sorted?
                    if (sys_config_is_set(dependancy_lookup_table[i].tag, &is_set))
                        continue;

                    if (is_set)
                        continue;

                    // A dependency is not fulfilled
                    *required = true;
                    break;
                }
            }
        }
    }

    return SYS_CONFIG_NO_ERROR;
}

/**
 * @brief      Gets the value in the given configuration tag
 *
 * @param[in]  tag     The configuration tag
 * @param[out] value   The pointer to the configuration data
 *
 * @return     The length of the data read on success
 * @return     SYS_CONFIG_ERROR_INVALID_TAG if the given tag is invalid
 * @return     SYS_CONFIG_ERROR_TAG_NOT_SET if the tag hasn't been set
 */
int sys_config_get(uint16_t tag, void ** value)
{
    uint32_t index;
    int ret = sys_config_get_index(tag, &index);

    if (ret)
        return ret;

    if (sys_config_lookup_table[index].getter)
        sys_config_lookup_table[index].getter();

    if (!TAG_SET_FLAG(sys_config_lookup_table[index]))
        return SYS_CONFIG_ERROR_TAG_NOT_SET;

    // Get the address of the configuration data in RAM
    if (value != NULL)
        (*value) = ((uint8_t *)sys_config_lookup_table[index].data) + sizeof(sys_config_hdr_t);

    return sys_config_lookup_table[index].length;
}

/**
 * @brief      Set the configuration tag to the given value
 *
 * @param[in]  tag     The configuration tag
 * @param[in]  data    The value to set the tag to
 * @param[in]  length  The length of the data to be written
 *
 * @return     SYS_CONFIG_NO_ERROR on success
 * @return     SYS_CONFIG_ERROR_INVALID_TAG if the given tag is invalid
 * @return     SYS_CONFIG_ERROR_WRONG_SIZE if the given length does not meet the
 *             configuration tag length
 */
int sys_config_set(uint16_t tag, const void * data, size_t length)
{
    uint32_t index;
    int ret = sys_config_get_index(tag, &index);

    if (ret)
        return ret;

    if (sys_config_lookup_table[index].length != length)
        return SYS_CONFIG_ERROR_WRONG_SIZE;

    // Copy in the data to the contents section
    memcpy(((uint8_t *)sys_config_lookup_table[index].data) + sizeof(sys_config_hdr_t), data, length);

    // Set the set flag
    TAG_SET_FLAG(sys_config_lookup_table[index]) = true;

    if (sys_config_lookup_table[index].setter)
        sys_config_lookup_table[index].setter();

    return SYS_CONFIG_NO_ERROR;
}

/**
 * @brief      Returns the next tag after the given tag. This is used for
 *             iterating through all the tags
 *
 * @param[in]  tag         Pointer to tag value to populate
 * @param      last_index  Private variable for maintaining state. For the first
 *                         call pass a pointer to a uint32_t set to 0
 *
 * @return     SYS_CONFIG_NO_ERROR on success
 * @return     SYS_CONFIG_ERROR_NO_MORE_TAGS if there are no more tags after this
 *             one
 */
int sys_config_iterate(uint16_t * tag, uint16_t * last_index)
{
    uint16_t idx = *last_index;
    if (idx >= NUM_OF_TAGS)
        return SYS_CONFIG_ERROR_NO_MORE_TAGS;

    // Return the next tag in the list
    (*last_index)++;
    *tag = sys_config_lookup_table[idx].tag;

    return SYS_CONFIG_NO_ERROR;
}

/**
 * @brief      Load the configuration data from FLASH.
 *
 * @param[in]  fs       The file system
 * @param[in]  file_id  The file identifier
 * @param[out] config   The configuration struct to load the data into
 *
 * @return     SYS_CONFIG_NO_ERROR if the configuration file is valid
 * @return     SYS_CONFIG_ERROR_FS an error occured in the file system
 * @return     SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND if no valid
 *             configuration file is found
 */
static int get_configuration_data_from_fs(fs_t fs, uint8_t file_id, sys_config_t * config)
{
    fs_handle_t file_handle;
    uint32_t total_bytes_read = 0;
    uint32_t bytes_read;
    uint32_t calculated_crc32;
    uint32_t stored_crc32;
    int ret;

    // Populate the sys_config struct from the FLASH file
    ret = fs_open(fs, &file_handle, file_id, FS_MODE_READONLY, NULL);
    if (FS_ERROR_FILE_NOT_FOUND == ret)
        return SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND;
    else if (ret)
        return SYS_CONFIG_ERROR_FS;

    // Read the sys_config struct out of the file system
    ret = fs_read(file_handle, config, sizeof(sys_config_t), &bytes_read);
    if (ret)
    {
        fs_close(file_handle);
        if (FS_ERROR_END_OF_FILE == ret)
        {
            fs_delete(fs, file_id); // If the file is empty, remove it
            return SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND;
        }

        return SYS_CONFIG_ERROR_FS;
    }
    total_bytes_read += bytes_read;

    // Read the sys_config crc32 out of the file system
    ret = fs_read(file_handle, &stored_crc32, sizeof(stored_crc32), &bytes_read);
    if (ret)
    {
        fs_close(file_handle);
        if (FS_ERROR_END_OF_FILE == ret)
        {
            fs_delete(fs, file_id); // If the file does not contain a crc, remove it
            return SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND;
        }

        return SYS_CONFIG_ERROR_FS;
    }
    total_bytes_read += bytes_read;

    ret = fs_close(file_handle);
    if (ret)
        return SYS_CONFIG_ERROR_FS;

    calculated_crc32 = crc32(0, config, sizeof(sys_config_t));

    if (calculated_crc32 != stored_crc32)
    {
        DEBUG_PR_WARN("%s() configuration file has an invalid crc32. Read 0x%08lX, got 0x%08lX. Deleting it ID: %u", __FUNCTION__, stored_crc32, calculated_crc32, file_id);
        fs_delete(fs, file_id);
        return SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND;
    }

    if (SYS_CONFIG_FORMAT_VERSION != config->format_version)
    {
        DEBUG_PR_WARN("%s() configuration file is of an incompatible format version. Deleting it ID: %u", __FUNCTION__, file_id);
        fs_delete(fs, file_id);
        return SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND;
    }

    if (total_bytes_read != sizeof(sys_config) + sizeof(stored_crc32))
    {
        DEBUG_PR_WARN("%s() configuration file is of an unexpected size. Deleting it ID: %u", __FUNCTION__, file_id);
        fs_delete(fs, file_id);
        return SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND;
    }

    return SYS_CONFIG_NO_ERROR;
}

/**
 * @brief      Load the configuration data from the file system into RAM
 *
 * @param[in]  fs    The file system
 *
 * @return     SYS_CONFIG_NO_ERROR on success
 * @return     SYS_CONFIG_ERROR_FS an error occured in the file system
 * @return     SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND if no valid
 *             configuration file is found
 */
int sys_config_load_from_fs(fs_t fs)
{
    bool primary_valid, secondary_valid;
    uint32_t primary_version = 0, secondary_version = 0;
    sys_config_t loaded_config; // NOTE: This is quite large and is stored on the stack
    int ret;

    ret = get_configuration_data_from_fs(fs, FILE_ID_CONF_PRIMARY, &loaded_config);
    if (ret != SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND && ret != SYS_CONFIG_NO_ERROR)
        return SYS_CONFIG_ERROR_FS;

    primary_valid = (ret == SYS_CONFIG_NO_ERROR) ? true : false;
    if (primary_valid && loaded_config.version.hdr.set)
        primary_version = loaded_config.version.contents.version;

    ret = get_configuration_data_from_fs(fs, FILE_ID_CONF_SECONDARY, &loaded_config);
    if (ret != SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND && ret != SYS_CONFIG_NO_ERROR)
        return SYS_CONFIG_ERROR_FS;

    secondary_valid = (ret == SYS_CONFIG_NO_ERROR) ? true : false;
    if (secondary_valid && loaded_config.version.hdr.set)
        secondary_version = loaded_config.version.contents.version;

    // If there is no valid configuration file
    if (!primary_valid && !secondary_valid)
        return SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND;

    // If there are two valid configuration files (very unlikely but possible)
    if (primary_valid && secondary_valid)
    {
        // Then choose the one with the highest version number and delete the other
        if (primary_version >= secondary_version)
        {
            ret = get_configuration_data_from_fs(fs, FILE_ID_CONF_PRIMARY, &sys_config);
            if (ret)
                return SYS_CONFIG_ERROR_FS;

            ret = fs_delete(fs, FILE_ID_CONF_SECONDARY);
            if (ret)
                return SYS_CONFIG_ERROR_FS;
        }
        else
        {
            sys_config = loaded_config;
            ret = fs_delete(fs, FILE_ID_CONF_PRIMARY);
            if (ret)
                return SYS_CONFIG_ERROR_FS;
        }

        return SYS_CONFIG_NO_ERROR;
    }

    // If there is only one configuration file
    if (primary_valid)
    {
        ret = get_configuration_data_from_fs(fs, FILE_ID_CONF_PRIMARY, &sys_config);
        if (ret)
            return SYS_CONFIG_ERROR_FS;
    }
    else
    {
        sys_config = loaded_config;
    }

    return SYS_CONFIG_NO_ERROR;
}

/**
 * @brief      Save the configuration data from RAM into the file system
 *
 * @param[in]  fs    The file system
 *
 * @return     SYS_CONFIG_NO_ERROR on success
 * @return     SYS_CONFIG_ERROR_FS an error occured in the file system
 */
int sys_config_save_to_fs(fs_t fs)
{
    uint32_t primary_version = 0, secondary_version = 0;
    sys_config_t loaded_config; // NOTE: This is quite large and is stored on the stack
    bool primary_valid, secondary_valid;
    fs_handle_t file_handle;
    uint32_t total_bytes_written = 0;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    uint8_t file_id;
    int ret;

    ret = get_configuration_data_from_fs(fs, FILE_ID_CONF_PRIMARY, &loaded_config);
    primary_valid = (ret == SYS_CONFIG_NO_ERROR) ? true : false;
    if (primary_valid && loaded_config.version.hdr.set)
        primary_version = loaded_config.version.contents.version;

    ret = get_configuration_data_from_fs(fs, FILE_ID_CONF_SECONDARY, &loaded_config);
    secondary_valid = (ret == SYS_CONFIG_NO_ERROR) ? true : false;
    if (secondary_valid && loaded_config.version.hdr.set)
        secondary_version = loaded_config.version.contents.version;

    // If there are two valid configuration files (very unlikely but possible)
    if (primary_valid && secondary_valid)
    {
        // Then delete the one with the lowest version number to make room for a new one
        if (primary_version < secondary_version)
        {
            ret = fs_delete(fs, FILE_ID_CONF_PRIMARY);
            if (ret)
                return SYS_CONFIG_ERROR_FS;
            primary_valid = false;
        }
        else
        {
            ret = fs_delete(fs, FILE_ID_CONF_SECONDARY);
            if (ret)
                return SYS_CONFIG_ERROR_FS;
            secondary_valid = false;
        }
    }

    file_id = primary_valid ? FILE_ID_CONF_SECONDARY : FILE_ID_CONF_PRIMARY;

    // Ensure our configuration version flag is set
    sys_config.format_version = SYS_CONFIG_FORMAT_VERSION;

    // Write our configuration struct to the file system
    ret = fs_open(file_system, &file_handle, file_id, FS_MODE_CREATE, NULL);
    if (ret)
        return SYS_CONFIG_ERROR_FS;

    ret = fs_write(file_handle, &sys_config, sizeof(sys_config), &bytes_written);
    if (ret)
    {
        fs_close(file_handle);
        return SYS_CONFIG_ERROR_FS;
    }
    total_bytes_written += bytes_written;

    calculated_crc32 = crc32(0, &sys_config, sizeof(sys_config));

    ret = fs_write(file_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written);
    if (ret)
    {
        fs_close(file_handle);
        return SYS_CONFIG_ERROR_FS;
    }
    total_bytes_written += bytes_written;

    ret = fs_close(file_handle);
    if (ret)
        return SYS_CONFIG_ERROR_FS;

    if (total_bytes_written != sizeof(sys_config) + sizeof(calculated_crc32))
        return SYS_CONFIG_ERROR_FS;

    // Now delete our old configuration file if there is one
    if (FILE_ID_CONF_PRIMARY == file_id)
    {
        if (secondary_valid)
        {
            ret = fs_delete(fs, FILE_ID_CONF_SECONDARY);
            if (ret)
                return SYS_CONFIG_ERROR_FS;
        }
    }
    else
    {
        if (primary_valid)
        {
            ret = fs_delete(fs, FILE_ID_CONF_PRIMARY);
            if (ret)
                return SYS_CONFIG_ERROR_FS;
        }
    }

    return SYS_CONFIG_NO_ERROR;
}
