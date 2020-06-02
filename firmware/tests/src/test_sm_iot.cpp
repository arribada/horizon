// test_sm_iot.cpp - IOT subsystem unit tests
//
// Copyright (C) 2019 Arribada
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

extern "C"
{
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "unity.h"
#include "sm_iot.h"
#include "Mockiot.h"
#include "Mocksyshal_device.h"
#include "Mocksyshal_rtc.h"
#include "Mocksyshal_pmu.h"
#include "Mocksyshal_firmware.h"
#include "Mocksyshal_sat.h"
#include "syshal_timer.h"
#include "syshal_sat.h"
#include "sm_main.h"
}

#include <gtest/gtest.h>
#include <time.h>
#include <math.h>
#include <queue>

fs_t file_system;

static sys_config_iot_general_settings_t        iot_general_settings;
static sys_config_iot_cellular_settings_t       iot_cellular_settings;
static sys_config_iot_cellular_aws_settings_t   iot_cellular_aws_settings;
static sys_config_iot_cellular_apn_t            iot_cellular_apn;
static sys_config_iot_sat_settings_t            iot_sat_settings;
static sys_config_iot_sat_artic_settings_t      iot_sat_artic_settings;
static sys_config_system_device_identifier_t    system_device_identifier;
static sys_config_gps_last_known_position_t     gps_last_known_position;
static sys_config_version_t                     config_version;

static sm_iot_init_t config;

std::queue<sm_iot_event_t> occured_events;
void sm_iot_callback(sm_iot_event_t *event)
{
    occured_events.push(*event);
    printf("Event: %d\n", event->id);
}

void iot_get_error_report_GTest(iot_error_report_t *report_error, int cmock_num_calls)
{

}

time_t current_time;
uint32_t uptime;
int syshal_rtc_get_date_and_time_GTest(syshal_rtc_data_and_time_t * date_time, int cmock_num_calls)
{
    struct tm * timeinfo = localtime(&current_time);

    date_time->milliseconds = 0;
    date_time->seconds = timeinfo->tm_sec; // seconds of minutes from 0 to 61
    date_time->minutes = timeinfo->tm_min; // minutes of hour from 0 to 59
    date_time->hours = timeinfo->tm_hour;  // hours of day from 0 to 24
    date_time->day = timeinfo->tm_mday;    // day of month from 1 to 31
    date_time->month = timeinfo->tm_mon;   // month of year from 0 to 11
    date_time->year = timeinfo->tm_year;   // year since 1900

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_timestamp_GTest(uint32_t * timestamp, int cmock_num_calls)
{
    *timestamp = current_time;
    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_uptime_GTest(uint32_t * timestamp, int cmock_num_calls)
{
    *timestamp = uptime;
    return SYSHAL_RTC_NO_ERROR;
}

class SmIotTest : public ::testing::Test
{

    virtual void SetUp()
    {
        Mockiot_Init();
        Mocksyshal_device_Init();
        Mocksyshal_rtc_Init();
        Mocksyshal_pmu_Init();
        Mocksyshal_firmware_Init();
        Mocksyshal_sat_Init();

        iot_get_error_report_StubWithCallback(iot_get_error_report_GTest);

        syshal_rtc_get_date_and_time_StubWithCallback(syshal_rtc_get_date_and_time_GTest);
        syshal_rtc_get_timestamp_StubWithCallback(syshal_rtc_get_timestamp_GTest);
        syshal_rtc_set_alarm_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);
        syshal_rtc_get_uptime_StubWithCallback(syshal_rtc_get_uptime_GTest);

        srand(time(NULL));

        current_time = time(NULL);
        uptime = 0;

        // Blank any configuration
        memset(&iot_general_settings, 0, sizeof(iot_general_settings));
        memset(&iot_cellular_settings, 0, sizeof(iot_cellular_settings));
        memset(&iot_cellular_aws_settings, 0, sizeof(iot_cellular_aws_settings));
        memset(&iot_cellular_apn, 0, sizeof(iot_cellular_apn));
        memset(&iot_sat_settings, 0, sizeof(iot_sat_settings));
        memset(&iot_sat_artic_settings, 0, sizeof(iot_sat_artic_settings));
        memset(&system_device_identifier, 0, sizeof(system_device_identifier));
        memset(&gps_last_known_position, 0, sizeof(gps_last_known_position));
        memset(&config_version, 0, sizeof(config_version));

        // Clear the event queue
        while (!occured_events.empty()) occured_events.pop();

        // Will be nice when C++20 comes around and I can use designated/tagged initialization
        config.file_system = file_system;
        config.iot_init.iot_config = &iot_general_settings;
        config.iot_init.iot_cellular_config = &iot_cellular_settings;
        config.iot_init.iot_cellular_aws_config = &iot_cellular_aws_settings;
        config.iot_init.iot_cellular_apn = &iot_cellular_apn;
        config.iot_init.iot_sat_config = &iot_sat_settings;
        config.iot_init.iot_sat_artic_config = &iot_sat_artic_settings;
        config.iot_init.system_device_identifier = &system_device_identifier;
        config.gps_last_known_position = &gps_last_known_position;
        config.configuration_version = &config_version;
    }

    virtual void TearDown()
    {
        EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());

        Mockiot_Verify();
        Mockiot_Destroy();
        Mocksyshal_device_Verify();
        Mocksyshal_device_Destroy();
        Mocksyshal_rtc_Verify();
        Mocksyshal_rtc_Destroy();
        Mocksyshal_pmu_Verify();
        Mocksyshal_pmu_Destroy();
        Mocksyshal_firmware_Verify();
        Mocksyshal_firmware_Destroy();
        Mocksyshal_sat_Verify();
        Mocksyshal_sat_Destroy();
    }

public:

    void compare_expected_events_to_actual(std::queue<sm_iot_event_id_t> expected_event_ids, int line_num)
    {
        EXPECT_EQ(expected_event_ids.size(), occured_events.size()) << "Check called from line: " << line_num;

        auto num_steps = std::min(expected_event_ids.size(), occured_events.size());

        for (auto i = 0; i < num_steps; ++i)
        {
            EXPECT_EQ(expected_event_ids.front(), occured_events.front().id) << "Check called from line: " << line_num;
            expected_event_ids.pop();
            occured_events.pop();
        }
    }

    void increment_time(unsigned int seconds)
    {
        for (auto i = 0; i < seconds; ++i)
        {
            current_time += 1;
            uptime += 1;
            syshal_timer_tick();
            EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());
        }
    }

    std::queue<sm_iot_event_id_t> cellular_expected_events_transmission_without_logging()
    {
        std::queue<sm_iot_event_id_t> expected_event_ids;
        expected_event_ids.push(SM_IOT_ABOUT_TO_POWER_ON);
        expected_event_ids.push(SM_IOT_CELLULAR_POWER_ON);
        expected_event_ids.push(SM_IOT_CELLULAR_NETWORK_INFO);
        expected_event_ids.push(SM_IOT_CELLULAR_CONNECT);
        expected_event_ids.push(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW);
        expected_event_ids.push(SM_IOT_CELLULAR_SEND_DEVICE_STATUS);
        expected_event_ids.push(SM_IOT_CELLULAR_POWER_OFF);
        return expected_event_ids;
    }

    std::queue<sm_iot_event_id_t> cellular_expected_events_transmission_with_logging()
    {
        std::queue<sm_iot_event_id_t> expected_event_ids;
        expected_event_ids.push(SM_IOT_ABOUT_TO_POWER_ON);
        expected_event_ids.push(SM_IOT_CELLULAR_POWER_ON);
        expected_event_ids.push(SM_IOT_CELLULAR_NETWORK_INFO);
        expected_event_ids.push(SM_IOT_CELLULAR_CONNECT);
        expected_event_ids.push(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW);
        expected_event_ids.push(SM_IOT_CELLULAR_SEND_LOGGING);
        expected_event_ids.push(SM_IOT_CELLULAR_SEND_DEVICE_STATUS);
        expected_event_ids.push(SM_IOT_CELLULAR_POWER_OFF);
        return expected_event_ids;
    }

    std::queue<sm_iot_event_id_t> satellite_expected_events()
    {
        std::queue<sm_iot_event_id_t> expected_event_ids;
        expected_event_ids.push(SM_IOT_ABOUT_TO_POWER_ON);
        expected_event_ids.push(SM_IOT_SATELLITE_POWER_ON);
        expected_event_ids.push(SM_IOT_SATELLITE_SEND_DEVICE_STATUS);
        expected_event_ids.push(SM_IOT_SATELLITE_POWER_OFF);
        return expected_event_ids;
    }

    void update_position(uint32_t prepas_prediction_time_from_now, bool expect_prepas = false)
    {
        sm_iot_timestamped_position_t timestamped_pos;
        timestamped_pos.longitude = rand(); // We don't really care what the position is for these tests
        timestamped_pos.latitude = rand();
        timestamped_pos.timestamp = current_time;

        if (expect_prepas)
        {
            iot_prepass_result_t result = prepas_prediction_time_from_now + current_time;

            iot_calc_prepass_ExpectAndReturn(nullptr, &result, IOT_NO_ERROR);
            iot_calc_prepass_ReturnThruPtr_result(&result);
            iot_calc_prepass_IgnoreArg_result();
            iot_calc_prepass_IgnoreArg_gps(); // TODO: Check the GPS arguments
        }

        EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_new_position(timestamped_pos));

        bool bulletin_data_exists = false;
        for (auto i = 0; i < 8; ++i)
        {
            if (iot_sat_artic_settings.contents.bulletin_data[i].sat[0] || iot_sat_artic_settings.contents.bulletin_data[i].sat[1])
            {
                bulletin_data_exists = true;
                break;
            }
        }

        if (bulletin_data_exists)
        {
            std::queue<sm_iot_event_id_t> event;
            event.push(SM_IOT_NEXT_PREPAS);
            compare_expected_events_to_actual(event, __LINE__);
        }
    }
};

TEST_F(SmIotTest, simpleInit)
{
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());
}

/////////////////////////////////////////////////////////
//////////////////// Cellular Tests /////////////////////
/////////////////////////////////////////////////////////

TEST_F(SmIotTest, cellularForceTrigger)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_get_network_info_ExpectAnyArgs();

    iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);

    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);

    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    auto expected_events = cellular_expected_events_transmission_without_logging();
    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, cellularForceTriggerWithLogging)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.log_filter = 1;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_get_network_info_ExpectAnyArgs();

    iot_device_shadow_t shadow;
    shadow.device_status.last_log_file_read_pos = rand();

    iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_fetch_device_shadow_ReturnThruPtr_shadow(&shadow);

    iot_send_logging_ExpectAndReturn(0, file_system, FILE_ID_LOG, shadow.device_status.last_log_file_read_pos, IOT_NO_ERROR);
    iot_send_logging_IgnoreArg_timeout_ms();

    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);

    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    auto expected_events = cellular_expected_events_transmission_with_logging();
    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, cellularNoMinInterval)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.min_updates = 1;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_get_network_info_ExpectAnyArgs();
    iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    update_position(0);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());

    auto expected_events = cellular_expected_events_transmission_without_logging();
    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, cellularOneFailedConnection)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.min_updates = 1;
    iot_cellular_settings.contents.max_backoff_interval = 60;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_ERROR_ATTACH_FAIL);
    iot_get_network_info_ExpectAnyArgs();
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    update_position(0);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());

    std::queue<sm_iot_event_id_t> expected_events;
    expected_events.push(SM_IOT_ABOUT_TO_POWER_ON);
    expected_events.push(SM_IOT_CELLULAR_POWER_ON);
    expected_events.push(SM_IOT_CELLULAR_NETWORK_INFO);
    expected_events.push(SM_IOT_CELLULAR_CONNECT);
    expected_events.push(SM_IOT_CELLULAR_POWER_OFF);

    compare_expected_events_to_actual(expected_events, __LINE__);

    // Move to one second before the retry attempt
    increment_time(29);
    EXPECT_EQ(0, occured_events.size());

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_get_network_info_ExpectAnyArgs();
    iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    increment_time(1);

    auto successful_events = cellular_expected_events_transmission_without_logging();
    compare_expected_events_to_actual(successful_events, __LINE__);

    increment_time(1);
    EXPECT_EQ(0, occured_events.size());
}

TEST_F(SmIotTest, cellularMaximumBackoffReached)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.min_updates = 1;
    iot_cellular_settings.contents.max_backoff_interval = 120;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    std::queue<sm_iot_event_id_t> failed_events;
    failed_events.push(SM_IOT_ABOUT_TO_POWER_ON);
    failed_events.push(SM_IOT_CELLULAR_POWER_ON);
    failed_events.push(SM_IOT_CELLULAR_NETWORK_INFO);
    failed_events.push(SM_IOT_CELLULAR_CONNECT);
    failed_events.push(SM_IOT_CELLULAR_POWER_OFF);

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_ERROR_ATTACH_FAIL);
    iot_get_network_info_ExpectAnyArgs();
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    update_position(0);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());

    compare_expected_events_to_actual(failed_events, __LINE__);

    // Move to one second before the retry attempt
    increment_time(29);
    EXPECT_EQ(0, occured_events.size());

    // Trigger the retry attempt
    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_ERROR_ATTACH_FAIL);
    iot_get_network_info_ExpectAnyArgs();
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    increment_time(1);

    compare_expected_events_to_actual(failed_events, __LINE__);

    // Move to one second before the retry attempt
    increment_time(59);
    EXPECT_EQ(0, occured_events.size());

    // Trigger the retry attempt
    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_ERROR_ATTACH_FAIL);
    iot_get_network_info_ExpectAnyArgs();
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    increment_time(1);

    std::queue<sm_iot_event_id_t> backoff_reached_events;
    backoff_reached_events.push(SM_IOT_ABOUT_TO_POWER_ON);
    backoff_reached_events.push(SM_IOT_CELLULAR_POWER_ON);
    backoff_reached_events.push(SM_IOT_CELLULAR_NETWORK_INFO);
    backoff_reached_events.push(SM_IOT_CELLULAR_CONNECT);
    backoff_reached_events.push(SM_IOT_CELLULAR_POWER_OFF);
    backoff_reached_events.push(SM_IOT_CELLULAR_MAX_BACKOFF_REACHED);

    compare_expected_events_to_actual(backoff_reached_events, __LINE__);
}

TEST_F(SmIotTest, cellularMinInterval)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.min_interval = (rand() % 10) + 5;
    iot_cellular_settings.contents.min_updates = 1;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    update_position(0);

    // Get to 1 second before the minimum interval
    increment_time(iot_cellular_settings.contents.min_interval - 1);
    EXPECT_EQ(0, occured_events.size());

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_get_network_info_ExpectAnyArgs();
    iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    // Step past our minimum interval threshold
    increment_time(1);

    auto expected_events = cellular_expected_events_transmission_without_logging();
    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, cellularMaxInterval)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.max_interval = (rand() % 10) + 5;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    for (unsigned int i = 0; i < 5; ++i)
    {
        // Get to 1 second before the maximum interval
        increment_time(iot_cellular_settings.contents.max_interval - 1);
        EXPECT_EQ(0, occured_events.size());

        iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
        iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
        iot_get_network_info_ExpectAnyArgs();
        iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
        iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
        iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

        // Step past our maximum interval threshold
        increment_time(1);

        auto expected_events = cellular_expected_events_transmission_without_logging();
        compare_expected_events_to_actual(expected_events, __LINE__);
    }
}

TEST_F(SmIotTest, cellularConfigUpdate)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.min_updates = 1;
    iot_cellular_settings.contents.check_configuration_updates = true;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_device_shadow_t shadow;

    shadow.device_update.presence_flags = IOT_DEVICE_UPDATE_PRESENCE_FLAG_CONFIGURATION_UPDATE;
    shadow.device_update.configuration_update.version = 10;
    strncpy(shadow.device_update.configuration_update.url.domain, "ThisIsATestDomain", sizeof(shadow.device_update.configuration_update.url.domain));
    strncpy(shadow.device_update.configuration_update.url.path, "/path/to/config/update", sizeof(shadow.device_update.configuration_update.url.path));
    shadow.device_update.configuration_update.url.port = 8080;

    uint32_t file_size;

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_get_network_info_ExpectAnyArgs();
    iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_fetch_device_shadow_ReturnThruPtr_shadow(&shadow);
    iot_download_file_ExpectAndReturn(0, &shadow.device_update.configuration_update.url, file_system, FILE_ID_CONF_COMMANDS, &file_size, IOT_NO_ERROR);
    iot_download_file_IgnoreArg_timeout_ms();
    iot_download_file_IgnoreArg_file_size();
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);
    syshal_pmu_reset_Expect();

    update_position(0);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());

    std::queue<sm_iot_event_id_t> expected_events;
    expected_events.push(SM_IOT_ABOUT_TO_POWER_ON);
    expected_events.push(SM_IOT_CELLULAR_POWER_ON);
    expected_events.push(SM_IOT_CELLULAR_NETWORK_INFO);
    expected_events.push(SM_IOT_CELLULAR_CONNECT);
    expected_events.push(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW);
    expected_events.push(SM_IOT_CELLULAR_SEND_DEVICE_STATUS);
    expected_events.push(SM_IOT_CELLULAR_DOWNLOAD_CONFIG_FILE);
    expected_events.push(SM_IOT_CELLULAR_POWER_OFF);
    expected_events.push(SM_IOT_APPLY_CONFIG_UPDATE);

    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, cellularConfigUpdateCurrentVersion)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.min_updates = 1;
    iot_cellular_settings.contents.check_configuration_updates = true;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_device_shadow_t shadow;

    shadow.device_update.presence_flags = IOT_DEVICE_UPDATE_PRESENCE_FLAG_CONFIGURATION_UPDATE;
    shadow.device_update.configuration_update.version = 10;
    strncpy(shadow.device_update.configuration_update.url.domain, "ThisIsATestDomain", sizeof(shadow.device_update.configuration_update.url.domain));
    strncpy(shadow.device_update.configuration_update.url.path, "/path/to/config/update", sizeof(shadow.device_update.configuration_update.url.path));
    shadow.device_update.configuration_update.url.port = 8080;

    config.configuration_version->hdr.set = true;
    config.configuration_version->contents.version = shadow.device_update.configuration_update.version;

    uint32_t file_size;

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_get_network_info_ExpectAnyArgs();
    iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_fetch_device_shadow_ReturnThruPtr_shadow(&shadow);
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    update_position(0);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());

    auto expected_events = cellular_expected_events_transmission_without_logging();
    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, cellularFirmwareUpdate)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.min_updates = 1;
    iot_cellular_settings.contents.check_firmware_updates = true;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_device_shadow_t shadow;
    uint32_t current_firmware_version = 10;

    shadow.device_update.presence_flags = IOT_DEVICE_UPDATE_PRESENCE_FLAG_FIRMWARE_UPDATE;
    shadow.device_update.firmware_update.version = current_firmware_version + 1;
    strncpy(shadow.device_update.firmware_update.url.domain, "ThisIsATestDomain", sizeof(shadow.device_update.firmware_update.url.domain));
    strncpy(shadow.device_update.firmware_update.url.path, "/path/to/firmware/update", sizeof(shadow.device_update.firmware_update.url.path));
    shadow.device_update.firmware_update.url.port = 8080;

    uint32_t file_size;

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_get_network_info_ExpectAnyArgs();
    iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_fetch_device_shadow_ReturnThruPtr_shadow(&shadow);

    syshal_device_firmware_version_ExpectAndReturn(&current_firmware_version, SYSHAL_DEVICE_NO_ERROR);
    syshal_device_firmware_version_IgnoreArg_version();
    syshal_device_firmware_version_ReturnThruPtr_version(&current_firmware_version);

    iot_download_file_ExpectAndReturn(0, &shadow.device_update.firmware_update.url, file_system, FILE_ID_APP_FIRM_IMAGE, &file_size, IOT_NO_ERROR);
    iot_download_file_IgnoreArg_timeout_ms();
    iot_download_file_IgnoreArg_file_size();
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    syshal_firmware_update_ExpectAndReturn(FILE_ID_APP_FIRM_IMAGE, shadow.device_update.firmware_update.version, SYSHAL_FIRMWARE_NO_ERROR);

    update_position(0);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());

    std::queue<sm_iot_event_id_t> expected_events;
    expected_events.push(SM_IOT_ABOUT_TO_POWER_ON);
    expected_events.push(SM_IOT_CELLULAR_POWER_ON);
    expected_events.push(SM_IOT_CELLULAR_NETWORK_INFO);
    expected_events.push(SM_IOT_CELLULAR_CONNECT);
    expected_events.push(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW);
    expected_events.push(SM_IOT_CELLULAR_SEND_DEVICE_STATUS);
    expected_events.push(SM_IOT_CELLULAR_DOWNLOAD_FIRMWARE_FILE);
    expected_events.push(SM_IOT_CELLULAR_POWER_OFF);
    expected_events.push(SM_IOT_APPLY_FIRMWARE_UPDATE);

    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, cellularFirmwareUpdateCurrentVersion)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_cellular_settings.hdr.set = true;
    iot_cellular_settings.contents.enable = true;
    iot_cellular_settings.contents.min_updates = 1;
    iot_cellular_settings.contents.check_firmware_updates = true;

    iot_cellular_aws_settings.hdr.set = true;

    iot_cellular_apn.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_device_shadow_t shadow;
    uint32_t current_firmware_version = 10;

    shadow.device_update.presence_flags = IOT_DEVICE_UPDATE_PRESENCE_FLAG_FIRMWARE_UPDATE;
    shadow.device_update.firmware_update.version = current_firmware_version;
    strncpy(shadow.device_update.firmware_update.url.domain, "ThisIsATestDomain", sizeof(shadow.device_update.firmware_update.url.domain));
    strncpy(shadow.device_update.firmware_update.url.path, "/path/to/firmware/update", sizeof(shadow.device_update.firmware_update.url.path));
    shadow.device_update.firmware_update.url.port = 8080;

    uint32_t file_size;

    iot_power_on_ExpectAndReturn(IOT_RADIO_CELLULAR, IOT_NO_ERROR);
    iot_connect_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_get_network_info_ExpectAnyArgs();
    iot_fetch_device_shadow_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_fetch_device_shadow_ReturnThruPtr_shadow(&shadow);

    syshal_device_firmware_version_ExpectAndReturn(&current_firmware_version, SYSHAL_DEVICE_NO_ERROR);
    syshal_device_firmware_version_IgnoreArg_version();
    syshal_device_firmware_version_ReturnThruPtr_version(&current_firmware_version);

    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    update_position(0);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());

    auto expected_events = cellular_expected_events_transmission_without_logging();
    compare_expected_events_to_actual(expected_events, __LINE__);
}

/////////////////////////////////////////////////////////
//////////////////// Satellite Tests ////////////////////
/////////////////////////////////////////////////////////

TEST_F(SmIotTest, satelliteForceTriggerPowerOnFailure)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_sat_settings.hdr.set = true;
    iot_sat_settings.contents.enable = true;

    iot_sat_artic_settings.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_power_on_ExpectAndReturn(IOT_RADIO_SATELLITE, IOT_ERROR_POWER_ON_FAIL);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_ERROR_CONNECTION_FAILED, sm_iot_trigger_force(IOT_RADIO_SATELLITE));

    std::queue<sm_iot_event_id_t> expected_event_ids;
    expected_event_ids.push(SM_IOT_ABOUT_TO_POWER_ON);
    expected_event_ids.push(SM_IOT_SATELLITE_POWER_ON);
    expected_event_ids.push(SM_IOT_SATELLITE_POWER_OFF);
    compare_expected_events_to_actual(expected_event_ids, __LINE__);
}

TEST_F(SmIotTest, satelliteForceTriggerSendDeviceStatusFailure)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_sat_settings.hdr.set = true;
    iot_sat_settings.contents.enable = true;

    iot_sat_artic_settings.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_power_on_ExpectAndReturn(IOT_RADIO_SATELLITE, IOT_NO_ERROR);
    iot_send_device_status_ExpectAndReturn(0, 0, IOT_ERROR_TRANSMISSION_FAIL);
    iot_send_device_status_IgnoreArg_timeout_ms();
    iot_send_device_status_IgnoreArg_device_status();
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_ERROR_CONNECTION_FAILED, sm_iot_trigger_force(IOT_RADIO_SATELLITE));

    std::queue<sm_iot_event_id_t> expected_event_ids;
    expected_event_ids.push(SM_IOT_ABOUT_TO_POWER_ON);
    expected_event_ids.push(SM_IOT_SATELLITE_POWER_ON);
    expected_event_ids.push(SM_IOT_SATELLITE_SEND_DEVICE_STATUS);
    expected_event_ids.push(SM_IOT_SATELLITE_POWER_OFF);
    compare_expected_events_to_actual(expected_event_ids, __LINE__);
}

TEST_F(SmIotTest, satelliteForceTrigger)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_sat_settings.hdr.set = true;
    iot_sat_settings.contents.enable = true;

    iot_sat_artic_settings.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    iot_power_on_ExpectAndReturn(IOT_RADIO_SATELLITE, IOT_NO_ERROR);
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_SATELLITE));

    auto expected_events = satellite_expected_events();
    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, satelliteIgnorePrepas)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_sat_settings.hdr.set = true;
    iot_sat_settings.contents.enable = true;
    iot_sat_settings.contents.min_updates = 1;

    iot_sat_artic_settings.hdr.set = true;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    update_position(1);

    iot_power_on_ExpectAndReturn(IOT_RADIO_SATELLITE, IOT_NO_ERROR);
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());

    auto expected_events = satellite_expected_events();
    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, satelliteNoWindow)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_sat_settings.hdr.set = true;
    iot_sat_settings.contents.enable = true;
    iot_sat_settings.contents.min_updates = 1;

    iot_sat_artic_settings.hdr.set = true;
    iot_sat_artic_settings.contents.bulletin_data[0].sat[0] = 1;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    update_position(1, true);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());
    EXPECT_EQ(0, occured_events.size());
}

TEST_F(SmIotTest, satelliteInWindow)
{
    const uint32_t time_till_prepas = (rand() % 10) + 2;

    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_sat_settings.hdr.set = true;
    iot_sat_settings.contents.enable = true;
    iot_sat_settings.contents.min_updates = 1;

    iot_sat_artic_settings.hdr.set = true;
    iot_sat_artic_settings.contents.bulletin_data[0].sat[0] = 1;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());
    EXPECT_EQ(0, occured_events.size());

    update_position(time_till_prepas, true);

    increment_time(time_till_prepas - 1);
    EXPECT_EQ(0, occured_events.size());

    iot_power_on_ExpectAndReturn(IOT_RADIO_SATELLITE, IOT_NO_ERROR);
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    increment_time(1);

    auto expected_events = satellite_expected_events();
    compare_expected_events_to_actual(expected_events, __LINE__);

    increment_time(time_till_prepas * 2);
    EXPECT_EQ(0, occured_events.size());
}

TEST_F(SmIotTest, satelliteMaxInterval)
{
    const uint32_t max_interval = (rand() % 10) + 2;
    const uint32_t time_till_sat_window = max_interval + (rand() % 5) + 2;

    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_sat_settings.hdr.set = true;
    iot_sat_settings.contents.enable = true;
    iot_sat_settings.contents.max_interval = max_interval;

    iot_sat_artic_settings.hdr.set = true;
    iot_sat_artic_settings.contents.bulletin_data[0].sat[0] = 1;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    update_position(max_interval + time_till_sat_window, true);

    // The satellite shouldn't fire after max_interval as we are not in a prepas window
    increment_time(max_interval);
    EXPECT_EQ(0, occured_events.size());

    // Step to one second before the prepas window
    increment_time(time_till_sat_window - 1);
    EXPECT_EQ(0, occured_events.size());

    iot_power_on_ExpectAndReturn(IOT_RADIO_SATELLITE, IOT_NO_ERROR);
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    increment_time(1);

    // Satellite should now fire as max_interval has elapsed and we are in a prepas window
    auto expected_events = satellite_expected_events();
    compare_expected_events_to_actual(expected_events, __LINE__);

    // Satellite should not re-fire until a new prepas window has been entered
    increment_time(max_interval);
    EXPECT_EQ(0, occured_events.size());

    iot_power_on_ExpectAndReturn(IOT_RADIO_SATELLITE, IOT_NO_ERROR);
    iot_send_device_status_ExpectAnyArgsAndReturn(IOT_NO_ERROR);
    iot_power_off_ExpectAndReturn(IOT_NO_ERROR);

    // So enter a new prepas window
    update_position(1, true);
    increment_time(1);

    compare_expected_events_to_actual(expected_events, __LINE__);
}

TEST_F(SmIotTest, satelliteMinInterval)
{
    const uint32_t min_interval = (rand() % 10) + 2;
    const uint32_t time_till_sat_window = min_interval + (rand() % 5) + 2;

    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_sat_settings.hdr.set = true;
    iot_sat_settings.contents.enable = true;
    iot_sat_settings.contents.min_interval = min_interval;

    iot_sat_artic_settings.hdr.set = true;
    iot_sat_artic_settings.contents.bulletin_data[0].sat[0] = 1;

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    update_position(1, true);

    // The satellite shouldn't fire as our min_interval did not elapse before our prepas window was reached
    increment_time(10);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());
    EXPECT_EQ(0, occured_events.size());
}

TEST_F(SmIotTest, next_predict)
{
    iot_general_settings.hdr.set = true;
    iot_general_settings.contents.enable = true;

    iot_sat_settings.hdr.set = true;
    iot_sat_settings.contents.enable = true;

    iot_sat_artic_settings.hdr.set = true;
    iot_sat_artic_settings.contents.bulletin_data[0].sat[0] = 1;

    config.iot_init.iot_sat_config->contents.randomised_tx_window = 20;
    uint32_t value = rand();
    memcpy(&config.iot_init.iot_sat_artic_config->contents.device_identifier, &value, 3);

    iot_init_IgnoreAndReturn(IOT_NO_ERROR);
    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_init(config));

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_tick());
    EXPECT_EQ(0, occured_events.size());

    sm_iot_timestamped_position_t timestamped_pos;
    timestamped_pos.longitude = 52.00; // We don't really care what the position is for these tests
    timestamped_pos.latitude = 1.00;
    timestamped_pos.timestamp = 1580068019;

    iot_prepass_result_t result = 1580068417;

    iot_calc_prepass_ExpectAndReturn(nullptr, &result, IOT_NO_ERROR);
    iot_calc_prepass_ReturnThruPtr_result(&result);
    iot_calc_prepass_IgnoreArg_result();
    iot_calc_prepass_IgnoreArg_gps(); // TODO: Check the GPS arguments

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_new_position(timestamped_pos));
}