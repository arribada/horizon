/* gps_scheduler.h - GPS Schedule Handling
 *
 * Copyright (C) 2019 Arribada
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

#include "gps_scheduler.h"
#include "syshal_timer.h"
#include "syshal_gps.h"
#include "logging.h"
#include "debug.h"

// Timer handles
static timer_handle_t timer_interval;
static timer_handle_t timer_no_fix;
static timer_handle_t timer_maximum_acquisition;

// Timer callbacks
static void timer_interval_callback(void);
static void timer_no_fix_callback(void);
static void timer_maximum_acquisition_callback(void);

static gps_scheduler_init_t config;
static uint32_t positions_this_on_period;
static bool scheduler_running = false;

static void start_no_fix_timeout(void)
{
    // Are we supposed to be scheduling a no fix timer?
    // If our interval time is 0 this is a special case meaning run the GPS forever
    // If our no fix timeout is 0 as this is a special case meaning there is no no-fix timeout
    if (config.scheduled_acquisition_interval->hdr.set && config.scheduled_acquisition_interval->contents.seconds &&
        config.scheduled_acquisition_no_fix_timeout->hdr.set && config.scheduled_acquisition_no_fix_timeout->contents.seconds)
        syshal_timer_set(timer_no_fix, one_shot, config.scheduled_acquisition_no_fix_timeout->contents.seconds);
}

static void GPS_on(void)
{
    if (STATE_ASLEEP != syshal_gps_get_state())
        return; // GPS already awake

    positions_this_on_period = 0;

    syshal_gps_wake_up();
    start_no_fix_timeout();
}

static void GPS_off(void)
{
    if (STATE_ASLEEP == syshal_gps_get_state())
        return; // GPS already asleep

    syshal_gps_shutdown();
}

int gps_scheduler_init(gps_scheduler_init_t init)
{
    if (STATE_UNINIT == syshal_gps_get_state())
        return GPS_SCHEDULER_ERROR_INVALID_STATE;

    config = init;

    syshal_timer_init(&timer_interval, timer_interval_callback);
    syshal_timer_init(&timer_no_fix, timer_no_fix_callback);
    syshal_timer_init(&timer_maximum_acquisition, timer_maximum_acquisition_callback);

    return GPS_SCHEDULER_NO_ERROR;
}

int gps_scheduler_set_interval(uint32_t interval)
{
    if (STATE_UNINIT == syshal_gps_get_state())
        return GPS_SCHEDULER_ERROR_INVALID_STATE;

    if (interval && ( SYSHAL_TIMER_RUNNING == syshal_timer_running(timer_interval)) )
        syshal_timer_set(timer_interval, periodic, interval);

    return GPS_SCHEDULER_NO_ERROR;
}

int gps_scheduler_start(void)
{
    scheduler_running = true;

    GPS_off();

    if (!config.trigger_mode->hdr.set)
        return GPS_SCHEDULER_NO_ERROR;

    if (SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED_BITMASK & config.trigger_mode->contents.mode)
    {
        if (config.scheduled_acquisition_interval->hdr.set && config.scheduled_acquisition_interval->contents.seconds)
            syshal_timer_set(timer_interval, periodic, config.scheduled_acquisition_interval->contents.seconds);
        else
            GPS_on();

        return GPS_SCHEDULER_NO_ERROR;
    }

    return GPS_SCHEDULER_NO_ERROR;
}

int gps_scheduler_stop(void)
{
    scheduler_running = false;

    GPS_off();

    syshal_timer_cancel(timer_interval);
    syshal_timer_cancel(timer_no_fix);
    syshal_timer_cancel(timer_maximum_acquisition);

    return GPS_SCHEDULER_NO_ERROR;
}

int gps_scheduler_trigger(gps_scheduler_trigger_t trigger, bool active)
{
    if (!scheduler_running)
        return GPS_SCHEDULER_NO_ERROR;

    switch (trigger)
    {
        case GPS_SCHEDULER_TRIGGER_SALTWATER_SWITCH:
            if (!(SYS_CONFIG_GPS_TRIGGER_MODE_SALTWATER_SWITCH_TRIGGERED_BITMASK & config.trigger_mode->contents.mode))
                return GPS_SCHEDULER_NO_ERROR;
            break;
        case GPS_SCHEDULER_TRIGGER_ACCEL:
            if (!(SYS_CONFIG_GPS_TRIGGER_MODE_ACCEL_TRIGGERED_BITMASK & config.trigger_mode->contents.mode))
                return GPS_SCHEDULER_NO_ERROR;
            break;
        case GPS_SCHEDULER_TRIGGER_REED_SWITCH:
            if (!(SYS_CONFIG_GPS_TRIGGER_MODE_REED_TRIGGERED_BITMASK & config.trigger_mode->contents.mode))
                return GPS_SCHEDULER_NO_ERROR;
            break;
        default:
            break;
    }

    if (active)
    {
        GPS_on();

        if ((SYS_CONFIG_GPS_TRIGGER_MODE_SALTWATER_SWITCH_TRIGGERED_BITMASK & config.trigger_mode->contents.mode))
        {
            syshal_timer_cancel(timer_maximum_acquisition);
        }
        else
        {
            // If our maximum acquisition time is not set to forever (0)
            if (config.maximum_acquisition_time->hdr.set && config.maximum_acquisition_time->contents.seconds)
                syshal_timer_set(timer_maximum_acquisition, one_shot, config.maximum_acquisition_time->contents.seconds);
        }
    }
    else
    {
        syshal_timer_cancel(timer_maximum_acquisition);
        GPS_off();
    }

    return GPS_SCHEDULER_NO_ERROR;
}

static void timer_interval_callback(void)
{
    DEBUG_PR_TRACE("%s() called", __FUNCTION__);

    GPS_on();
    syshal_timer_set(timer_maximum_acquisition, one_shot, config.maximum_acquisition_time->contents.seconds);
}

static void timer_no_fix_callback(void)
{
    DEBUG_PR_TRACE("%s() called", __FUNCTION__);

    syshal_timer_cancel(timer_maximum_acquisition);
    // We have been unable to achieve a GPS fix
    // So shutdown the GPS
    GPS_off();
}

static void timer_maximum_acquisition_callback(void)
{
    DEBUG_PR_TRACE("%s() called", __FUNCTION__);

    syshal_timer_cancel(timer_no_fix);

    // We have been GPS logging for our maximum allowed time
    // So shutdown the GPS
    GPS_off();
}

void syshal_gps_callback(const syshal_gps_event_t * event)
{
    static syshal_gps_state_t last_gps_state = STATE_ASLEEP;

    if (scheduler_running)
    {
        switch (event->id)
        {
            case SYSHAL_GPS_EVENT_STATUS:

                if (STATE_FIXED == last_gps_state &&
                    STATE_ACQUIRING == syshal_gps_get_state())
                    start_no_fix_timeout();

                if (STATE_FIXED == syshal_gps_get_state())
                    syshal_timer_cancel(timer_no_fix);

                break;

            case SYSHAL_GPS_EVENT_PVT:

                if (STATE_FIXED == last_gps_state &&
                    STATE_ACQUIRING == syshal_gps_get_state())
                    start_no_fix_timeout();

                if (STATE_FIXED == syshal_gps_get_state())
                {
                    syshal_timer_cancel(timer_no_fix);
                    positions_this_on_period++;

                    // If we are only meant to get a certain number of fixes per connection then stop the GPS
                    if (config.max_fixes->hdr.set && config.max_fixes->contents.fixes)
                    {
                        if (positions_this_on_period >= config.max_fixes->contents.fixes)
                        {
                            GPS_off();
                            // Stop our maximum acquisition timer as this is now not needed
                            syshal_timer_cancel(timer_maximum_acquisition);
                        }
                    }
                }

                break;

            default:
                break;
        }
    }

    last_gps_state = syshal_gps_get_state();

    gps_scheduler_callback(event);
}

/**
 * @brief      GPS scheduler callback stub, should be overriden by the user application
 *
 * @param[in]  event  The event that occured
 */
__attribute__((weak)) void gps_scheduler_callback(const syshal_gps_event_t * event)
{
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);
}