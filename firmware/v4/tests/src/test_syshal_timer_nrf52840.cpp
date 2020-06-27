// test_syshal_timer.cpp - Syshal_timer unit tests
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
#include <stdint.h>
#include "unity.h"
#include "syshal_timer.h"
#include "nrfx_rtc.h"
#include "syshal_rtc.h"
#include <stdlib.h>
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include <list>
#include <deque>

uint32_t up_time;

std::deque<uint32_t> timer_triggered;
void timer_callback(void) { timer_triggered.push_back(up_time); }
std::deque<uint32_t> wakeup_event;
void syshal_rtc_wakeup_event(void) { wakeup_event.push_back(up_time); }

class Syshal_TimerTestNRF52840 : public ::testing::Test
{
    virtual void SetUp()
    {
        srand(time(NULL));
        up_time = 0;
        timer_triggered.clear();
        wakeup_event.clear();

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_init());

        // Start at a random date
        set_random_date_time();
    }

    virtual void TearDown()
    {
        // Clear any timers
        for (timer_handle_t i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
            syshal_timer_term(i);

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_term());
    }

public:

    void set_random_date_time()
    {
        syshal_rtc_data_and_time_t set_date_time;
        set_date_time.year = rand() % 30 + 1970;
        set_date_time.month = rand() % 12 + 1;
        set_date_time.day = rand() % 28 + 1;
        set_date_time.hours = rand() % 24;
        set_date_time.minutes = rand() % 60;
        set_date_time.seconds = rand() % 60;

        EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));
    }

    void increment_time(uint32_t seconds)
    {
        for (auto i = 0; i < seconds; ++i)
        {
            up_time++;
            nrfx_rtc_increment_second();
            syshal_timer_tick();
            syshal_timer_recalculate_next_alarm();
        }
    }
};

TEST_F(Syshal_TimerTestNRF52840, InitTimer)
{
    timer_handle_t handle;

    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_init(&handle, nullptr));
}

TEST_F(Syshal_TimerTestNRF52840, NoFreeTimer)
{
    timer_handle_t handle;

    for (auto i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
        syshal_timer_init(&handle, nullptr);

    EXPECT_EQ(SYSHAL_TIMER_ERROR_NO_FREE_TIMER, syshal_timer_init(&handle, nullptr));
}

TEST_F(Syshal_TimerTestNRF52840, RandomInitTimer)
{
    std::vector<timer_handle_t> handles;

    unsigned int numberOfInitHandles = 0;

    for (auto i = 0; i < 1000; ++i)
    {
        if (rand() % 2)
        {
            // Create new timer
            int expected_return = handles.size() >= SYSHAL_TIMER_NUMBER_OF_TIMERS ? SYSHAL_TIMER_ERROR_NO_FREE_TIMER : SYSHAL_TIMER_NO_ERROR;

            timer_handle_t handle;
            EXPECT_EQ(expected_return, syshal_timer_init(&handle, nullptr));

            if (SYSHAL_TIMER_NO_ERROR == expected_return)
                handles.push_back(handle);
        }
        else
        {
            if (handles.size())
            {
                unsigned int index = rand() % handles.size();
                EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_term(handles[index]));
                handles.erase(handles.begin() + index);
            }
        }
    }
}

TEST_F(Syshal_TimerTestNRF52840, TermInvalid)
{
    EXPECT_EQ(SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE, syshal_timer_term(0xAA));
}

TEST_F(Syshal_TimerTestNRF52840, 30SecondOneShotTimer)
{
    timer_handle_t handle;

    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_init(&handle, timer_callback));
    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_set(handle, one_shot, 30));

    increment_time(29);
    EXPECT_EQ(0, timer_triggered.size());

    increment_time(1);

    ASSERT_EQ(1, timer_triggered.size());
    EXPECT_EQ(30, timer_triggered.front());

    ASSERT_EQ(1, wakeup_event.size());
    EXPECT_EQ(30, wakeup_event.front());
}

TEST_F(Syshal_TimerTestNRF52840, 30SecondPeriodicTimer)
{
    timer_handle_t handle;

    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_init(&handle, timer_callback));
    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_set(handle, periodic, 30));

    increment_time(120);

    ASSERT_EQ(4, timer_triggered.size());
    EXPECT_EQ(30, timer_triggered.front());
    timer_triggered.pop_front();
    EXPECT_EQ(60, timer_triggered.front());
    timer_triggered.pop_front();
    EXPECT_EQ(90, timer_triggered.front());
    timer_triggered.pop_front();
    EXPECT_EQ(120, timer_triggered.front());

    ASSERT_EQ(4, wakeup_event.size());
    EXPECT_EQ(30, wakeup_event.front());
    wakeup_event.pop_front();
    EXPECT_EQ(60, wakeup_event.front());
    wakeup_event.pop_front();
    EXPECT_EQ(90, wakeup_event.front());
    wakeup_event.pop_front();
    EXPECT_EQ(120, wakeup_event.front());
}

TEST_F(Syshal_TimerTestNRF52840, ChangeTimeBackwards5Seconds)
{
    timer_handle_t handle;
    syshal_rtc_data_and_time_t set_date_time;

    set_date_time.year = 2000;
    set_date_time.month = 6;
    set_date_time.day = 15;
    set_date_time.hours = 12;
    set_date_time.minutes = 30;
    set_date_time.seconds = 30;

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_init(&handle, timer_callback));
    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_set(handle, one_shot, 30));

    set_date_time.seconds -= 5;
    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

    increment_time(60);

    ASSERT_EQ(1, timer_triggered.size());
    EXPECT_EQ(30, timer_triggered.front());
    ASSERT_EQ(1, wakeup_event.size());
    EXPECT_EQ(30, wakeup_event.front());
}

TEST_F(Syshal_TimerTestNRF52840, ChangeTimeForward5Seconds)
{
    timer_handle_t handle;
    syshal_rtc_data_and_time_t set_date_time;

    set_date_time.year = 2000;
    set_date_time.month = 6;
    set_date_time.day = 15;
    set_date_time.hours = 12;
    set_date_time.minutes = 30;
    set_date_time.seconds = 30;

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_init(&handle, timer_callback));
    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_set(handle, one_shot, 30));

    set_date_time.seconds += 5;
    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

    increment_time(60);

    ASSERT_EQ(1, timer_triggered.size());
    EXPECT_EQ(30, timer_triggered.front());
    ASSERT_EQ(1, wakeup_event.size());
    EXPECT_EQ(30, wakeup_event.front());
}

TEST_F(Syshal_TimerTestNRF52840, ChangeTimeBackwards5Minutes)
{
    timer_handle_t handle;
    syshal_rtc_data_and_time_t set_date_time;

    set_date_time.year = 2000;
    set_date_time.month = 6;
    set_date_time.day = 15;
    set_date_time.hours = 12;
    set_date_time.minutes = 30;
    set_date_time.seconds = 30;

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_init(&handle, timer_callback));
    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_set(handle, one_shot, 30));

    set_date_time.minutes -= 5;
    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

    increment_time(10 * 60);

    ASSERT_EQ(1, timer_triggered.size());
    EXPECT_EQ(30, timer_triggered.front());
    ASSERT_EQ(1, wakeup_event.size());
    EXPECT_EQ(30, wakeup_event.front());
}

TEST_F(Syshal_TimerTestNRF52840, ChangeTimeForward5Minutes)
{
    timer_handle_t handle;
    syshal_rtc_data_and_time_t set_date_time;

    set_date_time.year = 2000;
    set_date_time.month = 6;
    set_date_time.day = 15;
    set_date_time.hours = 12;
    set_date_time.minutes = 30;
    set_date_time.seconds = 30;

    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_init(&handle, timer_callback));
    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_set(handle, one_shot, 30));

    set_date_time.minutes += 5;
    EXPECT_EQ(SYSHAL_RTC_NO_ERROR, syshal_rtc_set_date_and_time(set_date_time));

    increment_time(10 * 60);

    ASSERT_EQ(1, timer_triggered.size());
    EXPECT_EQ(30, timer_triggered.front());
    ASSERT_EQ(1, wakeup_event.size());
    EXPECT_EQ(30, wakeup_event.front());
}

TEST_F(Syshal_TimerTestNRF52840, MultipleTimers)
{
    timer_handle_t handles[SYSHAL_TIMER_NUMBER_OF_TIMERS];

    std::multiset<uint32_t> expected_timer_times;

    for (auto i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
    {
        EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_init(&handles[i], timer_callback));
        uint32_t time = rand() % 1024;
        expected_timer_times.insert(time);
        EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_set(handles[i], one_shot, time));
    }

    increment_time(1024 * 2);

    // Expect every timer to have fired
    ASSERT_EQ(SYSHAL_TIMER_NUMBER_OF_TIMERS, timer_triggered.size());

    // Any timers that share a wakeup time will only generate one event
    // Thus we must find how many unique wakeup events there are
    std::set<uint32_t> unique_times(timer_triggered.begin(), timer_triggered.end());
    ASSERT_EQ(unique_times.size(), wakeup_event.size());

    // Now check they all fired in the correct order
    for (auto i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
    {
        uint32_t expected_time = *std::next(expected_timer_times.begin(), i);
        EXPECT_EQ(expected_time, timer_triggered.front());
        timer_triggered.pop_front();
    }

    // And that we got the right times for our wakeup events
    for (auto i = 0; i < unique_times.size(); ++i)
    {
        uint32_t expected_time = *std::next(unique_times.begin(), i);
        EXPECT_EQ(expected_time, wakeup_event.front());
        wakeup_event.pop_front();
    }
}