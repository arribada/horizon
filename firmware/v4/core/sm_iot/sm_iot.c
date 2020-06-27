/* sm_iot.c - IOT state machine
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
#include <stdlib.h>
#include "debug.h"
#include "cexception.h"
#include "syshal_batt.h"
#include "syshal_pmu.h"
#include "syshal_timer.h"
#include "syshal_rtc.h"
#include "syshal_firmware.h"
#include "syshal_sat.h"
#include "syshal_device.h"
#include "sm_main.h"
#include "sm_iot.h"
#include "ARTIC.h"

#define CELLULAR_CONNECT_TIMEOUT_MS (3 * 60 * 1000)
#define CELLULAR_DEFAULT_TIMEOUT_MS (30 * 1000)
#define CELLULAR_START_BACKOFF_TIME (30)

#define SATELLITE_TIMEOUT_NOT_USED (0)

#define SIZE_OF_MEMBER(struct, member) (sizeof(((struct*)0)->member))

static sm_iot_init_t config;
static bool is_init = false;

static struct
{
    bool enabled;
    bool is_pending;
    bool retry_requested;
    bool min_interval_reached;
    bool max_interval_reached;
    bool max_backoff_reached;
    uint32_t num_updates;
    uint32_t backoff_time;
    uint32_t last_successful_connection;
} cellular;

static struct
{
    bool enabled;
    bool transmit_requested;
    bool min_interval_reached;
    bool max_interval_reached;
    bool ignore_prepas;
    bool prepas_reached;
    uint32_t num_updates;
    uint32_t last_tx_timestamp;
    uint32_t next_tx_timestamp;
} satellite;

// Timer handles
static timer_handle_t timer_cellular_max_interval;
static timer_handle_t timer_cellular_min_interval;
static timer_handle_t timer_cellular_retry;
static timer_handle_t timer_satellite_min_interval;
static timer_handle_t timer_satellite_max_interval;
static timer_handle_t timer_satellite_prepas;

static void generate_event(sm_iot_event_id_t id, iot_error_report_t *iot_report_error)
{
    sm_iot_event_t event;
    iot_error_report_t iot_error_rep = {0};
    event.id = id;

    if (iot_report_error)
        event.iot_report_error = *iot_report_error;
    else
        event.iot_report_error = iot_error_rep;

    sm_iot_callback(&event);
}

static void reset_min_interval_timers(void)
{
    if (config.iot_init.iot_cellular_config->contents.min_interval)
    {
        cellular.min_interval_reached = false;
        syshal_timer_set(timer_cellular_min_interval, one_shot, config.iot_init.iot_cellular_config->contents.min_interval);
    }

    if (config.iot_init.iot_sat_config->contents.min_interval)
    {
        satellite.min_interval_reached = false;
        syshal_timer_set(timer_satellite_min_interval, one_shot, config.iot_init.iot_sat_config->contents.min_interval);
    }
}

/**
 * @brief      Populate the device status using the latest values.
 *
 * @note       This does not modify status->last_log_file_read_pos
 *
 * @param      status  The pointer to the device status struct
 */
static void populate_device_status( iot_radio_type_t radio_type, iot_device_status_t *status)
{
    int ret;
    uint32_t status_filter;

    status->presence_flags = 0;

    switch (radio_type)
    {
        case IOT_RADIO_CELLULAR:
            status_filter = config.iot_init.iot_cellular_config->contents.status_filter;
            break;
        case IOT_RADIO_SATELLITE:
            status_filter = config.iot_init.iot_sat_config->contents.status_filter;
            break;
        default:
            return;
    }

    if (status_filter & IOT_PRESENCE_FLAG_LAST_GPS_LOCATION)
    {
        if (config.gps_last_known_position && config.gps_last_known_position->hdr.set)
        {
            // Convert from date time to timestamp format
            syshal_rtc_data_and_time_t data_and_time;
            data_and_time.year =    config.gps_last_known_position->contents.year;
            data_and_time.month =   config.gps_last_known_position->contents.month;
            data_and_time.day =     config.gps_last_known_position->contents.day;
            data_and_time.hours =   config.gps_last_known_position->contents.hours;
            data_and_time.minutes = config.gps_last_known_position->contents.minutes;
            data_and_time.seconds = config.gps_last_known_position->contents.seconds;
            data_and_time.milliseconds = 0;

            ret = syshal_rtc_date_time_to_timestamp(data_and_time, &status->last_gps_location.timestamp);
            if (!ret)
            {
                status->last_gps_location.longitude = config.gps_last_known_position->contents.lon;
                status->last_gps_location.latitude =  config.gps_last_known_position->contents.lat;
                status->presence_flags |= IOT_PRESENCE_FLAG_LAST_GPS_LOCATION;
            }
        }
    }

    if (status_filter & IOT_PRESENCE_FLAG_LAST_SAT_TX_TIMESTAMP)
    {
        if (satellite.last_tx_timestamp)
        {
            status->presence_flags |= IOT_PRESENCE_FLAG_LAST_SAT_TX_TIMESTAMP;
            status->last_sat_tx_timestamp = satellite.last_tx_timestamp;
        }
    }

    if (status_filter & IOT_PRESENCE_FLAG_NEXT_SAT_TX_TIMESTAMP)
    {
        if (satellite.next_tx_timestamp)
        {
            status->presence_flags |= IOT_PRESENCE_FLAG_NEXT_SAT_TX_TIMESTAMP;
            status->next_sat_tx_timestamp = satellite.next_tx_timestamp;
        }
    }

    if (status_filter & IOT_PRESENCE_FLAG_LAST_LOG_FILE_READ_POS)
        status->presence_flags |= IOT_PRESENCE_FLAG_LAST_LOG_FILE_READ_POS;

    if (status_filter & IOT_PRESENCE_FLAG_BATTERY_LEVEL)
    {
        ret = syshal_batt_level(&status->battery_level);
        if (!ret)
            status->presence_flags |= IOT_PRESENCE_FLAG_BATTERY_LEVEL;
    }

    if (status_filter & IOT_PRESENCE_FLAG_BATTERY_VOLTAGE)
    {
        ret = syshal_batt_voltage(&status->battery_voltage);
        if (!ret)
            status->presence_flags |= IOT_PRESENCE_FLAG_BATTERY_VOLTAGE;
    }

    if (status_filter & IOT_PRESENCE_FLAG_LAST_CELLULAR_CONNECTED_TIMESTAMP &&
        cellular.last_successful_connection)
    {
        status->last_cellular_connected_timestamp = cellular.last_successful_connection;
        status->presence_flags |= IOT_PRESENCE_FLAG_LAST_CELLULAR_CONNECTED_TIMESTAMP;
    }

    // If there is a config version
    if (config.configuration_version && config.configuration_version->hdr.set)
    {
        status->configuration_version = config.configuration_version->contents.version;
        if (status_filter & IOT_PRESENCE_FLAG_CONFIGURATION_VERSION)
            status->presence_flags |= IOT_PRESENCE_FLAG_CONFIGURATION_VERSION;
    }
    else
    {
        status->configuration_version = 0;
    }

    // Fetch our current firmware version
    if (status_filter & IOT_PRESENCE_FLAG_FIRMWARE_VERSION)
        if (SYSHAL_DEVICE_NO_ERROR == syshal_device_firmware_version(&status->firmware_version))
            status->presence_flags |= IOT_PRESENCE_FLAG_FIRMWARE_VERSION;
}

static int cellular_connect(void)
{
    iot_device_shadow_t shadow;
    bool firmware_update = false;
    bool config_update = false;
    CEXCEPTION_T e = CEXCEPTION_NONE;
    iot_error_report_t iot_report_error;
    int ret;

    cellular.is_pending = false;
    cellular.retry_requested = false;

    memset(&iot_report_error, 0, sizeof(iot_report_error));
    generate_event(SM_IOT_ABOUT_TO_POWER_ON, &iot_report_error);

    Try
    {
        ret = iot_power_on(IOT_RADIO_CELLULAR);
        iot_get_error_report(&iot_report_error);
        generate_event(SM_IOT_CELLULAR_POWER_ON, &iot_report_error);
        if (ret)
            Throw(ret);

        ret = iot_connect(CELLULAR_CONNECT_TIMEOUT_MS);

        sm_iot_event_t event;
        event.id = SM_IOT_CELLULAR_NETWORK_INFO;
        event.iot_report_error.iot_error_code = IOT_NO_ERROR;
        iot_get_network_info(&(event.network_info));
        sm_iot_callback(&event);

        iot_get_error_report(&iot_report_error);
        generate_event(SM_IOT_CELLULAR_CONNECT, &iot_report_error);
        if (ret)
            Throw(ret);

        ret = iot_fetch_device_shadow(CELLULAR_DEFAULT_TIMEOUT_MS, &shadow);
        iot_get_error_report(&iot_report_error);
        generate_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, &iot_report_error);
        if (ret)
            Throw(ret);

        if (config.iot_init.iot_cellular_config->contents.log_filter) // TODO: [NGPT-370] This is the incorrect use of log_filter and needs to be remedied
        {
            ret = iot_send_logging(CELLULAR_DEFAULT_TIMEOUT_MS, config.file_system, FILE_ID_LOG, shadow.device_status.last_log_file_read_pos);
            iot_get_error_report(&iot_report_error);
            generate_event(SM_IOT_CELLULAR_SEND_LOGGING, &iot_report_error);
            if (ret >= 0)
            {
                shadow.device_status.last_log_file_read_pos = ret;
            }
            else
            {
                // [NGPT-369] Ignore errors here so that a device status is still sent
            }
        }

        // Update the remote device status
        populate_device_status(IOT_RADIO_CELLULAR, &shadow.device_status);
        ret = iot_send_device_status(CELLULAR_DEFAULT_TIMEOUT_MS, &shadow.device_status);
        iot_get_error_report(&iot_report_error);
        generate_event(SM_IOT_CELLULAR_SEND_DEVICE_STATUS, &iot_report_error);
        if (ret)
            Throw(ret);
    }
    Catch(e)
    {
        DEBUG_PR_ERROR("%s() failed with %d", __FUNCTION__, e);
        ret = iot_power_off();
        iot_get_error_report(&iot_report_error);
        generate_event(SM_IOT_CELLULAR_POWER_OFF, &iot_report_error);

        syshal_timer_cancel(timer_cellular_max_interval);

        if (cellular.backoff_time >= config.iot_init.iot_cellular_config->contents.max_backoff_interval)
        {
            cellular.backoff_time = config.iot_init.iot_cellular_config->contents.max_backoff_interval;
            cellular.max_backoff_reached = true;
            generate_event(SM_IOT_CELLULAR_MAX_BACKOFF_REACHED, NULL);
        }

        syshal_timer_set(timer_cellular_retry, one_shot, cellular.backoff_time);

        if (!cellular.max_backoff_reached)
            cellular.backoff_time *= 2; // Increase our backoff timer exponentially until we get a successful connection

        return SM_IOT_ERROR_CONNECTION_FAILED;
    }

    // If we are supposed to be checking for firmware updates
    if (config.iot_init.iot_cellular_config->contents.check_firmware_updates)
    {
        DEBUG_PR_TRACE("Checking for firmware updates...");
        // And there is a firmware update
        if (shadow.device_update.presence_flags & IOT_DEVICE_UPDATE_PRESENCE_FLAG_FIRMWARE_UPDATE)
        {
            // And it is of a greater version than we have
            uint32_t current_version;
            syshal_device_firmware_version(&current_version);
            if (shadow.device_update.firmware_update.version > current_version)
            {
                sm_iot_event_t event;

                // Then download it
                DEBUG_PR_TRACE("Downloading firmware update, version: %lu, from: %s:%u/%s",
                               shadow.device_update.firmware_update.version,
                               shadow.device_update.firmware_update.url.domain,
                               shadow.device_update.firmware_update.url.port,
                               shadow.device_update.firmware_update.url.path);

                ret = iot_download_file(CELLULAR_DEFAULT_TIMEOUT_MS, &shadow.device_update.firmware_update.url, config.file_system, FILE_ID_APP_FIRM_IMAGE, &event.firmware_update.length);
                if (ret)
                {
                    DEBUG_PR_ERROR("Download failed with %d", ret);
                }
                else
                {
                    firmware_update = true;
                }

                event.id = SM_IOT_CELLULAR_DOWNLOAD_FIRMWARE_FILE;
                iot_get_error_report(&event.iot_report_error);
                event.firmware_update.version = shadow.device_update.firmware_update.version;
                sm_iot_callback(&event);
            }
            else
            {
                DEBUG_PR_TRACE("Firmware update found but a lower or same version as current (v%lu vs v%lu)", current_version, shadow.device_update.firmware_update.version);
            }
        }
        else
        {
            DEBUG_PR_TRACE("No firmware update found");
        }
    }

    // If we are supposed to be checking for configuration updates
    if (config.iot_init.iot_cellular_config->contents.check_configuration_updates)
    {
        DEBUG_PR_TRACE("Checking for configuration updates...");
        // And there is a config update
        if (shadow.device_update.presence_flags & IOT_DEVICE_UPDATE_PRESENCE_FLAG_CONFIGURATION_UPDATE)
        {
            // And it is of a greater version than we have
            if (shadow.device_update.configuration_update.version > shadow.device_status.configuration_version)
            {
                sm_iot_event_t event;

                // Then download it
                DEBUG_PR_TRACE("Downloading configuration update, version: %lu, from: %s:%u/%s",
                               shadow.device_update.configuration_update.version,
                               shadow.device_update.configuration_update.url.domain,
                               shadow.device_update.configuration_update.url.port,
                               shadow.device_update.configuration_update.url.path);

                ret = iot_download_file(CELLULAR_DEFAULT_TIMEOUT_MS, &shadow.device_update.configuration_update.url, config.file_system, FILE_ID_CONF_COMMANDS, &event.config_update.length);
                if (ret)
                {
                    DEBUG_PR_ERROR("Download failed with %d", ret);
                }
                else
                {
                    config_update = true;
                }

                event.id = SM_IOT_CELLULAR_DOWNLOAD_CONFIG_FILE;
                iot_get_error_report(&event.iot_report_error);
                event.config_update.version = shadow.device_update.configuration_update.version;
                sm_iot_callback(&event);
            }
            else
            {
                DEBUG_PR_TRACE("Configuration update found but a lower or same version as current (v%lu vs v%lu)", shadow.device_status.configuration_version, shadow.device_update.configuration_update.version);
            }
        }
        else
        {
            DEBUG_PR_TRACE("No configuration update found");
        }
    }

    ret = iot_power_off();
    iot_get_error_report(&iot_report_error);
    generate_event(SM_IOT_CELLULAR_POWER_OFF, &iot_report_error);

    // Transmission was a success, update our internal states and timers //
    reset_min_interval_timers();

    syshal_rtc_get_timestamp(&cellular.last_successful_connection);

    // If there is a maximum interval then start its timer
    if (config.iot_init.iot_cellular_config->contents.max_interval)
    {
        cellular.max_interval_reached = false;
        DEBUG_PR_TRACE("Set Max interval to false");
        syshal_timer_set(timer_cellular_max_interval, one_shot, config.iot_init.iot_cellular_config->contents.max_interval);
    }

    cellular.is_pending = false;

    cellular.backoff_time = CELLULAR_START_BACKOFF_TIME;
    cellular.max_backoff_reached = false;

    cellular.num_updates = 0;

    // Apply any firmware update
    if (firmware_update)
    {
        generate_event(SM_IOT_APPLY_FIRMWARE_UPDATE, NULL);
        syshal_firmware_update(FILE_ID_APP_FIRM_IMAGE, shadow.device_update.firmware_update.version);
    }

    // Reset as the config update is handled on startup
    if (config_update)
    {
        generate_event(SM_IOT_APPLY_CONFIG_UPDATE, NULL);
        syshal_pmu_reset();
    }

    return SM_IOT_NO_ERROR;
}

static int satellite_connect(void)
{
    iot_device_shadow_t shadow;
    CEXCEPTION_T e = CEXCEPTION_NONE;
    iot_error_report_t iot_report_error;
    int ret;

    memset(&iot_report_error, 0, sizeof(iot_report_error));
    generate_event(SM_IOT_ABOUT_TO_POWER_ON, &iot_report_error);

    // If we are not using prepas then we can not rely on sm_iot_new_position()
    // to update the next/last_tx_timestamp variable. Instead we'll do it here
    if (satellite.ignore_prepas)
    {
        syshal_rtc_get_timestamp(&satellite.last_tx_timestamp);
        satellite.next_tx_timestamp = 0; // In this state next_tx will always be zero
    }

    Try
    {
        ret = iot_power_on(IOT_RADIO_SATELLITE);
        iot_get_error_report(&iot_report_error);
        generate_event(SM_IOT_SATELLITE_POWER_ON, &iot_report_error);
        if (ret)
            Throw(ret);

        if (config.iot_init.iot_sat_config->contents.test_mode_enable)
        {
            const uint8_t test_data[15] = {0xC1, 0x47, 0xCA, 0x6B, 0x48, 0x17, 0xC7, 0x65, 0xDC, 0x8A, 0x2A, 0x9D, 0xA1, 0xE2, 0x18};
            ret = syshal_sat_send_message(test_data, sizeof(test_data));
            if (ret)
                Throw(ret);
        }
        else
        {
            populate_device_status(IOT_RADIO_SATELLITE, &shadow.device_status);

            // Transmit the device status
            ret = iot_send_device_status(SATELLITE_TIMEOUT_NOT_USED, &shadow.device_status);
            iot_get_error_report(&iot_report_error);
            generate_event(SM_IOT_SATELLITE_SEND_DEVICE_STATUS, &iot_report_error);
            if (ret)
                Throw(ret);
        }
        
    }
    Catch(e)
    {
        DEBUG_PR_ERROR("%s() failed with %d", __FUNCTION__, e);

        ret = iot_power_off();
        iot_get_error_report(&iot_report_error);
        generate_event(SM_IOT_SATELLITE_POWER_OFF, &iot_report_error);

        if (config.iot_init.iot_sat_config->contents.max_interval)
        {
            satellite.max_interval_reached = false;
            syshal_timer_set(timer_satellite_max_interval, one_shot, config.iot_init.iot_sat_config->contents.max_interval);
        }

        return SM_IOT_ERROR_CONNECTION_FAILED;
    }

    ret = iot_power_off();
    iot_get_error_report(&iot_report_error);
    generate_event(SM_IOT_SATELLITE_POWER_OFF, &iot_report_error);

    // Transmission was a success, update our internal states and timers //

    if (!satellite.ignore_prepas)
        satellite.last_tx_timestamp = satellite.next_tx_timestamp;

    reset_min_interval_timers();

    satellite.num_updates = 0;

    // If there is a maximum interval then start its timer
    if (config.iot_init.iot_sat_config->contents.max_interval)
    {
        satellite.max_interval_reached = false;
        syshal_timer_set(timer_satellite_max_interval, one_shot, config.iot_init.iot_sat_config->contents.max_interval);
    }

    return SM_IOT_NO_ERROR;
}

static void cellular_min_interval_callback(void)
{
    cellular.min_interval_reached = true;
}

static void cellular_max_interval_callback(void)
{
    cellular.max_interval_reached = true;
}

static void cellular_retry_callback(void)
{
    cellular.retry_requested = true;
}

static void satellite_min_interval_callback(void)
{
    DEBUG_PR_WARN("satellite_min_interval_callback");
    satellite.min_interval_reached = true;
}

static void satellite_prepas_callback(void)
{
    satellite.prepas_reached = true;

    // Check and see if we meet the requirements to transmit now
    sm_iot_tick();

    // Our window is only considered valid in the now
    satellite.prepas_reached = false;
}

static void satellite_max_interval_callback(void)
{
    DEBUG_PR_WARN("satellite_max_interval_callback");
    satellite.max_interval_reached = true;
}

int sm_iot_init(sm_iot_init_t init)
{
    // Create a local copy of the init params
    config = init;

    // Back out if the IOT layer is not enabled
    if (!config.iot_init.iot_config->contents.enable || !config.iot_init.iot_config->hdr.set)
        return SM_IOT_NO_ERROR;

    // Setup our internal state variables
    memset(&cellular, 0, sizeof(cellular));
    memset(&satellite, 0, sizeof(satellite));

    cellular.enabled = config.iot_init.iot_cellular_config->contents.enable &&
                       config.iot_init.iot_cellular_config->hdr.set &&
                       config.iot_init.iot_cellular_aws_config->hdr.set;
    cellular.backoff_time = CELLULAR_START_BACKOFF_TIME;

    satellite.enabled = config.iot_init.iot_sat_config->contents.enable &&
                        config.iot_init.iot_sat_config->hdr.set &&
                        config.iot_init.iot_sat_artic_config->hdr.set;

    // Back out if no IOT backend is enabled
    if (!cellular.enabled && !satellite.enabled)
        return SM_IOT_NO_ERROR;

    // Initialise the IOT subsystem
    if (iot_init(config.iot_init))
        return SM_IOT_ERROR_INIT;

    // Init timers
    syshal_timer_init(&timer_cellular_min_interval, cellular_min_interval_callback);
    syshal_timer_init(&timer_cellular_max_interval, cellular_max_interval_callback);
    syshal_timer_init(&timer_cellular_retry, cellular_retry_callback);
    syshal_timer_init(&timer_satellite_min_interval, satellite_min_interval_callback);
    syshal_timer_init(&timer_satellite_max_interval, satellite_max_interval_callback);
    syshal_timer_init(&timer_satellite_prepas, satellite_prepas_callback);

    // Seed the random generator used for transmission collision avoidance
    srand(config.iot_init.iot_sat_artic_config->contents.device_identifier);

    if (satellite.enabled)
    {
        // If our given satellite data is all zeros then we should ignore pass predictions
        satellite.ignore_prepas = true;
        for (uint32_t i = 0; i < SIZE_OF_MEMBER(sys_config_iot_sat_artic_settings_t, contents.bulletin_data) / sizeof(bulletin_data_t); ++i)
        {
            if (config.iot_init.iot_sat_artic_config->contents.bulletin_data[i].sat[0] ||
                config.iot_init.iot_sat_artic_config->contents.bulletin_data[i].sat[1])
            {
                satellite.ignore_prepas = false;
                break;
            }
        }
    }

    // Set up cellular timers
    if (cellular.enabled && config.iot_init.iot_cellular_config->contents.max_interval)
        syshal_timer_set(timer_cellular_max_interval, one_shot, config.iot_init.iot_cellular_config->contents.max_interval);

    if (cellular.enabled && config.iot_init.iot_cellular_config->contents.min_interval)
        syshal_timer_set(timer_cellular_min_interval, one_shot, config.iot_init.iot_cellular_config->contents.min_interval);
    else
        cellular.min_interval_reached = true;

    // Set up satellite timers
    if (satellite.enabled && config.iot_init.iot_sat_config->contents.max_interval)
        syshal_timer_set(timer_satellite_max_interval, one_shot, config.iot_init.iot_sat_config->contents.max_interval);

    if (satellite.enabled && config.iot_init.iot_sat_config->contents.min_interval)
        syshal_timer_set(timer_satellite_min_interval, one_shot, config.iot_init.iot_sat_config->contents.min_interval);
    else
        satellite.min_interval_reached = true;

    is_init = true;

    return SM_IOT_NO_ERROR;
}

int sm_iot_term(void)
{
    cellular.enabled = false;
    satellite.enabled = false;

    if (is_init)
    {
        syshal_timer_term(timer_cellular_min_interval);
        syshal_timer_term(timer_cellular_max_interval);
        syshal_timer_term(timer_cellular_retry);
        syshal_timer_term(timer_satellite_min_interval);
        syshal_timer_term(timer_satellite_max_interval);
        syshal_timer_term(timer_satellite_prepas);
    }

    is_init = false;

    return SM_IOT_NO_ERROR;
}

int sm_iot_trigger_force(iot_radio_type_t radio_type)
{
    switch (radio_type)
    {
        case IOT_RADIO_CELLULAR:
            return cellular_connect();
            break;

        case IOT_RADIO_SATELLITE:
            return satellite_connect();
            break;

        default:
            return SM_IOT_ERROR_INVALID_PARAM;
            break;
    }
    return SM_IOT_NO_ERROR;
}

static bool cellular_criteria_met(void)
{
    if (!cellular.enabled)
        return false;

    if (!cellular.min_interval_reached)
        return false;

    if (cellular.retry_requested)
        return true;

    if (cellular.is_pending)
        return false;

    // If a retry attempt is already pending then wait until that has fired before proceeding
    if (SYSHAL_TIMER_RUNNING == syshal_timer_running(timer_cellular_retry))
        return false;

    if (config.iot_init.iot_cellular_config->contents.min_updates &&
        cellular.num_updates >= config.iot_init.iot_cellular_config->contents.min_updates)
        return true;

    if (cellular.max_interval_reached)
        return true;

    return false;
}

static bool satellite_criteria_met(void)
{
    if (!satellite.enabled)
        return false;

    if (!satellite.min_interval_reached)
        return false;

    if (!satellite.ignore_prepas && !satellite.prepas_reached)
        return false;

    if (config.iot_init.iot_sat_config->contents.min_updates &&
        satellite.num_updates >= config.iot_init.iot_sat_config->contents.min_updates)
        return true;

    if (satellite.max_interval_reached)
        return true;

    return false;
}

int sm_iot_tick(void)
{
    bool cell_ready = cellular_criteria_met();
    bool sat_ready = satellite_criteria_met();

    if (cell_ready && sat_ready)
    {
        // [NGPT-371] If both devices are ready handle the one with the higher priority
        if (config.iot_init.iot_cellular_config->contents.connection_priority >= config.iot_init.iot_sat_config->contents.connection_priority)
            cellular_connect();
        else
            satellite_connect();
    }
    else if (cell_ready)
    {
        cellular_connect();
    }
    else if (sat_ready)
    {
        satellite_connect();
    }

    return SM_IOT_NO_ERROR;
}

// Returns a uniformly distributed random value between the two given ranges
static uint32_t random_range(uint32_t lower_value, uint32_t upper_value)
{
    if (lower_value >= upper_value)
        return lower_value;

    uint32_t range = upper_value - lower_value + 1;

    // Largest value that when multiplied by "range"
    // is less than or equal to RAND_MAX
    int chunkSize = ( (int64_t) RAND_MAX + 1) / range;
    int endOfLastChunk = chunkSize * range;

    int random_val = rand();
    while (random_val >= endOfLastChunk)
        random_val = rand();

    return lower_value + (random_val / chunkSize);
}

int sm_iot_new_position(sm_iot_timestamped_position_t timestamped_pos)
{
    iot_prepass_result_t next_transmission;
    iot_last_gps_location_t gps;
    uint32_t relative_time_until_tx;

    if (cellular.enabled)
        cellular.num_updates++;

    if (satellite.enabled)
        satellite.num_updates++;
    else
        return SM_IOT_NO_ERROR;

    if (satellite.ignore_prepas)
        return SM_IOT_NO_ERROR;

    // [NGPT-427] Don't recalculate our satellite pass prediction if we already have one
    if (SYSHAL_TIMER_RUNNING == syshal_timer_running(timer_satellite_prepas))
        return SM_IOT_NO_ERROR;

    DEBUG_PR_TRACE("Predicting satellite window...");

    gps.longitude = timestamped_pos.longitude;
    gps.latitude  = timestamped_pos.latitude;
    gps.timestamp = timestamped_pos.timestamp;

    if (iot_calc_prepass(&gps, &next_transmission))
        return SM_IOT_ERROR_PREPAS_CALC_FAILED;

    // [NGPT-372] Should we be randomising our transmission time to avoid possible collisions with neighbours?
    if (config.iot_init.iot_sat_config->contents.randomised_tx_window)
    {
        uint8_t half_window = config.iot_init.iot_sat_config->contents.randomised_tx_window / 2;

        // Enforce a minimum level of randomisation if a tx window is given
        if (half_window == 0)
            half_window = 1;

        if (next_transmission >= half_window)
            next_transmission = random_range(next_transmission - half_window, next_transmission + half_window);
    }

    // Work out the time delta from now until the prepas window
    if (next_transmission > timestamped_pos.timestamp + SYSHAL_SAT_TURN_ON_TIME_S)
        relative_time_until_tx = next_transmission - timestamped_pos.timestamp - SYSHAL_SAT_TURN_ON_TIME_S;
    else
        relative_time_until_tx = 0; // Do not permit negative deltas

    satellite.next_tx_timestamp = timestamped_pos.timestamp + relative_time_until_tx;

    DEBUG_PR_TRACE("Satellite window predicted at %lu, waking at %lu", next_transmission, satellite.next_tx_timestamp);
    sm_iot_event_t event;
    iot_error_report_t iot_error_rep = {0};
    event.iot_report_error = iot_error_rep;

    event.id = SM_IOT_NEXT_PREPAS;
    event.next_prepas.gps_timestamp = gps.timestamp;
    event.next_prepas.prepas_result = next_transmission;
    sm_iot_callback(&event);

    syshal_timer_set(timer_satellite_prepas, one_shot, relative_time_until_tx);

    return SM_IOT_NO_ERROR;
}

__attribute__((weak)) void sm_iot_callback(sm_iot_event_t *event)
{
    ((void)(event)); // Remove unused variable compiler warning

    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);
#ifdef GTEST
    printf("sm_iot_callback event: %d\n", event->id);
#endif
}