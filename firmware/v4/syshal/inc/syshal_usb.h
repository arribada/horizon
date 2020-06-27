/* syshal_usb.h - HAL for USB
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

#ifndef _SYSHAL_USB_H_
#define _SYSHAL_USB_H_

#include <stdint.h>
#include <stdbool.h>

// Constants
#define SYSHAL_USB_NO_ERROR            ( 0)
#define SYSHAL_USB_ERROR_BUSY          (-1)
#define SYSHAL_USB_ERROR_FAIL          (-2)
#define SYSHAL_USB_ERROR_DISCONNECTED  (-3)
#define SYSHAL_USB_BUFFER_TOO_SMALL    (-4)

typedef enum
{
    SYSHAL_USB_EVENT_SEND_COMPLETE,
    SYSHAL_USB_EVENT_RECEIVE_COMPLETE,
    SYSHAL_USB_EVENT_CONNECTED,
    SYSHAL_USB_EVENT_DISCONNECTED,
} syshal_usb_event_id_t;

typedef struct
{
    syshal_usb_event_id_t id;
    union
    {
        struct
        {
            uint8_t * buffer;
            uint32_t size;
        } send;
        struct
        {
            uint8_t * buffer;
            uint32_t size;
        } receive;
    };
} syshal_usb_event_t;

int syshal_usb_init(void);
int syshal_usb_term(void);
bool syshal_usb_plugged_in(void);
int syshal_usb_send(uint8_t * data, uint32_t size);
int syshal_usb_receive(uint8_t * buffer, uint32_t size);
int syshal_usb_tick(void);

int syshal_usb_event_handler(syshal_usb_event_t * event);

#endif /* _SYSHAL_USB_H_ */