/* syshal_timer.c - HAL for MCU timers
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

#include <stdbool.h>
#include "syshal_timer.h"
#include "syshal_rtc.h"
#include "debug.h"

#define MILLISECONDS_IN_A_DAY   (24 * 60 * 60 * 1000)

#define SECONDS_IN_A_DAY        (24 * 60 * 60)

#undef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#ifndef NULL
#define NULL (0)
#endif

typedef struct
{
    bool init;                // Is this timer init/used
    bool running;             // Is this timer running?
    syshal_timer_mode_t mode; // Is this timer a one-shot or continuous
    uint32_t start;           // A timestamp of when this timer was started
    uint32_t duration;        // How long this timer should run for before triggering (in seconds)
    void (*callback)(void);   // The function that is called when the timer is triggered
} syshal_timer_t;

static volatile syshal_timer_t timers_priv[SYSHAL_TIMER_NUMBER_OF_TIMERS];

int syshal_timer_init(timer_handle_t *handle, void (*callback)(void))
{
    // Look for the first free timer
    for (uint32_t i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
    {
        if (!timers_priv[i].init)
        {
            timers_priv[i].init = true;
            timers_priv[i].callback = callback;
            timers_priv[i].running = false;
            *handle = i; // Return a handle to this timer
            return SYSHAL_TIMER_NO_ERROR;
        }
    }

    DEBUG_PR_ERROR("Ran out of avaliable timer instances");
    return SYSHAL_TIMER_ERROR_NO_FREE_TIMER;
}

int syshal_timer_term(timer_handle_t handle)
{
    if (handle >= SYSHAL_TIMER_NUMBER_OF_TIMERS)
        return SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE;

    timers_priv[handle].init = false;
    timers_priv[handle].callback = NULL;
    timers_priv[handle].running = false;
    return SYSHAL_TIMER_NO_ERROR;
}

int syshal_timer_set(timer_handle_t handle, syshal_timer_mode_t mode, uint32_t seconds)
{
    if (handle >= SYSHAL_TIMER_NUMBER_OF_TIMERS)
        return SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE;

    if (!timers_priv[handle].init)
        return SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE;

    if (seconds == 0)
        seconds = 1; // Our minimum duration is 1 second

    timers_priv[handle].mode = mode;
    syshal_rtc_get_uptime((uint32_t *) &timers_priv[handle].start);
    timers_priv[handle].duration = seconds;
    timers_priv[handle].running = true;

    DEBUG_PR_SYS("%s(%lu, %d, %lu)", __FUNCTION__, handle, mode, seconds);

    return SYSHAL_TIMER_NO_ERROR;
}

int syshal_timer_reset(timer_handle_t handle)
{
    if (handle >= SYSHAL_TIMER_NUMBER_OF_TIMERS)
        return SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE;

    if (!timers_priv[handle].init)
        return SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE;

    syshal_rtc_get_uptime((uint32_t *) &timers_priv[handle].start);

    return SYSHAL_TIMER_NO_ERROR;
}

int syshal_timer_running(timer_handle_t handle)
{
    if (handle >= SYSHAL_TIMER_NUMBER_OF_TIMERS)
        return SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE;

    if (!timers_priv[handle].init)
        return SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE;

    if (timers_priv[handle].running)
        return SYSHAL_TIMER_RUNNING;
    else
        return SYSHAL_TIMER_NOT_RUNNING;
}

int syshal_timer_cancel(timer_handle_t handle)
{
    if (handle >= SYSHAL_TIMER_NUMBER_OF_TIMERS)
        return SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE;

    if (!timers_priv[handle].init)
        return SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE;

#ifndef DEBUG_DISABLED
    if (timers_priv[handle].running)
    {
        DEBUG_PR_SYS("Cancelling timer: %lu", handle);
    }
#endif

    timers_priv[handle].running = false;

    return SYSHAL_TIMER_NO_ERROR;
}

int syshal_timer_cancel_all(void)
{
    for (uint32_t i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
        syshal_timer_cancel(i);

    return SYSHAL_TIMER_NO_ERROR;
}

void syshal_timer_tick(void)
{
    uint32_t current_time_seconds;
    syshal_rtc_get_uptime(&current_time_seconds);

    for (uint32_t i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
    {
        if (timers_priv[i].running)
        {
            if (current_time_seconds - timers_priv[i].start >= timers_priv[i].duration)
            {
                // If this is a periodic timer, then auto-reload it
                if (periodic == timers_priv[i].mode)
                    syshal_timer_set(i, timers_priv[i].mode, timers_priv[i].duration);
                else
                    syshal_timer_cancel(i); // Else cancel it

                if (timers_priv[i].callback)
                    timers_priv[i].callback(); // Call the callback function
            }
        }
    }
}

int syshal_timer_recalculate_next_alarm(void)
{
    uint32_t current_time_seconds;
    syshal_rtc_get_uptime(&current_time_seconds);

    /* Calculate when the next timer is due */
    uint32_t next_timer_due = UINT32_MAX;
    for (uint32_t i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
    {
        if (timers_priv[i].running)
        {
            if (timers_priv[i].start + timers_priv[i].duration <= current_time_seconds)
            {
                /* If the timer is due now then we must avoid the below arithmetic to prevent an underflow */
                next_timer_due = 0;
                break;
            }
            else
            {
                /* Compare the actual next timer event with the remaining time in the i timer */
                next_timer_due = MIN(timers_priv[i].start + timers_priv[i].duration - current_time_seconds, next_timer_due);
            }
        }
    }

    /* Set next rtc alarm using the minimum next timer event */
    if (next_timer_due != UINT32_MAX)
        syshal_rtc_set_alarm(next_timer_due);
    
    return SYSHAL_TIMER_NO_ERROR;
}