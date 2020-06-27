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

#include <time.h>
#include "syshal_rtc.h"

// Leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y) ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )

#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
#define SECS_PER_WEEK (SECS_PER_DAY * 7UL)
#define SECS_PER_YEAR (SECS_PER_DAY * 365UL) // For a non-leap year

#define TIME_ACC 30

static const uint8_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int syshal_rtc_set_date_and_time(syshal_rtc_data_and_time_t date_time)
{
    printf("APPLICATION TRIED TO SET SYSTEM TIME!\r\n");
    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_date_and_time(syshal_rtc_data_and_time_t * date_time)
{
    struct tm * timeinfo;

    time_t now;
    struct tm *now_tm;

    now = time(NULL) * TIME_ACC;
    timeinfo = localtime(&now);

    date_time->year = timeinfo->tm_year;
    date_time->month = timeinfo->tm_mon + 1;
    date_time->day = timeinfo->tm_mday;
    date_time->hours = timeinfo->tm_hour;
    date_time->minutes = timeinfo->tm_min;
    date_time->seconds = timeinfo->tm_sec;
    date_time->milliseconds = 0;

    return SYSHAL_RTC_NO_ERROR;
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

int syshal_rtc_get_timestamp(uint32_t * timestamp)
{
    *timestamp = (uint32_t) time(NULL) *TIME_ACC ;
    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_uptime(uint32_t * uptime)
{
    syshal_rtc_get_timestamp(uptime);
    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_set_alarm(uint32_t seconds_time)
{
    ((void)(seconds_time)); 
    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_soft_watchdog_refresh(void)
{
    return SYSHAL_RTC_NO_ERROR; 
}