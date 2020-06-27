/* syshal_rtc.c - HAL for RTC device
 *
 * Copyright (C) 2018 Arribada
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

#include "stm32f0xx_hal.h"
#include "syshal_rtc.h"
#include "debug.h"

#define YEAR_OFFSET (2018) // RTC can only handle 100 years so offset by 2000

// Software watchdog state
static void (*soft_watchdog_callback)(unsigned int) = NULL; // User callback
static int soft_watchdog_now = -1;  // Current timer -1=>not started
static int soft_watchdog_init = -1; // Init timer    -1=>not started

static RTC_HandleTypeDef rtc_handle;

// HAL to SYSHAL error code mapping table
static int hal_error_map[] =
{
    SYSHAL_RTC_NO_ERROR,
    SYSHAL_RTC_ERROR_DEVICE,
    SYSHAL_RTC_ERROR_BUSY,
    SYSHAL_RTC_ERROR_TIMEOUT,
};

int syshal_rtc_init(void)
{
    RTC_AlarmTypeDef alarm_handle;
    HAL_StatusTypeDef status;

    rtc_handle.Instance = RTC;
    rtc_handle.Init.HourFormat = RTC_HOURFORMAT_24;
    rtc_handle.Init.AsynchPrediv = 31;
    rtc_handle.Init.SynchPrediv = 1023; // Set to generate a subsecond update every 0.976ms 
    rtc_handle.Init.OutPut = RTC_OUTPUT_DISABLE;
    rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    rtc_handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    status = HAL_RTC_Init(&rtc_handle);

    if (hal_error_map[status] < 0)
        return hal_error_map[status];

    alarm_handle.AlarmTime.Hours = 0;
    alarm_handle.AlarmTime.Minutes = 0;
    alarm_handle.AlarmTime.Seconds = 0;
    alarm_handle.AlarmTime.SubSeconds = 0;
    alarm_handle.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    alarm_handle.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    alarm_handle.AlarmMask = RTC_ALARMMASK_ALL; // Interrupt every second
    alarm_handle.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    alarm_handle.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    alarm_handle.AlarmDateWeekDay = RTC_WEEKDAY_MONDAY; // Nonspecific
    alarm_handle.Alarm = RTC_ALARM_A;

    status = HAL_RTC_SetAlarm_IT(&rtc_handle, &alarm_handle, RTC_FORMAT_BIN);

    return hal_error_map[status];
}

int syshal_rtc_set_date_and_time(syshal_rtc_data_and_time_t date_time)
{
    HAL_StatusTypeDef status;

    RTC_DateTypeDef date;
    date.WeekDay = RTC_WEEKDAY_MONDAY; // Unused as we don't track the name of the day
    date.Year = date_time.year - YEAR_OFFSET;
    date.Month = date_time.month;
    date.Date = date_time.day;

    status = HAL_RTC_SetDate(&rtc_handle, &date, RTC_FORMAT_BIN);
    if (HAL_OK != status)
        return hal_error_map[status];

    RTC_TimeTypeDef time;
    time.Hours = date_time.hours;
    time.Minutes = date_time.minutes;
    time.Seconds = date_time.seconds;
    time.SubSeconds = 0;
    time.SecondFraction = 0;
    time.TimeFormat = RTC_HOURFORMAT12_AM;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_SET;

    status = HAL_RTC_SetTime(&rtc_handle, &time, RTC_FORMAT_BIN);

    return hal_error_map[status];
}

int syshal_rtc_get_date_and_time(syshal_rtc_data_and_time_t * date_time)
{
    HAL_StatusTypeDef status;

    // You must call HAL_RTC_GetDate() after HAL_RTC_GetTime() to unlock the values in
    // the higher-order calendar shadow registers to ensure consistency between the time
    // and date values. Reading RTC current time locks the values in calendar shadow
    // registers until Current date is read
    RTC_TimeTypeDef time;
    status = HAL_RTC_GetTime(&rtc_handle, &time, RTC_FORMAT_BIN);
    date_time->hours = time.Hours;
    date_time->minutes = time.Minutes;
    date_time->seconds = time.Seconds;
    date_time->milliseconds = 1000 * (time.SecondFraction - time.SubSeconds) / (time.SecondFraction + 1);

    RTC_DateTypeDef date;
    status = HAL_RTC_GetDate(&rtc_handle, &date, RTC_FORMAT_BIN);
    if (HAL_OK != status)
        return hal_error_map[status];

    date_time->year = date.Year + YEAR_OFFSET;
    date_time->month = date.Month;
    date_time->day = date.Date;

    return hal_error_map[status];
}

int syshal_rtc_soft_watchdog_enable(unsigned int seconds, void (*callback)(unsigned int))
{
    // The software watchdog timer is initialized as a count-down in seconds.  Once
    // it has started, it can not be disabled.
    soft_watchdog_callback = callback;
    soft_watchdog_now = soft_watchdog_init = (int)seconds;

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_soft_watchdog_refresh(void)
{
    // Restart the timer -- this should be called periodically to prevent triggering
    soft_watchdog_now = soft_watchdog_init;
    return SYSHAL_RTC_NO_ERROR;
}

void HAL_RTC_MspInit(RTC_HandleTypeDef * rtcHandle)
{
    if (rtcHandle->Instance == RTC)
    {
        __HAL_RCC_RTC_ENABLE();

        HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RTC_IRQn);
    }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef * rtcHandle)
{
    if (rtcHandle->Instance == RTC)
    {
        __HAL_RCC_RTC_DISABLE();

        HAL_NVIC_DisableIRQ(RTC_IRQn);
    }
}

void RTC_IRQHandler(void)
{
    register int sp asm("sp");

    HAL_RTC_AlarmIRQHandler(&rtc_handle);
    HAL_RTCEx_WakeUpTimerIRQHandler(&rtc_handle); // NOTE: is this needed?

    // The software watchdog is inactive if the counter is less than zero
    if (soft_watchdog_now > 0)
    {
        // Decrement counter and check for trigger condition
        soft_watchdog_now--;
        if (soft_watchdog_now == 0 && soft_watchdog_callback)
        {
            // FIXME: This is a hack and might fail with other gcc versions or indeed
            // if the compilation flags are changed.
            // This is the exact location of the LR register relative to SP on entry
            // which we pass back to the caller.
            soft_watchdog_callback(((unsigned int *)sp)[8]);
        }
    }
}
