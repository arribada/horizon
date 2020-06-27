/* syshal_rtc.h - HAL for RTC device
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

#ifndef _SYSHAL_RTC_H_
#define _SYSHAL_RTC_H_

#include <stdbool.h>
#include <stdint.h>

#define SYSHAL_RTC_NO_ERROR           ( 0)
#define SYSHAL_RTC_ERROR_DEVICE       (-1)
#define SYSHAL_RTC_ERROR_BUSY         (-2)
#define SYSHAL_RTC_ERROR_TIMEOUT      (-3)
#define SYSHAL_RTC_INVALID_PARAMETER  (-4)

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;

    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint16_t milliseconds;
} syshal_rtc_data_and_time_t;

int syshal_rtc_init(void);
int syshal_rtc_term(void);
int syshal_rtc_set_date_and_time(syshal_rtc_data_and_time_t date_time);
int syshal_rtc_get_date_and_time(syshal_rtc_data_and_time_t * date_time);
int syshal_rtc_get_timestamp(uint32_t * timestamp);
int syshal_rtc_get_uptime(uint32_t * uptime);
int syshal_rtc_stash_time(void);
int syshal_rtc_soft_watchdog_enable(unsigned int seconds, void (*callback)(unsigned int));
int syshal_rtc_soft_watchdog_running(bool *is_running);
int syshal_rtc_soft_watchdog_refresh(void);
int syshal_rtc_set_alarm(uint32_t seconds_time);
int syshal_rtc_date_time_to_timestamp(syshal_rtc_data_and_time_t data_and_time, uint32_t * timestamp);

void syshal_rtc_wakeup_event(void);

#endif /* _SYSHAL_RTC_H_ */
