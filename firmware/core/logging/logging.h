/* logging.h - Logging defines
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

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdint.h>
#include <stddef.h>

#define LOGGING_NO_ERROR          ( 0)
#define LOGGING_ERROR_INVALID_TAG (-1)

#define LOGGING_MAX_SIZE sizeof(logging_struct_union_t)
#define LOGGING_COUNT    sizeof(logging_struct_t)

#define LOGGING_SET_HDR(p, i)  \
    (p)->h.id   = i;

// The technique used below is referred to as an X Macro
#define LOGGING_VALUES(X) \
    /* ID       TAG NAME                  TAG DATA STRUCT */ \
    X(0x7E, LOGGING_LOG_START,          logging_log_start_t)         /* Used to synchronize to the start of a log entry */ \
    X(0x7F, LOGGING_LOG_END,            logging_log_end_t)           /* Used to synchronize to the end of a log entry, including an 8-bit parity checksum */ \
    X(0x00, LOGGING_GPS_POSITION,       logging_gps_position_t)      /* GPS location */ \
    X(0x01, LOGGING_GPS_TTFF,           logging_gps_ttff_t)          /* GPS time to first fix */ \
    X(0x02, LOGGING_PRESSURE,           logging_pressure_t)          /* Pressure sensor reading */ \
    X(0x03, LOGGING_AXL_XYZ,            logging_axl_xyz_t)           /* Accelerometer X/Y/Z reading */ \
    X(0x04, LOGGING_DATE_TIME,          logging_date_time_t)         /* The date and time retrieved from the RTC */ \
    X(0x05, LOGGING_HRT,                logging_hrt_t)               /* High resolution timer */ \
    X(0x06, LOGGING_TEMPERATURE,        logging_temperature_t)       \
    X(0x07, LOGGING_SURFACED,           logging_surfaced_t)          /* Saltwater switch opened event */ \
    X(0x08, LOGGING_SUBMERGED,          logging_submerged_t)         /* Saltwater switch closed event */ \
    X(0x09, LOGGING_BATTERY,            logging_battery_t)           /* Battery charge state */ \
    X(0x0A, LOGGING_BLE_ENABLED,        logging_ble_enabled_t)       /* Bluetooth function has been enabled */ \
    X(0x0B, LOGGING_BLE_DISABLED,       logging_ble_disabled_t)      /* Bluetooth function has been disabled */ \
    X(0x0C, LOGGING_BLE_CONNECTED,      logging_ble_connected_t)     /* Bluetooth GATT connection has been made */ \
    X(0x0D, LOGGING_BLE_DISCONNECTED,   logging_ble_disconnected_t)  /* Bluetooth GATT connection has been released */ \
    X(0x0E, LOGGING_GPS_ON,             logging_log_gps_on_t)        /* GPS module turned on */ \
    X(0x0F, LOGGING_GPS_OFF,            logging_log_gps_off_t)       /* GPS module turned off */ \
    X(0x10, LOGGING_SOFT_WDOG,          logging_soft_watchdog_t)     /* Software WDOG event */ \
    X(0x11, LOGGING_STARTUP,            logging_startup_t)           /* Start-up event */ \
    X(0x12, LOGGING_TIME,               logging_time_t)              /* Simple timestamp */ \
    X(0x13, LOGGING_BATTERY_VOLTAGE,    logging_battery_voltage_t)   /* The battery voltage */ \
    X(0x20, LOGGING_IOT_STATUS,         logging_iot_status_t)        /* The current IOT state machine status */ \
    X(0x21, LOGGING_IOT_CONFIG_UPDATE,  logging_iot_config_update_t) /* Configuration file received */ \
    X(0x22, LOGGING_IOT_FW_UPDATE,      logging_iot_fw_update_t)     /* Firmware update file received */ \
    X(0x23, LOGGING_IOT_REPORT_ERROR,   logging_iot_error_report_t)  /* IOT error condition */          \
    X(0x24, LOGGING_IOT_NETWORK_INFO,   logging_iot_network_info_t)  /* Cellular network info condition */ \
    X(0x25, LOGGING_IOT_NEXT_PREPAS,   logging_iot_next_prepas_t)  /* next satellite prediction */

// Macros for manipulating the x macro table above
#define LOGGING_EXPAND_AS_ENUMS(id, name, struct)   name = (id),
#define LOGGING_EXPAND_AS_BUFFERS(id, name, struct) uint8_t name##_buf[sizeof(struct)];
#define LOGGING_EXPAND_AS_STRUCT(id, name, struct)  uint8_t name;
#define LOGGING_EXPAND_AS_LOOKUP(id, name, struct)  (id),

// Create an enum for each logging entry
enum
{
    LOGGING_VALUES(LOGGING_EXPAND_AS_ENUMS)
};

// Generate a lookup table of tag ids for use in unit tests
#ifdef GTEST
static const uint8_t logging_tag_lookup[] =
{
    LOGGING_VALUES(LOGGING_EXPAND_AS_LOOKUP)
};
#endif

#define LOGGING_BLE_ENABLED_CAUSE_REED_SWITCH          (0x00)
#define LOGGING_BLE_ENABLED_CAUSE_SCHEDULE_TIMER       (0x01)
#define LOGGING_BLE_ENABLED_CAUSE_GEOFENCE             (0x02)
#define LOGGING_BLE_ENABLED_CAUSE_ONE_SHOT             (0x03)

#define LOGGING_BLE_DISABLED_CAUSE_REED_SWITCH         (0x00)
#define LOGGING_BLE_DISABLED_CAUSE_SCHEDULE_TIMER      (0x01)
#define LOGGING_BLE_DISABLED_CAUSE_GEOFENCE            (0x02)
#define LOGGING_BLE_DISABLED_CAUSE_INACTIVITY_TIMEOUT  (0x03)

#define LOGGING_IOT_STATUS_CELLULAR_POWERED_OFF             ( 0)
#define LOGGING_IOT_STATUS_CELLULAR_POWERED_ON              ( 1)
#define LOGGING_IOT_STATUS_CELLULAR_CONNECT                 ( 2)
#define LOGGING_IOT_STATUS_CELLULAR_FETCH_DEVICE_SHADOW     ( 3)
#define LOGGING_IOT_STATUS_CELLULAR_SEND_LOGGING            ( 4)
#define LOGGING_IOT_STATUS_CELLULAR_SEND_DEVICE_STATUS      ( 5)
#define LOGGING_IOT_STATUS_CELLULAR_MAX_BACKOFF_REACHED     ( 6)
#define LOGGING_IOT_STATUS_CELLULAR_DOWNLOAD_FIRMWARE_FILE  ( 7)
#define LOGGING_IOT_STATUS_CELLULAR_DOWNLOAD_CONFIG_FILE    ( 8)
#define LOGGING_IOT_STATUS_SATELLITE_POWERED_OFF            ( 9)
#define LOGGING_IOT_STATUS_SATELLITE_POWERED_ON             (10)
#define LOGGING_IOT_STATUS_SATELLITE_SEND_DEVICE_STATUS     (11)

typedef struct __attribute__((__packed__))
{
    uint8_t id; // Tag ID
} logging_hdr_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
} logging_log_start_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
} logging_log_end_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint32_t iTOW;   // Time since navigation epoch in ms
    int32_t lon;     // Longitude (10^-7)
    int32_t lat;     // Latitude (10^-7)
    int32_t height;  // Height in mm
    uint32_t hAcc;   // Horizontal accuracy estimate
    uint32_t vAcc;   // Vertical accuracy estimate
} logging_gps_position_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint32_t ttff; // GPS time to first fix
} logging_gps_ttff_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    int32_t pressure; // Pressure sensor reading
} logging_pressure_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    int16_t x; // X axis
    int16_t y; // Y axis
    int16_t z; // Z axis
} logging_axl_xyz_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} logging_date_time_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint64_t us; // High resolution timer in microseconds
} logging_hrt_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    int32_t temperature;
} logging_temperature_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
} logging_surfaced_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
} logging_submerged_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint8_t charge;
} logging_battery_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint8_t cause;
} logging_ble_enabled_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint8_t cause;
} logging_ble_disabled_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
} logging_ble_connected_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
} logging_ble_disconnected_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
} logging_log_gps_on_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
} logging_log_gps_off_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint32_t watchdog_address;
} logging_soft_watchdog_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint32_t cause;
} logging_startup_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint32_t time;
} logging_time_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint16_t millivolts;
} logging_battery_voltage_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint8_t status;
} logging_iot_status_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint32_t version;
    uint32_t length;
} logging_iot_config_update_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint32_t version;
    uint32_t length;
} logging_iot_fw_update_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    int16_t iot_error_code;
    int16_t hal_error_code;
    uint16_t hal_line_number;
    uint16_t vendor_error_code;
} logging_iot_error_report_t;

typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint8_t signal_power;
    uint8_t quality;
    uint8_t technology;
    uint8_t network_operator[25];
    uint8_t local_area_code[5];
    uint8_t cell_id[9];
} logging_iot_network_info_t;


typedef struct __attribute__((__packed__))
{
    logging_hdr_t h;
    uint32_t prepas_result;
    uint32_t gps_timestamp; 
}logging_iot_next_prepas_t;

/* Functions */
int logging_tag_size(uint8_t tag_id, size_t * size);

// Put all the logging structs into a union typedef
// This is useful as the size of this will be the maximum size
typedef union __attribute__((__packed__))
{
    LOGGING_VALUES(LOGGING_EXPAND_AS_BUFFERS)
} logging_struct_union_t;

typedef struct __attribute__((__packed__))
{
    LOGGING_VALUES(LOGGING_EXPAND_AS_STRUCT)
} logging_struct_t;

#endif /* _LOGGING_H_ */
