/* syshal_gps.h - HAL for gps device
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

#ifndef _SYSHAL_GPS_H_
#define _SYSHAL_GPS_H_

#include <stdint.h>
#include <stdbool.h>
#include "syshal_rtc.h"

// Constants
#define SYSHAL_GPS_NO_ERROR               ( 0)
#define SYSHAL_GPS_ERROR_BUSY             (-1)
#define SYSHAL_GPS_ERROR_TIMEOUT          (-2)
#define SYSHAL_GPS_ERROR_DEVICE           (-3)

#define SYSHAL_GPS_LON_LAT_SCALER (10000000.0f)

typedef enum
{
    SYSHAL_GPS_EVENT_STATUS,
    SYSHAL_GPS_EVENT_PVT
} syshal_gps_event_id_t;

typedef struct
{
    uint32_t iTOW;
    uint8_t  gpsFix;
    uint8_t  flags;
    uint8_t  fixStat;
    uint8_t  flags2;
    uint32_t ttff;
    uint32_t msss;
} syshal_gps_event_status_t;

typedef struct
{
    syshal_rtc_data_and_time_t date_time; // The timestamp of this reading
    bool date_time_valid; // Is the timestamp given valid?
    uint32_t iTOW;   // GPS time of week of the navigation epoch
    uint8_t  gpsFix; // GPS fix information
    int32_t  lon;    // Longitude
    int32_t  lat;    // Latitude
    int32_t  hMSL;   // Height above mean sea level
    uint32_t hAcc;   // Horizontal accuracy estimate
    uint32_t vAcc;   // Vertical accuracy estimate
} syshal_gps_event_pvt_t;

typedef struct
{
    syshal_gps_event_id_t id;
    union
    {
        syshal_gps_event_status_t  status;
        syshal_gps_event_pvt_t     pvt;
    };
} syshal_gps_event_t;

int syshal_gps_init(void);
int syshal_gps_shutdown(void);
int syshal_gps_wake_up(void);
int syshal_gps_tick(void);
int syshal_gps_set_baud(uint32_t baudrate);

int syshal_gps_send_raw(uint8_t * data, uint32_t size);
int syshal_gps_receive_raw(uint8_t * data, uint32_t size);
uint32_t syshal_gps_available_raw(void);
bool syshal_gps_is_present(void);

void syshal_gps_callback(const syshal_gps_event_t * event);

#endif /* _SYSHAL_GPS_H_ */