/* config_if.c - Configuration interface abstraction layer. This is used
 * to homogenise the USB and BLE syshals for seamless switching between them
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

#include "bsp.h"
#include "config_if.h"
#include "syshal_ble.h"
#include "syshal_usb.h"
#include "fs_script.h"
#include "debug.h"

static int (*config_if_func_send_priv)(uint8_t *, uint32_t);
static int (*config_if_func_receive_priv)(uint8_t *, uint32_t);
static int (*config_if_func_receive_byte_stream_priv)(uint8_t *, uint32_t);
static int (*config_if_func_tick_priv)(void);

static config_if_backend_t backend_priv = {.id = CONFIG_IF_BACKEND_NOT_SET};

// Constants
int config_if_init(config_if_backend_t backend)
{
    if (backend_priv.id == backend.id)
        return CONFIG_IF_ERROR_ALREADY_CONFIGURED;

    switch (backend.id)
    {
        case CONFIG_IF_BACKEND_USB:
            config_if_func_send_priv = &syshal_usb_send;
            config_if_func_receive_priv = &syshal_usb_receive;
            config_if_func_receive_byte_stream_priv = &syshal_usb_receive;
            config_if_func_tick_priv = &syshal_usb_tick;
            syshal_usb_init();
            break;

        case CONFIG_IF_BACKEND_BLE:
            config_if_func_send_priv = &syshal_ble_send;
            config_if_func_receive_priv = &syshal_ble_receive;
            config_if_func_receive_byte_stream_priv = &syshal_ble_receive;
            config_if_func_tick_priv = &syshal_ble_tick;
            syshal_ble_init(backend.ble.config);
            break;

        case CONFIG_IF_BACKEND_FS_SCRIPT:
            config_if_func_send_priv = &fs_script_send;
            config_if_func_receive_priv = &fs_script_receive;
            config_if_func_receive_byte_stream_priv = &fs_script_receive_byte_stream;
            config_if_func_tick_priv = NULL;
            fs_script_init(backend.fs_script.filesystem, backend.fs_script.file_id);
            break;

        default:
            return CONFIG_IF_ERROR_INVALID_INSTANCE;
            break;
    }

    backend_priv = backend;
    return CONFIG_IF_NO_ERROR;
}

config_if_backend_id_t config_if_current(void)
{
    return backend_priv.id;
}

int config_if_term(void)
{
    config_if_func_send_priv = NULL;
    config_if_func_receive_priv = NULL;
    config_if_func_tick_priv = NULL;

    if (backend_priv.id == CONFIG_IF_BACKEND_USB)
        syshal_usb_term();

    if (backend_priv.id == CONFIG_IF_BACKEND_BLE)
        syshal_ble_term();

    if (backend_priv.id == CONFIG_IF_BACKEND_FS_SCRIPT)
        fs_script_term();

    backend_priv.id = CONFIG_IF_BACKEND_NOT_SET;

    return CONFIG_IF_NO_ERROR;
}

int config_if_send(uint8_t * data, uint32_t size)
{
    if (config_if_func_send_priv == NULL)
        return CONFIG_IF_ERROR_INVALID_INSTANCE;

    return config_if_func_send_priv(data, size);
}

int config_if_receive(uint8_t * data, uint32_t size)
{
    if (config_if_func_receive_priv == NULL)
        return CONFIG_IF_ERROR_INVALID_INSTANCE;

    return config_if_func_receive_priv(data, size);
}

int config_if_receive_byte_stream(uint8_t * data, uint32_t size)
{
    if (config_if_func_receive_priv == NULL)
        return CONFIG_IF_ERROR_INVALID_INSTANCE;

    return config_if_func_receive_byte_stream_priv(data, size);
}

/**
 * @brief      This function is called whenever a event occurs on the configured
 *             backend. This should be the user's application code to handle
 *             communication events
 *
 * @param[out] event  The event
 *
 * @return     Return error code
 */
__attribute__((weak)) int config_if_callback(config_if_event_t * event)
{
    ((void)(event)); // Remove unused variable compiler warning
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);

    return CONFIG_IF_NO_ERROR;
}

/**
 * @brief      Processes & updates the internal config_if states
 */
void config_if_tick(void)
{
    if (config_if_func_tick_priv != NULL)
        config_if_func_tick_priv();
}

/**
 * @brief      This function is called whenever an event occurs on the USB bus
 *
 * @param[out] event  The event
 *
 * @return     Return error code
 */
int syshal_usb_event_handler(syshal_usb_event_t * event)
{
    config_if_event_t parsed_event;
    parsed_event.backend = CONFIG_IF_BACKEND_USB;

    switch (event->id)
    {
        case SYSHAL_USB_EVENT_CONNECTED:
            parsed_event.id = CONFIG_IF_EVENT_CONNECTED;
            break;

        case SYSHAL_USB_EVENT_DISCONNECTED:
            parsed_event.id = CONFIG_IF_EVENT_DISCONNECTED;
            break;

        case SYSHAL_USB_EVENT_SEND_COMPLETE:
            parsed_event.id = CONFIG_IF_EVENT_SEND_COMPLETE;
            parsed_event.send.size = event->send.size;
            break;

        case SYSHAL_USB_EVENT_RECEIVE_COMPLETE:
            parsed_event.id = CONFIG_IF_EVENT_RECEIVE_COMPLETE;
            parsed_event.receive.size = event->receive.size;
            break;

        default:
            DEBUG_PR_ERROR("%s() Unknown event", __FUNCTION__);
            return SYSHAL_USB_ERROR_FAIL;
            break;
    }

    config_if_callback(&parsed_event);

    return SYSHAL_USB_NO_ERROR;
}

/**
 * @brief      This function is called whenever an event occurs on the BLE bus
 *
 * @param      event  The event
 */
int syshal_ble_event_handler(syshal_ble_event_t * event)
{
    config_if_event_t parsed_event;
    parsed_event.backend = CONFIG_IF_BACKEND_BLE;

    switch (event->id)
    {
        case SYSHAL_BLE_EVENT_CONNECTED:
            parsed_event.id = CONFIG_IF_EVENT_CONNECTED;
            break;

        case SYSHAL_BLE_EVENT_DISCONNECTED:
            parsed_event.id = CONFIG_IF_EVENT_DISCONNECTED;
            break;

        case SYSHAL_BLE_EVENT_SEND_COMPLETE:
            parsed_event.id = CONFIG_IF_EVENT_SEND_COMPLETE;
            parsed_event.send.size = event->send_complete.length;
            break;

        case SYSHAL_BLE_EVENT_RECEIVE_COMPLETE:
            parsed_event.id = CONFIG_IF_EVENT_RECEIVE_COMPLETE;
            parsed_event.receive.size = event->receive_complete.length;
            break;

        case SYSHAL_BLE_EVENT_FW_UPGRADE_COMPLETE:
            // FIXME: Implement
            DEBUG_PR_WARN("%s() SYSHAL_BLE_EVENT_FW_UPGRADE_COMPLETE Not implemented", __FUNCTION__);
            break;

        case SYSHAL_BLE_EVENT_ERROR_INDICATION:
            // FIXME: Implement
            DEBUG_PR_WARN("%s() SYSHAL_BLE_EVENT_ERROR_INDICATION Not implemented. Error: %d", __FUNCTION__, event->error);
            break;

        default:
            DEBUG_PR_ERROR("%s() Unknown event", __FUNCTION__);
            return SYSHAL_BLE_ERROR_FAIL;
            break;
    }

    config_if_callback(&parsed_event);

    return SYSHAL_BLE_NO_ERROR;
}

/**
 * @brief      This function is called whenever an event occurs in the fs_script backend
 *
 * @param      event  The event
 */
void fs_script_event_handler(fs_script_event_t * event)
{
    config_if_event_t parsed_event;
    parsed_event.backend = CONFIG_IF_BACKEND_FS_SCRIPT;

    switch (event->id)
    {
        case FS_SCRIPT_FILE_OPENED:
            parsed_event.id = CONFIG_IF_EVENT_CONNECTED;
            break;

        case FS_SCRIPT_FILE_CLOSED:
            parsed_event.id = CONFIG_IF_EVENT_DISCONNECTED;
            break;

        case FS_SCRIPT_SEND_COMPLETE:
            parsed_event.id = CONFIG_IF_EVENT_SEND_COMPLETE;
            parsed_event.send.size = event->send.size;
            break;

        case FS_SCRIPT_RECEIVE_COMPLETE:
            parsed_event.id = CONFIG_IF_EVENT_RECEIVE_COMPLETE;
            parsed_event.receive.size = event->receive.size;
            break;

        default:
            DEBUG_PR_ERROR("%s() Unknown event", __FUNCTION__);
            return;
            break;
    }

    config_if_callback(&parsed_event);
}