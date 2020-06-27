/* cmd.h - Configuration interface commands
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

#ifndef _CMD_H_
#define _CMD_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "syshal_device.h"

#define CMD_SYNCWORD        (0x7E)

#define CMD_SIZE_HDR        (sizeof(cmd_hdr_t))
#define CMD_MIN_SIZE        (CMD_SIZE_HDR)
#define CMD_MAX_SIZE        (sizeof(cmd_t))
#define CMD_MAX_PAYLOAD     (CMD_MAX_SIZE - sizeof(cmd_hdr_t))

#define CMD_CFG_TAG_ALL (0xFFFF) // A special tag to denote a read of all configuration values from RAM

#define CMD_SET_HDR(p, i)  \
    p->hdr.sync  = CMD_SYNCWORD; \
    p->hdr.cmd   = i;

#define CMD_SIZE(i)  (sizeof(cmd_hdr_t) + sizeof(i))

#define CFG_READ_REQ_READ_ALL (0xFFFF) // Read all configuration tags
#define CFG_ERASE_REQ_ERASE_ALL (CFG_READ_REQ_READ_ALL) // Erase all configuration tags

#define FW_SEND_IMAGE_REQ_ARTIC (2)

#define RESET_REQ_APP             (0) // Reset the Application device
#define RESET_REQ_FLASH_ERASE_ALL (1)
#define RESET_REQ_ENTER_DFU_MODE  (2)

#define CMD_TEST_REQ_GPS_FLAG        (1 << 0)
#define CMD_TEST_REQ_CELLULAR_FLAG   (1 << 1)
#define CMD_TEST_REQ_SATELLITE_FLAG  (1 << 2)

typedef struct __attribute__((__packed__))
{
    uint8_t sync; // Start of command synchronization byte
    uint8_t cmd;
} cmd_hdr_t;

enum
{
    CMD_LOG_CREATE_REQ_MODE_FILL,
    CMD_LOG_CREATE_REQ_MODE_CIRCULAR
};

// The technique used below is referred to as an X Macro
#define CMD_VALUES(X) \
    /*      ID                             DATA STRUCT                         NAME */ \
    X(CMD_GENERIC_RESP,                cmd_generic_resp_t,                cmd_generic_resp)                /* Generic response message sent where only an error response is needed */ \
    \
    /* Configuration */ \
    X(CMD_CFG_READ_REQ,                cmd_cfg_read_req_t,                cmd_cfg_read_req)                /* Read single configuration tag or all configuration tags */ \
    X(CMD_CFG_WRITE_REQ,               cmd_cfg_write_req_t,               cmd_cfg_write_req)               /* Write new configuration items as a series of tag/value pairs */ \
    X(CMD_CFG_SAVE_REQ,                cmd_generic_req_t,                 cmd_cfg_save_req)                /* Save all current configuration settings to flash memory */ \
    X(CMD_CFG_RESTORE_REQ,             cmd_generic_req_t,                 cmd_cfg_restore_req)             /* Restore all current configuration settings from flash memory */ \
    X(CMD_CFG_ERASE_REQ,               cmd_cfg_erase_req_t,               cmd_cfg_erase_req)               /* Erase a single or all configuration tags in flash memory */ \
    X(CMD_CFG_PROTECT_REQ,             cmd_generic_req_t,                 cmd_cfg_protect_req)             /* Protect the configuration file in flash memory */ \
    X(CMD_CFG_UNPROTECT_REQ,           cmd_generic_req_t,                 cmd_cfg_unprotect_req)           /* Unprotect the configuration file in flash memory */ \
    X(CMD_CFG_READ_RESP,               cmd_cfg_read_resp_t,               cmd_cfg_read_resp)               \
    X(CMD_CFG_WRITE_CNF,               cmd_cfg_write_cnf_t,               cmd_cfg_write_cnf)               /* Confirmation message sent after the entire CFG_WRITE_REQ payload has been received */ \
    \
    /* GPS Bridging */ \
    X(CMD_GPS_WRITE_REQ,               cmd_gps_write_req_t,               cmd_gps_write_req)               /* Send UBX commands directly to the GPS module */ \
    X(CMD_GPS_READ_REQ,                cmd_gps_read_req_t,                cmd_gps_read_req)                /* Receive UBX command responses directly from the GPS module */ \
    X(CMD_GPS_READ_RESP,               cmd_gps_read_resp_t,               cmd_gps_read_resp)               /* The response from the GPS module */ \
    X(CMD_GPS_CONFIG_REQ,              cmd_gps_config_req_t,              cmd_gps_config_req)              /* Allow a GPS IRQ events to be generated and sent over the USB interrupt endpoint.  This shall be used to indicate that data is available to be read from the internal FIFO */ \
    \
    /* BLE Bridging */ \
    X(CMD_BLE_CONFIG_REQ,              cmd_ble_config_req_t,              cmd_ble_config_req)              /* Allow BLE IRQ events to be generated and sent over the USB interrupt endpoint */ \
    X(CMD_BLE_WRITE_REQ,               cmd_ble_write_req_t,               cmd_ble_write_req)               /* Initiate a write to the BLE module at Address with data of Length */ \
    X(CMD_BLE_READ_REQ,                cmd_ble_read_req_t,                cmd_ble_read_req)                /* Initiate a read from the BLE module from Address for data of Length */ \
    \
    /* System */ \
    X(CMD_STATUS_REQ,                  cmd_generic_req_t,                 cmd_status_req)                  /* Request firmware status */ \
    X(CMD_STATUS_RESP,                 cmd_status_resp_t,                 cmd_status_resp)                 /* Firmware status response */ \
    X(CMD_FW_SEND_IMAGE_REQ,           cmd_fw_send_image_req_t,           cmd_fw_send_image_req)           /* Request to send a new firmware image and store temporarily in local flash memory */ \
    X(CMD_FW_SEND_IMAGE_COMPLETE_CNF,  cmd_fw_send_image_complete_cnf_t,  cmd_fw_send_image_complete_cnf)  /* This shall be sent by the server to the client to indicate all bytes have been received and stored */ \
    X(CMD_FW_APPLY_IMAGE_REQ,          cmd_fw_apply_image_req_t,          cmd_fw_apply_image_req)          /* Request to apply an existing firmware image in temporary storage to the target */ \
    X(CMD_RESET_REQ,                   cmd_reset_req_t,                   cmd_reset_req)                   /* Request to reset the system */ \
    \
    /* Configuration */ \
    X(CMD_BATTERY_STATUS_REQ,          cmd_generic_req_t,                 cmd_battery_status_req)          /* Request battery status */ \
    X(CMD_BATTERY_STATUS_RESP,         cmd_battery_status_resp_t,         cmd_battery_status_resp)         /* Error response carrying the battery status information */ \
    \
    /* Logging */ \
    X(CMD_LOG_CREATE_REQ,              cmd_log_create_req_t,              cmd_log_create_req)              /* Request to create a log file of the specified operation mode and length */ \
    X(CMD_LOG_ERASE_REQ,               cmd_generic_req_t,                 cmd_log_erase_req)               /* Request to erase the current log file */ \
    X(CMD_LOG_READ_REQ,                cmd_log_read_req_t,                cmd_log_read_req)                /* Read the log file from the starting offset for the given number of bytes */ \
    X(CMD_LOG_READ_RESP,               cmd_log_read_resp_t,               cmd_log_read_resp)               /* Response for a read request */ \
    \
    /* Cellular Bridging */ \
    X(CMD_CELLULAR_CONFIG_REQ,         cmd_cellular_config_req_t,         cmd_cellular_config_req)         /* Set Cellular bridging mode as enabled or disabled. */ \
    X(CMD_CELLULAR_WRITE_REQ,          cmd_cellular_write_req_t,          cmd_cellular_write_req)          /* Send AT commands directly to the cellular module. Bytes may be transferred raw after a GENERIC_RESP has been sent by the device */ \
    X(CMD_CELLULAR_READ_REQ,           cmd_cellular_read_req_t,           cmd_cellular_read_req)           /* Receive AT command responses directly from the cellular module. Bytes may be transferred raw after a CELLULAR_READ_RESP has been sent by the device. */ \
    X(CMD_CELLULAR_READ_RESP,          cmd_cellular_read_resp_t,          cmd_cellular_read_resp)          /* The length field shall indicate the number of bytes to be transferred.  This may differ to the length requested. */ \
    \
    /* Testing */ \
    X(CMD_TEST_REQ,                    cmd_test_req_t,                    cmd_test_req)                    /* Set Cellular Test mode enabled or disabled. each test can be set independently GPS, CELLULAR or Satellite */ \
    \
    /* Debugging */ \
    X(CMD_FLASH_DOWNLOAD_REQ,          cmd_generic_req_t,                 cmd_flash_download_req)          /* Download the entire contents of the SPI FLASH device */ \
    X(CMD_FLASH_DOWNLOAD_RESP,         cmd_flash_download_resp_t,         cmd_flash_download_resp)         \

// Macros for manipulating the x macro table above
#define CMD_EXPAND_AS_ENUMS(id, struct, name)   id,
#define CMD_EXPAND_AS_UNION(id, struct, name)   struct name;

// Error codes
typedef enum
{
    CMD_NO_ERROR,                    // Successful completion of the command.
    CMD_ERROR_FILE_NOT_FOUND,        // File associated with the operation could not be found.
    CMD_ERROR_FILE_ALREADY_EXISTS,   // Unable to create a file that already exists.
    CMD_ERROR_INVALID_CONFIG_TAG,    // Invalid configuration tag found in the tag stream.
    CMD_ERROR_GPS_COMMS,             // GPS module communications error e.g., attempt to do a GPS read/write when not bridging.
    CMD_ERROR_TIMEOUT,               // A timeout happened waiting on the byte stream to be received.
    CMD_ERROR_CONFIG_PROTECTED,      // Configuration operation not permitted as it is protected.
    CMD_ERROR_CONFIG_TAG_NOT_SET,    // Configuration tag has not been set.
    CMD_ERROR_BRIDGING_DISABLED,     // Bridging is currently disabled for this module/device
    CMD_ERROR_DATA_OVERSIZE,         // We've received more data then we were expecting
    CMD_ERROR_INVALID_PARAMETER,     // An invalid parameter has been provided
    CMD_ERROR_INVALID_FW_IMAGE_TYPE, // An invalid image type was received in a CMD_FW_SEND_IMAGE_REQ
    CMD_ERROR_IMAGE_CRC_MISMATCH,    // The firmware images CRC does not match one in flash
    CMD_ERROR_FILE_INCOMPATIBLE,     // This file is incompatible with this firmware version
    CMD_ERROR_CELLULAR_COMMS,        // Cellullar module communications error e.g., attempt to do a Cellular read/write when not bridging.
} cmd_error_t;

// Generic response message
typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
} cmd_generic_resp_t;

// Generic request message
typedef struct __attribute__((__packed__))
{
} cmd_generic_req_t;

///////////////// Configuration /////////////////
typedef struct __attribute__((__packed__))
{
    uint16_t configuration_tag;
} cmd_cfg_read_req_t;

typedef struct __attribute__((__packed__))
{
    uint32_t length;
} cmd_cfg_write_req_t;

typedef struct __attribute__((__packed__))
{
    uint16_t configuration_tag;
} cmd_cfg_erase_req_t;

typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
    uint32_t length;
} cmd_cfg_read_resp_t;

typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
} cmd_cfg_write_cnf_t;

////////////////// GPS Bridge ///////////////////
typedef struct __attribute__((__packed__))
{
    uint32_t length;
} cmd_gps_write_req_t;

typedef struct __attribute__((__packed__))
{
    uint32_t length;
} cmd_gps_read_req_t;

typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
    uint32_t length;
} cmd_gps_read_resp_t;

typedef struct __attribute__((__packed__))
{
    uint8_t enable;
} cmd_gps_config_req_t;

////////////////// BLE Bridge ///////////////////
typedef struct __attribute__((__packed__))
{
    uint8_t enable;
} cmd_ble_config_req_t;

typedef struct __attribute__((__packed__))
{
    uint8_t address;
    uint16_t length;
} cmd_ble_write_req_t;

typedef struct __attribute__((__packed__))
{
    uint8_t address;
    uint16_t length;
} cmd_ble_read_req_t;

//////////////////// System /////////////////////
typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
    uint32_t app_firmware_version;
    uint32_t ble_firmware_version;
    uint32_t configuration_format_version;
    device_id_t mcu_uid;
    uint8_t gps_module_detected;
    uint8_t cellular_module_detected;
    uint8_t sim_card_present;
    uint8_t sim_card_imsi[16];
    uint8_t satellite_module_detected;
} cmd_status_resp_t;

typedef struct __attribute__((__packed__))
{
    uint8_t image_type;
    uint32_t length;
    uint32_t CRC32;
} cmd_fw_send_image_req_t;

typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
} cmd_fw_send_image_complete_cnf_t;

typedef struct __attribute__((__packed__))
{
    uint8_t image_type;
} cmd_fw_apply_image_req_t;

typedef struct __attribute__((__packed__))
{
    uint8_t reset_type;
} cmd_reset_req_t;

//////////////////// Battery /////////////////////
typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
    uint8_t charging_indicator;
    uint8_t charge_level; // Charge level in percent
    uint16_t millivolts;
} cmd_battery_status_resp_t;

//////////////////// Logging /////////////////////
typedef struct __attribute__((__packed__))
{
    uint8_t mode;
    uint8_t sync_enable;
} cmd_log_create_req_t;

typedef struct __attribute__((__packed__))
{
    uint32_t start_offset;
    uint32_t length;
} cmd_log_read_req_t;

typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
    uint32_t length;
} cmd_log_read_resp_t;


////////////////// Cellular Bridge ////////////////
typedef struct __attribute__((__packed__))
{
    uint32_t length;
} cmd_cellular_write_req_t;

typedef struct __attribute__((__packed__))
{
    uint32_t length;
} cmd_cellular_read_req_t;

typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
    uint32_t length;
} cmd_cellular_read_resp_t;

typedef struct __attribute__((__packed__))
{
    uint8_t enable;
} cmd_cellular_config_req_t;

typedef struct __attribute__((__packed__))
{
    uint8_t test_device_flag;
} cmd_test_req_t;

////////////////////// Debug //////////////////////
typedef struct __attribute__((__packed__))
{
    uint8_t error_code;
    uint32_t length;
} cmd_flash_download_resp_t;


typedef enum
{
    CMD_VALUES(CMD_EXPAND_AS_ENUMS)
} cmd_id_t;

typedef struct __attribute__((__packed__))
{
    cmd_hdr_t hdr;
    union
    {
        CMD_VALUES(CMD_EXPAND_AS_UNION)
    };
} cmd_t;

// Exposed functions
int cmd_get_size(cmd_id_t command, size_t * size);
bool cmd_check_size(cmd_id_t command, size_t size);

#endif /* _CMD_H_ */
