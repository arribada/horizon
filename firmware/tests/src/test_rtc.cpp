// test_rtc.cpp - RTC unit tests
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

extern "C" {
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "unity.h"
#include "syshal_rtc.h"
#include "nrfx_rtc.h"
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

class RTCTest : public ::testing::Test
{
    virtual void SetUp()
    {
        srand(time(NULL));

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_init());
    }

    virtual void TearDown()
    {
        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_term());
    }

public:
    void increment_seconds(unsigned int seconds = 1)
    {
        for (auto i = 0; i < seconds; ++i)
            nrfx_rtc_increment_second();
    }
};

TEST_F(RTCTest, RTCUptime)
{
    syshal_rtc_data_and_time_t set_date_time;
    uint32_t uptime;
    uint64_t time_elapsed = 0;
    

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_uptime(&uptime));
    auto start_time = uptime;

    for (auto i = 0; i < 100; ++i)
    {
        // Our uptime should not change even if our date does
        set_date_time.year = rand() % 30 + 1970;
        set_date_time.month = rand() % 12 + 1;
        set_date_time.day = rand() % 28 + 1;
        set_date_time.hours = rand() % 24;
        set_date_time.minutes = rand() % 60;
        set_date_time.seconds = rand() % 60;

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

        auto step = rand() % 600;
        time_elapsed += step;
        increment_seconds(step);
        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_uptime(&uptime));
        EXPECT_EQ(uptime, start_time + time_elapsed);
    }
}

TEST_F(RTCTest, RTCInit)
{
    uint32_t timestamp;

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_timestamp(&timestamp));
    EXPECT_EQ(0, timestamp);

    auto step = rand() % 600;
    increment_seconds(step);

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_timestamp(&timestamp));
    EXPECT_EQ(step, timestamp);
}

TEST_F(RTCTest, RTCFixedDate)
{
    syshal_rtc_data_and_time_t set_date_time, get_date_time;
    uint32_t timestamp;
    uint32_t expected_timestamp;

    set_date_time.year = 2019;
    set_date_time.month = 6;
    set_date_time.day = 11;
    set_date_time.hours = 19;
    set_date_time.minutes = 20;
    set_date_time.seconds = 3;

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_date_and_time(&get_date_time));

    EXPECT_EQ(get_date_time.year, set_date_time.year);
    EXPECT_EQ(get_date_time.month, set_date_time.month);
    EXPECT_EQ(get_date_time.day, set_date_time.day);
    EXPECT_EQ(get_date_time.hours, set_date_time.hours);
    EXPECT_EQ(get_date_time.minutes, set_date_time.minutes);
    EXPECT_EQ(get_date_time.seconds, set_date_time.seconds);

    expected_timestamp = 0x5cfffee3;

    for (auto i = 0; i < 10; ++i)
    {
        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_timestamp(&timestamp));
        EXPECT_EQ(expected_timestamp, timestamp) << "iteration: " << i;
        increment_seconds();
        expected_timestamp++;
    }
}

TEST_F(RTCTest, RTCRandomDateTimeFullCycle)
{
    syshal_rtc_data_and_time_t get_date_time;
    syshal_rtc_data_and_time_t set_date_time;
    uint32_t expected_timestamp;
    uint32_t timestamp;

    for (uint32_t i = 0; i < 10; ++i)
    {
        for (auto i = 0; i < rand() & 0xFFFF; ++i)
            nrfx_rtc_increment_tick();

        set_date_time.year = rand() % 30 + 1970;
        set_date_time.month = rand() % 12 + 1;
        set_date_time.day = rand() % 28 + 1;
        set_date_time.hours = rand() % 24;
        set_date_time.minutes = rand() % 60;
        set_date_time.seconds = rand() % 60;

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_date_and_time(&get_date_time));

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_timestamp(&expected_timestamp));

        for (auto i = 0; i < pow(2,24)/8+10; ++i)
        {
            EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_timestamp(&timestamp));
            EXPECT_EQ(expected_timestamp, timestamp) << "iteration: " << i;
            increment_seconds();
            expected_timestamp++;
        }
    }
}

TEST_F(RTCTest, RTCRandomDateTime)
{
    syshal_rtc_data_and_time_t get_date_time;
    syshal_rtc_data_and_time_t set_date_time;

    for (uint32_t i = 0; i < 1000; ++i)
    {
        for (auto i = 0; i < rand() & 0xFFFF; ++i)
            nrfx_rtc_increment_tick();

        set_date_time.year = rand() % 30 + 1970;
        set_date_time.month = rand() % 12 + 1;
        set_date_time.day = rand() % 28 + 1;
        set_date_time.hours = rand() % 24;
        set_date_time.minutes = rand() % 60;
        set_date_time.seconds = rand() % 60;

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_date_and_time(&get_date_time));

        EXPECT_EQ(get_date_time.year, set_date_time.year);
        EXPECT_EQ(get_date_time.month, set_date_time.month);
        EXPECT_EQ(get_date_time.day, set_date_time.day);
        EXPECT_EQ(get_date_time.hours, set_date_time.hours);
        EXPECT_EQ(get_date_time.minutes, set_date_time.minutes);
        EXPECT_EQ(get_date_time.seconds, set_date_time.seconds);
    }
}

TEST_F(RTCTest, RTCMultipleOverflows)
{
    uint32_t timestamp;

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_timestamp(&timestamp));
    EXPECT_EQ(0, timestamp);

    for (auto i = 0; i < pow(2, 25) - 1; ++i)
        increment_seconds();

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_get_timestamp(&timestamp));
    EXPECT_EQ((unsigned int) pow(2, 25) - 1, timestamp);
}