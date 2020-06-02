/**
  ******************************************************************************
  * @file     syshal_rtc.c
  * @brief    System hardware abstraction layer for the real-time clock.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2019 Arribada</center></h2>
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
  *
  ******************************************************************************
  */

#include "syshal_rtc.h"
#include "syshal_timer.h"
#include "syshal_pmu.h"
#include "debug.h"
#include "bsp.h"
#include "app_util_platform.h" // CRITICAL_REGION def
#include "nrfx_rtc.h"
#ifndef GTEST
#include "retained_ram.h"
#include "nrf_power.h"
#endif

#ifndef NULL
#define NULL 0
#endif

// Software watchdog defines
#define RTC_SOFT_WATCHDOG_FREQUENCY_HZ (8)

static void (*soft_watchdog_callback)(unsigned int) = NULL; // User callback
static uint32_t soft_watchdog_timeout_s;
static bool soft_watchdog_init;

// Timekeeping defines
#define RTC_TIME_KEEPING_FREQUENCY_HZ (8) // 8 Hz, this is the lowest speed available

#define TICKS_PER_OVERFLOW   (16777216) // 16777216 = 2 ^ 24
#define SECONDS_PER_OVERFLOW (TICKS_PER_OVERFLOW / RTC_TIME_KEEPING_FREQUENCY_HZ)

uint32_t retained_ram_rtc_timestamp __attribute__ ((section(".noinit")));

static int64_t timestamp_offset;
static volatile uint32_t overflows_occured;

// Leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y) ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )

#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
#define SECS_PER_WEEK (SECS_PER_DAY * 7UL)
#define SECS_PER_YEAR (SECS_PER_DAY * 365UL) // For a non-leap year

#ifdef GTEST
RTC_InitTypeDefAndInst_t RTC_Inits[RTC_TOTAL_NUMBER];
#endif

static const uint8_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static void seconds_to_date_time(uint32_t seconds, syshal_rtc_data_and_time_t * data_and_time)
{
    // Implemented from https://github.com/PaulStoffregen/Time/blob/master/Time.cpp

    data_and_time->milliseconds = 0;

    uint32_t working_time = seconds;
    uint32_t days = 0;
    uint8_t month = 0;
    uint8_t month_length = 0;
    uint32_t year = 0;

    data_and_time->seconds = working_time % 60;
    working_time /= 60; // working_time is now in minutes
    data_and_time->minutes = working_time % 60;
    working_time /= 60; // working_time is now in hours
    data_and_time->hours = working_time % 24;
    working_time /= 24; // working_time is now in days

    // Calculate which year we are in
    while ((days += (LEAP_YEAR(year) ? 366 : 365)) <= working_time)
        year++; // year is offset from 1970

    data_and_time->year = year + 1970;

    days -= LEAP_YEAR(year) ? 366 : 365; // Move to the previous year
    working_time -= days; // Now working_time is in days this year

    days = 0;
    for (month = 0; month < 12; month++)
    {
        month_length = month_days[month];

        if (month == 1)
            if (LEAP_YEAR(year))
                month_length += 1; // Add an extra leap day to February

        if (working_time >= month_length)
            working_time -= month_length;
        else
            break;
    }
    data_and_time->month = month + 1;
    data_and_time->day = working_time + 1;
}

int syshal_rtc_date_time_to_timestamp(syshal_rtc_data_and_time_t data_and_time, uint32_t * timestamp)
{
    uint32_t seconds;

    // Seconds from 1970 till 1 jan 00:00:00 of the given year
    data_and_time.year -= 1970;
    seconds = data_and_time.year * (SECS_PER_DAY * 365);
    for (uint32_t i = 0; i < data_and_time.year; i++)
    {
        if (LEAP_YEAR(i))
            seconds += SECS_PER_DAY; // Add an extra day for a leap year
    }

    // Add the right number of days for each month
    for (uint32_t i = 1; i < data_and_time.month; i++)
    {
        if ( (i == 2) && LEAP_YEAR(data_and_time.year))
            seconds += SECS_PER_DAY * 29; // Add an extra day to feburary
        else
            seconds += SECS_PER_DAY * month_days[i - 1];
    }
    seconds += (data_and_time.day - 1) * SECS_PER_DAY;
    seconds += data_and_time.hours * SECS_PER_HOUR;
    seconds += data_and_time.minutes * SECS_PER_MIN;
    seconds += data_and_time.seconds;
    *timestamp = seconds;

    return SYSHAL_RTC_NO_ERROR;
}

static void rtc_time_keeping_event_handler(nrfx_rtc_int_type_t int_type)
{
    if (NRFX_RTC_INT_OVERFLOW == int_type)
        overflows_occured++;
    else if (NRFX_RTC_INT_COMPARE0 == int_type)
        syshal_rtc_wakeup_event();
}

static __attribute__ ((noinline)) void rtc_soft_watchdog_event_handler(nrfx_rtc_int_type_t int_type)
{
    // TODO: This function should return the address of the function that was interrupted
    // However, __builtin_return_address() > 0 is not avaliable on ARM so this needs to be the actual IRQ handler.
    // Currently the real IRQ handler is tucked away in nrfx_rtc.c
    //
    // unsigned int return_address = (unsigned int) __builtin_extract_return_addr(__builtin_return_address(0));

    // Store our current RTC time into a retained RAM section
    // so that it is maintained through a reset
    syshal_rtc_stash_time();

    unsigned int return_address = 0;
    if (soft_watchdog_callback)
        soft_watchdog_callback(return_address);
}

int syshal_rtc_init(void)
{
    overflows_occured = 0;
    timestamp_offset = 0;

    const nrfx_rtc_config_t rtc_config =
    {
        .prescaler          = RTC_FREQ_TO_PRESCALER(RTC_TIME_KEEPING_FREQUENCY_HZ),
        .interrupt_priority = RTC_Inits[RTC_TIME_KEEPING].irq_priority,
        .reliable           = 0,
        .tick_latency       = 0
    };

    nrfx_rtc_init(&RTC_Inits[RTC_TIME_KEEPING].rtc, &rtc_config, rtc_time_keeping_event_handler);
    nrfx_rtc_enable(&RTC_Inits[RTC_TIME_KEEPING].rtc);
    nrfx_rtc_overflow_enable(&RTC_Inits[RTC_TIME_KEEPING].rtc, true);

#ifndef GTEST
    if (syshal_pmu_get_startup_status() & NRF_POWER_RESETREAS_SREQ_MASK ||
        syshal_pmu_get_startup_status() & NRF_POWER_RESETREAS_DOG_MASK)
    {
        syshal_rtc_data_and_time_t date_time;
        seconds_to_date_time(retained_ram_rtc_timestamp, &date_time);
        syshal_rtc_set_date_and_time(date_time);
        //DEBUG_PR_TRACE("Reset cause: 0x%08lX, loaded RTC timestamp. Address: %p, value: %lu", syshal_pmu_get_startup_status(), &retained_ram_rtc_timestamp, retained_ram_rtc_timestamp);
    }
#endif

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_term(void)
{
    nrfx_rtc_uninit(&RTC_Inits[RTC_TIME_KEEPING].rtc);

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_stash_time(void)
{
    // Store our current RTC time into a retained RAM section
    // so that it is maintained through a reset
    uint32_t timestamp;
    int ret = syshal_rtc_get_timestamp(&timestamp);
    if (SYSHAL_RTC_NO_ERROR == ret)
        retained_ram_rtc_timestamp = timestamp;

    return ret;
}

int syshal_rtc_set_date_and_time(syshal_rtc_data_and_time_t date_time)
{
    uint32_t timestamp_target, timestamp_start;
    syshal_rtc_date_time_to_timestamp(date_time, &timestamp_target);

    CRITICAL_REGION_ENTER();
    syshal_rtc_get_uptime(&timestamp_start);
    timestamp_offset = (int64_t) timestamp_target - timestamp_start;
    CRITICAL_REGION_EXIT();

    // Re-run syshal_timer_tick() to ensure any alarms are re-established
    syshal_timer_tick();

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_date_and_time(syshal_rtc_data_and_time_t * date_time)
{
    uint32_t current_time;
    int ret = syshal_rtc_get_timestamp(&current_time);
    if (ret)
        return ret;

    seconds_to_date_time(current_time, date_time);

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_timestamp(uint32_t * timestamp)
{
    uint32_t current_time;
    syshal_rtc_get_uptime(&current_time);
    *timestamp = (int64_t) current_time + timestamp_offset;

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_uptime(uint32_t * uptime)
{
    CRITICAL_REGION_ENTER();
    *uptime = nrfx_rtc_counter_get(&RTC_Inits[RTC_TIME_KEEPING].rtc) / RTC_TIME_KEEPING_FREQUENCY_HZ + (overflows_occured * SECONDS_PER_OVERFLOW);
    CRITICAL_REGION_EXIT();

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_set_alarm(uint32_t seconds_time)
{
    if (!seconds_time)
        seconds_time = 1; // Enforce a minimum alarm time of 1 second

    uint32_t compare_value = nrfx_rtc_counter_get(&RTC_Inits[RTC_TIME_KEEPING].rtc) + seconds_time * RTC_TIME_KEEPING_FREQUENCY_HZ;
    nrfx_rtc_cc_set(&RTC_Inits[RTC_TIME_KEEPING].rtc, 0, compare_value, true);

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_soft_watchdog_enable(unsigned int seconds, void (*callback)(unsigned int))
{
    const nrfx_rtc_config_t rtc_config =
    {
        .prescaler          = RTC_FREQ_TO_PRESCALER(RTC_TIME_KEEPING_FREQUENCY_HZ),
        .interrupt_priority = RTC_Inits[RTC_SOFT_WATCHDOG].irq_priority,
        .reliable           = 0,
        .tick_latency       = 0
    };

    soft_watchdog_timeout_s = seconds;
    soft_watchdog_callback = callback;

    nrfx_rtc_init(&RTC_Inits[RTC_SOFT_WATCHDOG].rtc, &rtc_config, rtc_soft_watchdog_event_handler);
    nrfx_rtc_enable(&RTC_Inits[RTC_SOFT_WATCHDOG].rtc);

    soft_watchdog_init = true;

    syshal_rtc_soft_watchdog_refresh();

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_soft_watchdog_running(bool *is_running)
{
    *is_running = soft_watchdog_init;

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_soft_watchdog_refresh(void)
{
    if (!soft_watchdog_init)
        return SYSHAL_RTC_ERROR_DEVICE;

    uint32_t compare_value = nrfx_rtc_counter_get(&RTC_Inits[RTC_SOFT_WATCHDOG].rtc) + soft_watchdog_timeout_s * RTC_SOFT_WATCHDOG_FREQUENCY_HZ;
    nrfx_rtc_cc_set(&RTC_Inits[RTC_SOFT_WATCHDOG].rtc, 0, compare_value, true);

    return SYSHAL_RTC_NO_ERROR;
}

/**
 * @brief      An interrupt called whenever a wakeup event has occured
 */
__attribute__((weak)) void syshal_rtc_wakeup_event(void)
{
    // NOTE: This should be overriden by the user if it is desired
}