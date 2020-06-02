// test_gps_scheduler.cpp - Filesystem unit tests
//
// Copyright (C) 2019 Icoteq
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

extern "C" {
#include <stdint.h>
#include <stdlib.h>
#include "unity.h"
#include "Mocksyshal_rtc.h"
#include "Mocksyshal_gps.h"
#include "Mocksyshal_time.h"
#include "gps_scheduler.h"
#include "syshal_timer.h"
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include <list>
#include <vector>

// syshal_time

int syshal_time_init_GTest(int cmock_num_calls) {return SYSHAL_TIME_NO_ERROR;}

uint64_t syshal_time_get_ticks_ms_value;
uint32_t syshal_time_get_ticks_ms_GTest(int cmock_num_calls)
{
    return static_cast<uint32_t>(syshal_time_get_ticks_ms_value);
}

void syshal_time_delay_us_GTest(uint32_t us, int cmock_num_calls) {}
void syshal_time_delay_ms_GTest(uint32_t ms, int cmock_num_calls) {}

// syshal_rtc
time_t current_date_time = time(0);
uint32_t current_milliseconds = 0;

int syshal_rtc_init_GTest(int cmock_num_calls) {return SYSHAL_RTC_NO_ERROR;}

int syshal_rtc_set_date_and_time_GTest(syshal_rtc_data_and_time_t date_time, int cmock_num_calls)
{
    struct tm * timeinfo = localtime(&current_date_time);

    timeinfo->tm_sec = date_time.seconds; // seconds of minutes from 0 to 61
    timeinfo->tm_min = date_time.minutes; // minutes of hour from 0 to 59
    timeinfo->tm_hour = date_time.hours;  // hours of day from 0 to 24
    timeinfo->tm_mday = date_time.day;    // day of month from 1 to 31
    timeinfo->tm_mon = date_time.month;   // month of year from 0 to 11
    timeinfo->tm_year = date_time.year;   // year since 1900

    current_date_time = mktime(timeinfo);

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_date_and_time_GTest(syshal_rtc_data_and_time_t * date_time, int cmock_num_calls)
{
    struct tm * timeinfo = localtime(&current_date_time);

    date_time->milliseconds = current_milliseconds;
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
    *timestamp = (uint32_t) current_date_time;
    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_uptime_GTest(uint32_t * timestamp, int cmock_num_calls)
{
    *timestamp = (uint32_t) syshal_time_get_ticks_ms_value / 1000;
    return SYSHAL_RTC_NO_ERROR;
}

// syshal_gps
syshal_gps_state_t gps_state;
int syshal_gps_init_GTest(int cmock_num_calls)
{
    gps_state = STATE_ASLEEP;
    return SYSHAL_GPS_NO_ERROR;
}

int syshal_gps_shutdown_GTest(int cmock_num_calls)
{
    gps_state = STATE_ASLEEP;
    return SYSHAL_GPS_NO_ERROR;
}

int syshal_gps_wake_up_GTest(int cmock_num_calls)
{
    gps_state = STATE_ACQUIRING;
    return SYSHAL_GPS_NO_ERROR;
}

syshal_gps_state_t syshal_gps_get_state_GTest(int cmock_num_calls)
{
    return gps_state;
}

sys_config_gps_trigger_mode_t                          trigger_mode;
sys_config_gps_scheduled_acquisition_interval_t        scheduled_acquisition_interval;
sys_config_gps_maximum_acquisition_time_t              maximum_acquisition_time;
sys_config_gps_scheduled_acquisition_no_fix_timeout_t  scheduled_acquisition_no_fix_timeout;
sys_config_gps_max_fixes_t                             max_fixes;

gps_scheduler_init_t init_config = {
    &trigger_mode,
    &scheduled_acquisition_interval,
    &maximum_acquisition_time,
    &scheduled_acquisition_no_fix_timeout,
    &max_fixes
};

class GpsSchedulerTest : public ::testing::Test {

    virtual void SetUp() {
        Mocksyshal_rtc_Init();
        Mocksyshal_gps_Init();
        Mocksyshal_time_Init();

        // syshal_time
        syshal_time_init_StubWithCallback(syshal_time_init_GTest);
        syshal_time_delay_us_StubWithCallback(syshal_time_delay_us_GTest);
        syshal_time_delay_ms_StubWithCallback(syshal_time_delay_ms_GTest);
        syshal_time_get_ticks_ms_StubWithCallback(syshal_time_get_ticks_ms_GTest);
        syshal_time_get_ticks_ms_value = rand() / 2; // Start with a random up time (not a typical use case but will help stress testing)

        // syshal_rtc
        syshal_rtc_init_StubWithCallback(syshal_rtc_init_GTest);
        syshal_rtc_set_date_and_time_StubWithCallback(syshal_rtc_set_date_and_time_GTest);
        syshal_rtc_get_date_and_time_StubWithCallback(syshal_rtc_get_date_and_time_GTest);
        syshal_rtc_get_timestamp_StubWithCallback(syshal_rtc_get_timestamp_GTest);
        syshal_rtc_get_uptime_StubWithCallback(syshal_rtc_get_uptime_GTest);
        syshal_rtc_set_alarm_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);
        syshal_rtc_soft_watchdog_enable_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);
        syshal_rtc_soft_watchdog_refresh_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);

        // syshal_gps
        syshal_gps_init_StubWithCallback(syshal_gps_init_GTest);
        syshal_gps_shutdown_StubWithCallback(syshal_gps_shutdown_GTest);
        syshal_gps_wake_up_StubWithCallback(syshal_gps_wake_up_GTest);
        syshal_gps_get_state_StubWithCallback(syshal_gps_get_state_GTest);
        syshal_gps_tick_IgnoreAndReturn(SYSHAL_GPS_NO_ERROR);
        syshal_gps_is_present_IgnoreAndReturn(true);
    }

    virtual void TearDown() {
        // Reset all syshal timers
        for (timer_handle_t i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
            syshal_timer_term(i);

        Mocksyshal_rtc_Verify();
        Mocksyshal_rtc_Destroy();
        Mocksyshal_gps_Verify();
        Mocksyshal_gps_Destroy();
        Mocksyshal_time_Verify();
        Mocksyshal_time_Destroy();
    }

public:
    static void IncrementMilliseconds(uint32_t milliseconds)
    {
        struct tm * timeinfo = localtime(&current_date_time);
        timeinfo->tm_sec += (current_milliseconds + milliseconds) / 1000;
        current_date_time = mktime(timeinfo);
        current_milliseconds = (current_milliseconds + milliseconds) % 1000;
        syshal_time_get_ticks_ms_value += milliseconds;
    }

    static void IncrementSeconds(uint32_t seconds)
    {
        IncrementMilliseconds(seconds * 1000);
        syshal_timer_tick();
    }
};

TEST_F(GpsSchedulerTest, InitBeforeGPSInit)
{
    gps_state = STATE_UNINIT;
    EXPECT_EQ(GPS_SCHEDULER_ERROR_INVALID_STATE, gps_scheduler_init(init_config));
}

TEST_F(GpsSchedulerTest, InitSucessful)
{
    gps_state = STATE_ASLEEP;
    EXPECT_EQ(GPS_SCHEDULER_NO_ERROR, gps_scheduler_init(init_config));
}

TEST_F(GpsSchedulerTest, ScheduledModeNoFix)
{
    trigger_mode.hdr.set = true;
    trigger_mode.contents.mode = SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED_BITMASK;
    scheduled_acquisition_interval.hdr.set = true;
    scheduled_acquisition_interval.contents.seconds = 60;
    maximum_acquisition_time.hdr.set = true;
    maximum_acquisition_time.contents.seconds = 30;
    scheduled_acquisition_no_fix_timeout.hdr.set = true;
    scheduled_acquisition_no_fix_timeout.contents.seconds = 10;
    
    gps_state = STATE_ASLEEP;
    EXPECT_EQ(GPS_SCHEDULER_NO_ERROR, gps_scheduler_init(init_config));
    EXPECT_EQ(GPS_SCHEDULER_NO_ERROR, gps_scheduler_start());

    for (int i = 0; i < scheduled_acquisition_interval.contents.seconds; ++i)
    {
        EXPECT_EQ(STATE_ASLEEP, gps_state);
        IncrementSeconds(1);
    }

    for (int test_num = 0; test_num < 3; ++test_num)
    {
        for (int i = 0; i < scheduled_acquisition_no_fix_timeout.contents.seconds; ++i)
        {
            EXPECT_EQ(STATE_ACQUIRING, gps_state);
            IncrementSeconds(1);
        }

        for (int i = 0; i < scheduled_acquisition_interval.contents.seconds - scheduled_acquisition_no_fix_timeout.contents.seconds; ++i)
        {
            EXPECT_EQ(STATE_ASLEEP, gps_state);
            IncrementSeconds(1);
        }
    }
}

TEST_F(GpsSchedulerTest, ScheduledModeFixed)
{
    trigger_mode.hdr.set = true;
    trigger_mode.contents.mode = SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED_BITMASK;
    scheduled_acquisition_interval.hdr.set = true;
    scheduled_acquisition_interval.contents.seconds = 60;
    maximum_acquisition_time.hdr.set = true;
    maximum_acquisition_time.contents.seconds = 30;
    scheduled_acquisition_no_fix_timeout.hdr.set = true;
    scheduled_acquisition_no_fix_timeout.contents.seconds = 10;
    
    gps_state = STATE_ASLEEP;
    EXPECT_EQ(GPS_SCHEDULER_NO_ERROR, gps_scheduler_init(init_config));
    EXPECT_EQ(GPS_SCHEDULER_NO_ERROR, gps_scheduler_start());

    for (int i = 0; i < scheduled_acquisition_interval.contents.seconds; ++i)
    {
        EXPECT_EQ(STATE_ASLEEP, gps_state);
        IncrementSeconds(1);
    }

    for (int test_num = 0; test_num < 3; ++test_num)
    {
        gps_state = STATE_FIXED;
        syshal_gps_event_t fixed_event;
        fixed_event.id = SYSHAL_GPS_EVENT_STATUS;
        syshal_gps_callback(&fixed_event);

        for (int i = 0; i < maximum_acquisition_time.contents.seconds; ++i)
        {
            EXPECT_EQ(STATE_FIXED, gps_state);
            IncrementSeconds(1);
        }

        for (int i = 0; i < scheduled_acquisition_interval.contents.seconds - maximum_acquisition_time.contents.seconds; ++i)
        {
            EXPECT_EQ(STATE_ASLEEP, gps_state);
            IncrementSeconds(1);
        }
    }
}