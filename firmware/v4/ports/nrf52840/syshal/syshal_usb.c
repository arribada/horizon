/**
  ******************************************************************************
  * @file     syshal_usb.c
  * @brief    System hardware abstraction layer for USB
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2019 Arribada</center></h2>
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
  *
  ******************************************************************************
  */

#include <stdint.h>
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"
#include "nrf_drv_usbd.h"
#include "syshal_usb.h"
#include "debug.h"

/**
 * @brief Startup delay
 *
 * Number of microseconds to start USBD after powering up.
 * Kind of port insert debouncing.
 */
#define STARTUP_DELAY 100

#define LOBYTE(x)  ((uint8_t)(x & 0x00FF))
#define HIBYTE(x)  ((uint8_t)((x & 0xFF00) >>8))

#define USBD_VID                      (0x1915) // Nordic
#define USBD_LANGID_STRING            (0x0809) // English (United Kingdom)
#define USBD_PID_FS                   (0x0100)
#define USBD_SERIALNUMBER_STRING_FS   "00000000001A"

/** Maximum size of the packed transfered by EP0 */
#define EP0_MAXPACKETSIZE NRF_DRV_USBD_EPSIZE
#define N_ENDPOINT (2) // Number of endpoints, ignoring EP0

/** Device descriptor */
#define USBD_DEVICE_DESCRIPTOR \
    0x12,                                     /* bLength | size of descriptor                                                  */ \
    0x01,                                     /* bDescriptorType | descriptor type (Device Descriptor)                         */ \
    0x00, 0x02,                               /* bcdUSB | USB spec release (ver 2.0)                                           */ \
    0x00,                                     /* bDeviceClass ¦ class code (each interface specifies class information)        */ \
    0x00,                                     /* bDeviceSubClass ¦ device sub-class (must be set to 0 because class code is 0) */ \
    0x00,                                     /* bDeviceProtocol | device protocol (no class specific protocol)                */ \
    EP0_MAXPACKETSIZE,                        /* bMaxPacketSize0 | maximum packet size (64 bytes)                              */ \
    LOBYTE(USBD_VID), HIBYTE(USBD_VID),       /* vendor ID                                                                     */ \
    LOBYTE(USBD_PID_FS), HIBYTE(USBD_PID_FS), /* product ID                                                                    */ \
    0x01, 0x01,                               /* bcdDevice | final device release number in BCD Format                         */ \
    USBD_STRING_MANUFACTURER_IX,              /* iManufacturer | index of manufacturer string                                  */ \
    USBD_STRING_PRODUCT_IX,                   /* iProduct | index of product string                                            */ \
    USBD_STRING_SERIAL_IX,                    /* iSerialNumber | Serial Number string                                          */ \
    0x01                                      /* bNumConfigurations | number of configurations                                 */

/** Configuration descriptor */
#define DEVICE_SELF_POWERED 1
#define REMOTE_WU           1

#define USBD_CONFIG_DESCRIPTOR_FULL_SIZE   (9 + (9 + 7 + 7)) // All the descriptor lengths added together
#define USBD_CONFIG_DESCRIPTOR  \
    0x09,         /* bLength | length of descriptor                                             */ \
    0x02,         /* bDescriptorType | descriptor type (Configuration Descriptor)               */ \
    USBD_CONFIG_DESCRIPTOR_FULL_SIZE, 0x00,    /* wTotalLength | total length of descriptor(s)  */ \
    0x01,         /* bNumInterfaces                                                             */ \
    0x01,         /* bConfigurationValue                                                        */ \
    0x00,         /* index of string Configuration | configuration string index (not supported) */ \
    0x80| (((DEVICE_SELF_POWERED) ? 1U:0U)<<6) | (((REMOTE_WU) ? 1U:0U)<<5), /* bmAttributes    */ \
    0x32          /* maximum power in steps of 2mA (100mA)                                      */

#define USBD_INTERFACE0_DESCRIPTOR  \
    0x09,         /* bLength                                                                        */ \
    0x04,         /* bDescriptorType | descriptor type (INTERFACE)                                  */ \
    0x00,         /* bInterfaceNumber                                                               */ \
    0x00,         /* bAlternateSetting                                                              */ \
    0x02,         /* bNumEndpoints | number of endpoints (1)                                        */ \
    0xFF,         /* bInterfaceClass | Vendor Specific class                                        */ \
    0xFF,         /* bInterfaceSubClass | Vendor Specific Class                                     */ \
    0xFF,         /* bInterfaceProtocol | Vendor Specific Class                                     */ \
    0x00          /* interface string index (not supported)                                         */

#define USBD_ENDPOINTOUT1_DESCRIPTOR  \
    0x07,                      /* bLength | length of descriptor (7 bytes)                                       */ \
    0x05,                      /* bDescriptorType | descriptor type (ENDPOINT)                                   */ \
    0x01,                      /* bEndpointAddress | endpoint address (OUT endpoint, endpoint 1)                 */ \
    0x02,                      /* bmAttributes | endpoint attributes (bulk)                                      */ \
    NRF_DRV_USBD_EPSIZE,0x00,  /* bMaxPacketSizeLowByte,bMaxPacketSizeHighByte | maximum packet size (64 bytes)  */ \
    0x00                       /* bInterval | polling interval (ignored for bulk endpoints)                      */

#define USBD_ENDPOINTIN1_DESCRIPTOR  \
    0x07,                      /* bLength | length of descriptor (7 bytes)                                       */ \
    0x05,                      /* bDescriptorType | descriptor type (ENDPOINT)                                   */ \
    0x81,                      /* bEndpointAddress | endpoint address (IN endpoint, endpoint 1)                  */ \
    0x02,                      /* bmAttributes | endpoint attributes (bulk)                                      */ \
    NRF_DRV_USBD_EPSIZE,0x00,  /* bMaxPacketSizeLowByte,bMaxPacketSizeHighByte | maximum packet size (64 bytes)  */ \
    0x00                       /* bInterval | polling interval (ignored for bulk endpoints)                      */

/**
 * String config descriptor
 */
#define USBD_STRING_LANG_IX  0x00
#define USBD_STRING_LANG \
    0x04,                        /* length of descriptor     */ \
    0x03,                        /* descriptor type (string) */ \
    LOBYTE(USBD_LANGID_STRING),  /* Supported LangID         */ \
    HIBYTE(USBD_LANGID_STRING)   /*                          */

#define USBD_STRING_MANUFACTURER_IX  0x01
#define USBD_STRING_MANUFACTURER \
    18,           /* length of descriptor (? bytes) */ \
    0x03,         /* descriptor type (string)       */ \
    'A', 0x00,    /* Unicode string                 */ \
    'r', 0x00, \
    'r', 0x00, \
    'i', 0x00, \
    'b', 0x00, \
    'a', 0x00, \
    'd', 0x00, \
    'a', 0x00,

#define USBD_STRING_PRODUCT_IX  0x02
#define USBD_STRING_PRODUCT \
    24,           /* length of descriptor (? bytes) */ \
    0x03,         /* descriptor type (string)       */ \
    'G', 0x00,    /* Unicode string                 */ \
    'P', 0x00, \
    'S', 0x00, \
    ' ', 0x00, \
    'T', 0x00, \
    'r', 0x00, \
    'a', 0x00, \
    'c', 0x00, \
    'k', 0x00, \
    'e', 0x00, \
    'r', 0x00,

#define USBD_STRING_SERIAL_IX  0x03
#define USBD_STRING_SERIAL \
    26,           /* length of descriptor (? bytes) */ \
    0x03,         /* descriptor type (string)       */ \
    '0', 0x00,    /* Unicode string                 */ \
    '0', 0x00, \
    '0', 0x00, \
    '0', 0x00, \
    '0', 0x00, \
    '0', 0x00, \
    '0', 0x00, \
    '0', 0x00, \
    '0', 0x00, \
    '0', 0x00, \
    '1', 0x00, \
    'A', 0x00,

static volatile bool ep_pending[N_ENDPOINT]; // Number of endpoints, ignoring ep0

static const uint8_t get_descriptor_device[] = {USBD_DEVICE_DESCRIPTOR};

static const uint8_t get_descriptor_configuration[] =
{
    USBD_CONFIG_DESCRIPTOR,
    USBD_INTERFACE0_DESCRIPTOR,
    USBD_ENDPOINTOUT1_DESCRIPTOR,
    USBD_ENDPOINTIN1_DESCRIPTOR,
};

static const uint8_t get_descriptor_string_lang[] = {USBD_STRING_LANG};
static const uint8_t get_descriptor_string_manuf[] = {USBD_STRING_MANUFACTURER};
static const uint8_t get_descriptor_string_prod[] = {USBD_STRING_PRODUCT};
static const uint8_t get_descriptor_string_serial[] = {USBD_STRING_SERIAL};

static const uint8_t get_config_resp_configured[]   = {1};
static const uint8_t get_config_resp_unconfigured[] = {0};

static const uint8_t get_status_device_resp_nrwu[] =
{
    ((DEVICE_SELF_POWERED) ? 1 : 0), // LSB first: self-powered, no remoteWk
    0
};

static const uint8_t get_status_device_resp_rwu[]  =
{
    ((DEVICE_SELF_POWERED) ? 1 : 0) | 2, // LSB first: self-powered, remoteWk
    0
};

static const uint8_t get_status_interface_resp[] = {0, 0};
static const uint8_t get_status_ep_halted_resp[] = {1, 0};
static const uint8_t get_status_ep_active_resp[] = {0, 0};

static volatile bool rx_pending;
static uint8_t * rx_buffer;
static uint32_t rx_length;

static volatile bool tx_pending;
static uint8_t * tx_buffer;
static uint32_t tx_length;

#define GET_CONFIG_DESC_SIZE    sizeof(get_descriptor_configuration)
#define GET_INTERFACE_DESC_SIZE 9
#define GET_ENDPOINT_DESC_SIZE  7

#define get_descriptor_interface_0 &get_descriptor_configuration[9]

#define get_descriptor_endpointout_1 &get_descriptor_configuration[9+GET_INTERFACE_DESC_SIZE]
#define get_descriptor_endpointin_1  &get_descriptor_configuration[9+GET_INTERFACE_DESC_SIZE+GET_ENDPOINT_DESC_SIZE]

/**
 * @brief USB configured flag
 *
 * The flag that is used to mark the fact that USB is configured and ready
 * to transmit data
 */
static volatile bool m_usbd_configured = false;

/**
 * @brief Mark the fact if remote wake up is enabled
 *
 * The internal flag that marks if host enabled the remote wake up functionality in this device.
 */
static
#if REMOTE_WU
volatile // Disallow optimization only if Remote wakeup is enabled
#endif
bool m_usbd_rwu_enabled = false;

/**
 * @brief The requested suspend state
 *
 * The currently requested suspend state based on the events
 * received from USBD library.
 * If the value here is different than the @ref m_usbd_suspended
 * the state changing would be processed inside main loop.
 */
static volatile bool m_usbd_suspend_state_req = false;

/**
 * @brief System OFF request flag
 *
 * This flag is used in button event processing and marks the fact that
 * system OFF should be activated from main loop.
 */
static volatile bool m_system_off_req = false;

/**
 * @brief Setup all the endpoints for selected configuration
 *
 * Function sets all the endpoints for specific configuration.
 *
 * @note
 * Setting the configuration index 0 means technically disabling the HID interface.
 * Such configuration should be set when device is starting or USB reset is detected.
 *
 * @param index Configuration index
 *
 * @retval NRF_ERROR_INVALID_PARAM Invalid configuration
 * @retval NRF_SUCCESS             Configuration successfully set
 */
static ret_code_t ep_configuration(uint8_t index)
{
    if (index == 1)
    {
        nrf_drv_usbd_ep_dtoggle_clear(NRF_DRV_USBD_EPOUT1);
        nrf_drv_usbd_ep_stall_clear(NRF_DRV_USBD_EPOUT1);
        nrf_drv_usbd_ep_enable(NRF_DRV_USBD_EPOUT1);

        nrf_drv_usbd_ep_dtoggle_clear(NRF_DRV_USBD_EPIN1);
        nrf_drv_usbd_ep_stall_clear(NRF_DRV_USBD_EPIN1);
        nrf_drv_usbd_ep_enable(NRF_DRV_USBD_EPIN1);

        m_usbd_configured = true;
        nrf_drv_usbd_setup_clear();

        // Generate USB connected event
        syshal_usb_event_t event;
        event.id = SYSHAL_USB_EVENT_CONNECTED;
        syshal_usb_event_handler(&event);
    }
    else if (index == 0)
    {
        if (m_usbd_configured)
        {
            // Generate USB disconnected event
            syshal_usb_event_t event;
            event.id = SYSHAL_USB_EVENT_DISCONNECTED;
            syshal_usb_event_handler(&event);
        }

        nrf_drv_usbd_ep_disable(NRF_DRV_USBD_EPOUT1);
        nrf_drv_usbd_ep_disable(NRF_DRV_USBD_EPIN1);
        m_usbd_configured = false;
        nrf_drv_usbd_setup_clear();
    }
    else
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    return NRF_SUCCESS;
}

/**
 * @brief Respond on ep 0
 *
 * Auxiliary function for sending respond on endpoint 0
 * @param[in] p_setup Pointer to setup data from current setup request.
 *                    It would be used to calculate the size of data to send.
 * @param[in] p_data  Pointer to the data to send.
 * @param[in] size    Number of bytes to send.
 * @note Data pointed by p_data has to be available till the USBD_EVT_BUFREADY event.
 */
static void respond_setup_data(
    nrf_drv_usbd_setup_t const * const p_setup,
    void const * p_data, size_t size)
{
    /* Check the size against required response size */
    if (size > p_setup->wLength)
    {
        size = p_setup->wLength;
    }
    ret_code_t ret;
    nrf_drv_usbd_transfer_t transfer =
    {
        .p_data = {.tx = p_data},
        .size = size
    };
    ret = nrf_drv_usbd_ep_transfer(NRF_DRV_USBD_EPIN0, &transfer);
    if (ret != NRF_SUCCESS)
    {
        DEBUG_PR_ERROR("Transfer starting failed: %ld", (uint32_t)ret);
    }
    ASSERT(ret == NRF_SUCCESS);
    UNUSED_VARIABLE(ret);
}

/** React to GetStatus */
static void usbd_setup_GetStatus(nrf_drv_usbd_setup_t const * const p_setup)
{
    switch (p_setup->bmRequestType)
    {
        case 0x80: // Device
            if (((p_setup->wIndex) & 0xff) == 0)
            {
                respond_setup_data(
                    p_setup,
                    m_usbd_rwu_enabled ? get_status_device_resp_rwu : get_status_device_resp_nrwu,
                    sizeof(get_status_device_resp_nrwu));
                return;
            }
            break;
        case 0x81: // Interface
            if (m_usbd_configured) // Respond only if configured
            {
                if (((p_setup->wIndex) & 0xff) == 0) // Only interface 0 supported
                {
                    respond_setup_data(
                        p_setup,
                        get_status_interface_resp,
                        sizeof(get_status_interface_resp));
                    return;
                }
            }
            break;
        case 0x82: // Endpoint
            if (((p_setup->wIndex) & 0xff) == 0) // Endpoint 0
            {
                respond_setup_data(
                    p_setup,
                    get_status_ep_active_resp,
                    sizeof(get_status_ep_active_resp));
                return;
            }
            if (m_usbd_configured) // Other endpoints responds if configured
            {
                if (((p_setup->wIndex) & 0xff) == NRF_DRV_USBD_EPOUT1)
                {
                    if (nrf_drv_usbd_ep_stall_check(NRF_DRV_USBD_EPOUT1))
                    {
                        respond_setup_data(
                            p_setup,
                            get_status_ep_halted_resp,
                            sizeof(get_status_ep_halted_resp));
                        return;
                    }
                    else
                    {
                        respond_setup_data(
                            p_setup,
                            get_status_ep_active_resp,
                            sizeof(get_status_ep_active_resp));
                        return;
                    }
                }

                if (((p_setup->wIndex) & 0xff) == NRF_DRV_USBD_EPIN1)
                {
                    if (nrf_drv_usbd_ep_stall_check(NRF_DRV_USBD_EPIN1))
                    {
                        respond_setup_data(
                            p_setup,
                            get_status_ep_halted_resp,
                            sizeof(get_status_ep_halted_resp));
                        return;
                    }
                    else
                    {
                        respond_setup_data(
                            p_setup,
                            get_status_ep_active_resp,
                            sizeof(get_status_ep_active_resp));
                        return;
                    }
                }
            }
            break;
        default:
            break; // Just go to stall
    }
    DEBUG_PR_ERROR("Unknown status: 0x%02X", p_setup->bmRequestType);
    nrf_drv_usbd_setup_stall();
}

static void usbd_setup_ClearFeature(nrf_drv_usbd_setup_t const * const p_setup)
{
    if ((p_setup->bmRequestType) == 0x02) // standard request, recipient=endpoint
    {
        if ((p_setup->wValue) == 0)
        {
            if ((p_setup->wIndex) == NRF_DRV_USBD_EPOUT1)
            {
                nrf_drv_usbd_ep_stall_clear(NRF_DRV_USBD_EPOUT1);
                nrf_drv_usbd_setup_clear();
                return;
            }
            if ((p_setup->wIndex) == NRF_DRV_USBD_EPIN1)
            {
                nrf_drv_usbd_ep_stall_clear(NRF_DRV_USBD_EPIN1);
                nrf_drv_usbd_setup_clear();
                return;
            }
        }
    }
    else if ((p_setup->bmRequestType) ==  0x0) // standard request, recipient=device
    {
        if (REMOTE_WU)
        {
            if ((p_setup->wValue) == 1) // Feature Wakeup
            {
                m_usbd_rwu_enabled = false;
                nrf_drv_usbd_setup_clear();
                return;
            }
        }
    }
    DEBUG_PR_ERROR("Unknown feature to clear");
    nrf_drv_usbd_setup_stall();
}

static void usbd_setup_SetFeature(nrf_drv_usbd_setup_t const * const p_setup)
{
    if ((p_setup->bmRequestType) == 0x02) // standard request, recipient=endpoint
    {
        if ((p_setup->wValue) == 0) // Feature HALT
        {
            if ((p_setup->wIndex) == NRF_DRV_USBD_EPOUT1)
            {
                nrf_drv_usbd_ep_stall(NRF_DRV_USBD_EPOUT1);
                nrf_drv_usbd_setup_clear();
                return;
            }
            if ((p_setup->wIndex) == NRF_DRV_USBD_EPIN1)
            {
                nrf_drv_usbd_ep_stall(NRF_DRV_USBD_EPIN1);
                nrf_drv_usbd_setup_clear();
                return;
            }
        }
    }
    else if ((p_setup->bmRequestType) ==  0x0) // standard request, recipient=device
    {
        if (REMOTE_WU)
        {
            if ((p_setup->wValue) == 1) // Feature Wakeup
            {
                m_usbd_rwu_enabled = true;
                nrf_drv_usbd_setup_clear();
                return;
            }
        }
    }
    DEBUG_PR_ERROR("Unknown feature to set");
    nrf_drv_usbd_setup_stall();
}

static void usbd_setup_GetDescriptor(nrf_drv_usbd_setup_t const * const p_setup)
{
    //determine which descriptor has been asked for
    switch ((p_setup->wValue) >> 8)
    {
        case 1: // Device
            if ((p_setup->bmRequestType) == 0x80)
            {
                respond_setup_data(
                    p_setup,
                    get_descriptor_device,
                    sizeof(get_descriptor_device));
                return;
            }
            break;
        case 2: // Configuration
            if ((p_setup->bmRequestType) == 0x80)
            {
                respond_setup_data(
                    p_setup,
                    get_descriptor_configuration,
                    GET_CONFIG_DESC_SIZE);
                return;
            }
            break;
        case 3: // String
            if ((p_setup->bmRequestType) == 0x80)
            {
                // Select the string
                switch ((p_setup->wValue) & 0xFF)
                {
                    case USBD_STRING_LANG_IX:
                        respond_setup_data(p_setup,
                                           get_descriptor_string_lang,
                                           sizeof(get_descriptor_string_lang));
                        return;
                    case USBD_STRING_MANUFACTURER_IX:
                        respond_setup_data(p_setup,
                                           get_descriptor_string_manuf,
                                           sizeof(get_descriptor_string_manuf));
                        return;
                    case USBD_STRING_PRODUCT_IX:
                        respond_setup_data(p_setup,
                                           get_descriptor_string_prod,
                                           sizeof(get_descriptor_string_prod));
                        return;
                    case USBD_STRING_SERIAL_IX:
                        respond_setup_data(p_setup,
                                           get_descriptor_string_serial,
                                           sizeof(get_descriptor_string_serial));
                        return;
                    default:
                        break;
                }
            }
            break;
        case 4: // Interface
            if ((p_setup->bmRequestType) == 0x80)
            {
                // Which interface?
                if ((((p_setup->wValue) & 0xFF) == 0))
                {
                    respond_setup_data(
                        p_setup,
                        get_descriptor_interface_0,
                        GET_INTERFACE_DESC_SIZE);
                    return;
                }
            }
            break;
        case 5: // Endpoint
            if ((p_setup->bmRequestType) == 0x80)
            {
                // Which endpoint?
                if (((p_setup->wValue) & 0xFF) == 1)
                {
                    respond_setup_data(
                        p_setup,
                        get_descriptor_endpointin_1,
                        GET_ENDPOINT_DESC_SIZE);
                    return;
                }

                if (((p_setup->wValue) & 0xFF) == 2)
                {
                    respond_setup_data(
                        p_setup,
                        get_descriptor_endpointout_1,
                        GET_ENDPOINT_DESC_SIZE);
                    return;
                }
            }
            break;
        default:
            break; // Not supported - go to stall
    }

    DEBUG_PR_WARN("Unknown USB descriptor requested: 0x%02X, type: 0x%02X or value: 0x%02X",
                  p_setup->wValue >> 8,
                  p_setup->bmRequestType,
                  p_setup->wValue & 0xFF);
    nrf_drv_usbd_setup_stall();
}

static void usbd_setup_GetConfig(nrf_drv_usbd_setup_t const * const p_setup)
{
    if (m_usbd_configured)
    {
        respond_setup_data(
            p_setup,
            get_config_resp_configured,
            sizeof(get_config_resp_configured));
    }
    else
    {
        respond_setup_data(
            p_setup,
            get_config_resp_unconfigured,
            sizeof(get_config_resp_unconfigured));
    }
}

static void usbd_setup_SetConfig(nrf_drv_usbd_setup_t const * const p_setup)
{
    if ((p_setup->bmRequestType) == 0x00)
    {
        // accept only 0 and 1
        if (((p_setup->wIndex) == 0) && ((p_setup->wLength) == 0) &&
            ((p_setup->wValue) <= UINT8_MAX))
        {
            if (NRF_SUCCESS == ep_configuration((uint8_t)(p_setup->wValue)))
            {
                nrf_drv_usbd_setup_clear();
                return;
            }
        }
    }
    DEBUG_PR_ERROR("Wrong configuration: Index: 0x%02X, Value: 0x%02X.",
                   p_setup->wIndex,
                   p_setup->wValue);
    nrf_drv_usbd_setup_stall();
}

static void usbd_setup_SetIdle(nrf_drv_usbd_setup_t const * const p_setup)
{
    if (p_setup->bmRequestType == 0x21)
    {
        //accept any value
        nrf_drv_usbd_setup_clear();
        return;
    }
    DEBUG_PR_ERROR("Set Idle wrong type: 0x%02X.", p_setup->bmRequestType);
    nrf_drv_usbd_setup_stall();
}

static void usbd_setup_SetInterface(
    nrf_drv_usbd_setup_t const * const p_setup)
{
    //no alternate setting is supported - STALL always
    DEBUG_PR_ERROR("No alternate interfaces supported.");
    nrf_drv_usbd_setup_stall();
}

static void usbd_setup_SetProtocol(
    nrf_drv_usbd_setup_t const * const p_setup)
{
    if (p_setup->bmRequestType == 0x21)
    {
        //accept any value
        nrf_drv_usbd_setup_clear();
        return;
    }
    DEBUG_PR_ERROR("Set Protocol wrong type: 0x%02X.", p_setup->bmRequestType);
    nrf_drv_usbd_setup_stall();
}

/**
 * @brief      This function is called whenever a event occurs on the USB bus.
 *             This should be overriden by config_if.c
 *
 * @param[out] event  The event
 *
 * @return     Return error code
 */
__attribute__((weak)) int syshal_usb_event_handler(syshal_usb_event_t * event)
{
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);

    return SYSHAL_USB_NO_ERROR;
}

static void usbd_event_handler(nrf_drv_usbd_evt_t const * const p_event)
{
    ret_code_t ret;

    switch (p_event->type)
    {
        case NRF_DRV_USBD_EVT_SUSPEND:
            DEBUG_PR_TRACE("SUSPEND state detected");
            ret = ep_configuration(0);
            UNUSED_VARIABLE(ret);
            m_usbd_suspend_state_req = true;
            break;
        case NRF_DRV_USBD_EVT_RESUME:
            DEBUG_PR_TRACE("RESUMING from suspend");
            m_usbd_suspend_state_req = false;
            break;
        case NRF_DRV_USBD_EVT_WUREQ:
            DEBUG_PR_TRACE("RemoteWU initiated");
            m_usbd_suspend_state_req = false;
            break;
        case NRF_DRV_USBD_EVT_RESET:
        {
            DEBUG_PR_TRACE("RemoteWU initiated");
            ret = ep_configuration(0);
            //ASSERT(ret == NRF_SUCCESS);
            UNUSED_VARIABLE(ret);
            m_usbd_suspend_state_req = false;
            break;
        }
        case NRF_DRV_USBD_EVT_SOF:
        {
            break;
        }
        case NRF_DRV_USBD_EVT_EPTRANSFER:
            if (NRF_DRV_USBD_EPIN0 == p_event->data.eptransfer.ep)
            {
                //DEBUG_PR_TRACE("EPIN0");
                if (NRF_USBD_EP_OK == p_event->data.eptransfer.status)
                {
                    if (!nrf_drv_usbd_errata_154())
                    {
                        nrf_drv_usbd_setup_clear();
                    }
                }
                else if (NRF_USBD_EP_ABORTED == p_event->data.eptransfer.status)
                {
                    /* Just ignore */
                    DEBUG_PR_TRACE("Transfer aborted event on EPIN0");
                }
                else
                {
                    DEBUG_PR_ERROR("Transfer failed on EPIN0: %d", p_event->data.eptransfer.status);
                    nrf_drv_usbd_setup_stall();
                }
            }
            else if (NRF_DRV_USBD_EPIN1 == p_event->data.eptransfer.ep)
            {
                //DEBUG_PR_TRACE("EPIN1");
                if (NRF_USBD_EP_OK == p_event->data.eptransfer.status)
                {
                    // Generate send complete event
                    syshal_usb_event_t event;
                    event.id = SYSHAL_USB_EVENT_SEND_COMPLETE;
                    event.send.buffer = &tx_buffer[0];
                    event.send.size = tx_length;
                    syshal_usb_event_handler(&event);

                    tx_pending = false;
                }
            }
            else if (NRF_DRV_USBD_EPOUT1 == p_event->data.eptransfer.ep)
            {
                //DEBUG_PR_TRACE("EPOUT1");
                if (NRF_USBD_EP_WAITING == p_event->data.eptransfer.status)
                {
                    //DEBUG_PR_TRACE("NRF_USBD_EP_WAITING");
                }
                else if (NRF_USBD_EP_OK == p_event->data.eptransfer.status)
                {
                    size_t size = 0;
                    nrf_drv_usbd_ep_status_get(p_event->data.eptransfer.ep, &size);

                    if (!size)
                    {
                        // If this is a zero length packet, ignore it and queue a new receive
                        NRF_DRV_USBD_TRANSFER_OUT(transfer, rx_buffer, rx_length);
                        nrf_drv_usbd_ep_transfer(NRF_DRV_USBD_EPOUT1, &transfer);
                        break;
                    }

                    // Generate receive complete event
                    syshal_usb_event_t event;
                    event.id = SYSHAL_USB_EVENT_RECEIVE_COMPLETE;
                    event.send.buffer = &rx_buffer[0];
                    rx_length = size;
                    event.send.size = rx_length;
                    syshal_usb_event_handler(&event);

                    rx_pending = false;
                }
                else
                {
                    DEBUG_PR_TRACE("Transfer failed on EPOUT1: %d\n", p_event->data.eptransfer.status);
                }
            }
            else
            {
                /* Nothing to do */
            }
            break;
        case NRF_DRV_USBD_EVT_SETUP:
        {
            //DEBUG_PR_TRACE("NRF_DRV_USBD_EVT_SETUP");
            nrf_drv_usbd_setup_t setup;
            nrf_drv_usbd_setup_get(&setup);
            switch (setup.bRequest)
            {
                case 0x00: // GetStatus
                    usbd_setup_GetStatus(&setup);
                    break;
                case 0x01: // CleartFeature
                    usbd_setup_ClearFeature(&setup);
                    break;
                case 0x03: // SetFeature
                    usbd_setup_SetFeature(&setup);
                    break;
                case 0x05: // SetAddress
                    //nothing to do, handled by hardware; but don't STALL
                    break;
                case 0x06: // GetDescriptor
                    usbd_setup_GetDescriptor(&setup);
                    break;
                case 0x08: // GetConfig
                    usbd_setup_GetConfig(&setup);
                    break;
                case 0x09: // SetConfig
                    usbd_setup_SetConfig(&setup);
                    break;
                //HID class
                case 0x0A: // SetIdle
                    usbd_setup_SetIdle(&setup);
                    break;
                case 0x0B: // SetProtocol or SetInterface
                    if (setup.bmRequestType == 0x01) // standard request, recipient=interface
                    {
                        usbd_setup_SetInterface(&setup);
                    }
                    else if (setup.bmRequestType == 0x21) // class request, recipient=interface
                    {
                        usbd_setup_SetProtocol(&setup);
                    }
                    else
                    {
                        DEBUG_PR_WARN("Command 0xB. Unknown USB request: 0x%02X", setup.bmRequestType);
                        nrf_drv_usbd_setup_stall();
                    }
                    break;
                default:
                    DEBUG_PR_WARN("Unknown USB request: 0x%02X", setup.bRequest);
                    nrf_drv_usbd_setup_stall();
                    return;
            }
            break;
        }
        default:
            break;
    }
}

int syshal_usb_init(void)
{
    ret_code_t ret;

    tx_pending = false;
    rx_pending = false;

    DEBUG_PR_TRACE("usb_init");
    if (NRF_DRV_USBD_ERRATA_ENABLE)
    {
        DEBUG_PR_TRACE("USB errata 104 %s", (nrf_drv_usbd_errata_104() ? "enabled" : "disabled"));
        DEBUG_PR_TRACE("USB errata 154 %s", (nrf_drv_usbd_errata_154() ? "enabled" : "disabled"));
    }

    /* USB work starts right here */
    ret = nrf_drv_usbd_init(usbd_event_handler);
    APP_ERROR_CHECK(ret);

    /* Configure selected size of the packed on EP0 */
    nrf_drv_usbd_ep_max_packet_size_set(NRF_DRV_USBD_EPOUT0, EP0_MAXPACKETSIZE);
    nrf_drv_usbd_ep_max_packet_size_set(NRF_DRV_USBD_EPIN0, EP0_MAXPACKETSIZE);

    DEBUG_PR_TRACE("Starting USB now");
    nrf_delay_us(STARTUP_DELAY);
    if (!nrf_drv_usbd_is_enabled())
    {
        nrf_drv_usbd_enable();
        ret = ep_configuration(0);
        APP_ERROR_CHECK(ret);
    }
    /* Wait for regulator power up */
    while (NRF_DRV_POWER_USB_STATE_CONNECTED == nrf_drv_power_usbstatus_get())
    {
        /* Just waiting */
    }

    if (NRF_DRV_POWER_USB_STATE_READY == nrf_drv_power_usbstatus_get())
    {
        if (!nrf_drv_usbd_is_started())
        {
            nrf_drv_usbd_start(true);
        }
    }
    else
    {
        nrf_drv_usbd_disable();
    }

    return SYSHAL_USB_NO_ERROR;
}

int syshal_usb_term(void)
{
    DEBUG_PR_TRACE("Stopping USB");
    nrf_drv_usbd_disable();
    nrf_drv_usbd_uninit();

    return SYSHAL_USB_NO_ERROR;
}

bool syshal_usb_plugged_in(void)
{
    return nrf_power_usbregstatus_vbusdet_get();
}

int syshal_usb_send(uint8_t * data, uint32_t size)
{
    if (!m_usbd_configured)
        return SYSHAL_USB_ERROR_DISCONNECTED;

    if (tx_pending)
        return SYSHAL_USB_ERROR_BUSY;

    tx_pending = true;
    tx_buffer = data;
    tx_length = size;

    NRF_DRV_USBD_TRANSFER_IN(transfer, data, size);
    nrf_drv_usbd_ep_transfer(NRF_DRV_USBD_EPIN1, &transfer);

    return SYSHAL_USB_NO_ERROR;
}

int syshal_usb_receive(uint8_t * buffer, uint32_t size)
{
    if (!m_usbd_configured)
        return SYSHAL_USB_ERROR_DISCONNECTED;

    if (rx_pending)
        return SYSHAL_USB_ERROR_BUSY;

    rx_pending = true;
    rx_buffer = buffer;

    NRF_DRV_USBD_TRANSFER_OUT(transfer, buffer, size);
    nrf_drv_usbd_ep_transfer(NRF_DRV_USBD_EPOUT1, &transfer);

    return SYSHAL_USB_NO_ERROR;
}

int syshal_usb_tick(void)
{
    return SYSHAL_USB_NO_ERROR;
}