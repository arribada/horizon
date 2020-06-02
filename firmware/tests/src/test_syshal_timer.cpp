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
//
extern "C" {
#include <stdint.h>
#include "unity.h"
#include "syshal_timer.h"
#include "Mocksyshal_rtc.h"
#include <stdlib.h>
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include <list>
#include <vector>

bool timerTriggered = false;

void timerCallback(void)
{
    timerTriggered = true;
}

class Syshal_TimerTest : public ::testing::Test
{

    virtual void SetUp()
    {
        srand(time(NULL));

        Mocksyshal_rtc_Init();
    }

    virtual void TearDown()
    {
        // Clear any timers
        for (timer_handle_t i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
            syshal_timer_term(i);

        Mocksyshal_rtc_Verify();
        Mocksyshal_rtc_Destroy();
    }

public:

};

TEST_F(Syshal_TimerTest, InitTimer)
{
    timer_handle_t handle;

    EXPECT_EQ(SYSHAL_TIMER_NO_ERROR, syshal_timer_init(&handle, nullptr));
}

TEST_F(Syshal_TimerTest, NoFreeTimer)
{
    timer_handle_t handle;

    for (auto i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
        syshal_timer_init(&handle, nullptr);

    EXPECT_EQ(SYSHAL_TIMER_ERROR_NO_FREE_TIMER, syshal_timer_init(&handle, nullptr));
}

TEST_F(Syshal_TimerTest, RandomInitTimer)
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

TEST_F(Syshal_TimerTest, TermInvalid)
{
    EXPECT_EQ(SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE, syshal_timer_term(0xAA));
}