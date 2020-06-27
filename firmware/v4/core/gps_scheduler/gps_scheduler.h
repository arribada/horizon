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

#ifndef _GPS_SCHEDULER_H_
#define _GPS_SCHEDULER_H_

#include <stdint.h>
#include <stdbool.h>
#include "sys_config.h"
#include "syshal_gps.h"

#define GPS_SCHEDULER_NO_ERROR            ( 0)
#define GPS_SCHEDULER_ERROR_INVALID_STATE (-1)
#define GPS_SCHEDULER_ERROR_INVALID_PARAM (-2)

typedef struct
{
    sys_config_gps_trigger_mode_t                              *trigger_mode;
    sys_config_gps_scheduled_acquisition_interval_t            *scheduled_acquisition_interval;
    sys_config_gps_maximum_acquisition_time_t                  *maximum_acquisition_time;
    sys_config_gps_scheduled_acquisition_no_fix_timeout_t      *scheduled_acquisition_no_fix_timeout;
    sys_config_gps_max_fixes_t                                 *max_fixes;
} gps_scheduler_init_t;

typedef enum
{
    GPS_SCHEDULER_TRIGGER_SALTWATER_SWITCH,
    GPS_SCHEDULER_TRIGGER_ACCEL,
    GPS_SCHEDULER_TRIGGER_REED_SWITCH
} gps_scheduler_trigger_t;

int gps_scheduler_init(gps_scheduler_init_t init);
int gps_scheduler_start(void);
int gps_scheduler_stop(void);
int gps_scheduler_set_interval(uint32_t interval);
int gps_scheduler_trigger(gps_scheduler_trigger_t trigger, bool active);

void gps_scheduler_callback(const syshal_gps_event_t * event);

#endif /* _GPS_SCHEDULER_H_ */
