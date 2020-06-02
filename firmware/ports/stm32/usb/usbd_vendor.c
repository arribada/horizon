/**
  ******************************************************************************
  * @file    usbd_vendor.c
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   This file provides the HID core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                HID Class  Description
  *          ===================================================================
  *           This module manages the HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - The Mouse protocol
  *             - Usage Page : Generic Desktop
  *             - Usage : Joystick
  *             - Collection : Application
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_vendor.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "syshal_usb.h"
#include "debug.h"

// Private defines
#define VENDOR_SPECIFIC               (0xFF)  // Vendor specified class

#define VENDOR_IN_EP                  (0x81)  // EP1 for data IN, Top bit is set to indicate the PC sends to this endpoint
#define VENDOR_OUT_EP                 (0x01)  // EP1 for data OUT, Top bit is clear to indicate the PC receives from this endpoint

#define VENDOR_DESC_ENDPOINT_COUNT    (2)

#define USB_VENDOR_CONFIG_DESC_SIZE (32)

////////////////////////////

static uint8_t USBD_Vendor_Init(USBD_HandleTypeDef * pdev, uint8_t cfgidx);
static uint8_t USBD_Vendor_DeInit(USBD_HandleTypeDef * pdev, uint8_t cfgidx);
static uint8_t USBD_Vendor_Setup(USBD_HandleTypeDef * pdev, USBD_SetupReqTypedef * req);
static uint8_t * USBD_Vendor_GetCfgDesc(uint16_t * length);
static uint8_t USBD_Vendor_DataIn(USBD_HandleTypeDef * pdev, uint8_t epnum);
static uint8_t USBD_Vendor_DataOut(USBD_HandleTypeDef * pdev, uint8_t epnum);

// USBD_HID_Private_Variables
USBD_ClassTypeDef USBD_VENDOR =
{
    USBD_Vendor_Init, // This callback is called when the device receives the set configuration request
    USBD_Vendor_DeInit, // This callback is called when the clear configuration request has been received
    USBD_Vendor_Setup, // This callback is called to handle the specific class setup requests
    NULL, // EP0_TxSent This callback is called when the send status is finished
    NULL, // EP0_RxReady This callback is called when the receive status is finished
    USBD_Vendor_DataIn, // This callback is called to perform the data in stage relative to the non-control endpoints
    USBD_Vendor_DataOut, // This callback is called to perform the data out stage relative to the non-control endpoints
    NULL, // This callback is called when a SOF interrupt is received
    NULL, // This callback is called when the last isochronous IN transfer is incomplete
    NULL, // This callback is called when the last isochronous OUT transfer is incomplete
    USBD_Vendor_GetCfgDesc, // This callback returns the HS USB Configuration descriptor
    USBD_Vendor_GetCfgDesc, // This callback returns the FS USB Configuration descriptor
    USBD_Vendor_GetCfgDesc, // his callback returns the other configuration descriptor of the used class in High Speed mode
    NULL, // This callback returns the Device Qualifier Descriptor (shouldn't be needed as we only support one speedmode)
};

// USB device Configuration Descriptor
__ALIGN_BEGIN static uint8_t USBD_Vendor_CfgDesc[USB_VENDOR_CONFIG_DESC_SIZE]  __ALIGN_END =
{
    /****************** Configuration Descriptor *****************/
    0x09,                              // Configuration Descriptor Size - always 9 bytes
    USB_DESC_TYPE_CONFIGURATION,       // Configuration descriptor type (0x02)
    USB_VENDOR_CONFIG_DESC_SIZE, 0x00, // Total length of the Configuration descriptor
    0x01,                              // Number of interfaces
    0x01,                              // Configuration value
    USBD_IDX_CONFIG_STR,               // Configuration Description String Index
    0xE0,                              // Attributes: bus powered and Support Remote Wake-up
    0x32,                              // MaxPower 100 mA: this current is used for detecting Vbus

    /******************* Interface Descriptor ********************/
    0x09,                       // Interface Descriptor size - always 9 bytes
    USB_DESC_TYPE_INTERFACE,    // Interface descriptor type (0x04)
    0x00,                       // The number of this interface
    0x00,                       // Alternate setting
    VENDOR_DESC_ENDPOINT_COUNT, // Number of endpoints used by this interface
    VENDOR_SPECIFIC,            // Class Code
    VENDOR_SPECIFIC,            // Subclass Code
    VENDOR_SPECIFIC,            // Protocol Code
    USBD_IDX_INTERFACE_STR,     // Index of String Descriptor describing this interface

    /******************* Endpoint Out Descriptor ******************/
    0x07,                                       // Interface Descriptor size - always 7 bytes
    USB_DESC_TYPE_ENDPOINT,                     // Endpoint descriptor type (0x05)
    VENDOR_OUT_EP,                              // Endpoint Address (OUT)
    USBD_EP_TYPE_BULK,                          // Attributes: Bulk data endpoint
    LOBYTE(VENDOR_ENDPOINT_PACKET_SIZE), HIBYTE(VENDOR_ENDPOINT_PACKET_SIZE), // Maximum Packet Size this endpoint is capable of receiving
    0x00, // Interval for polling endpoint data transfers, Ignored for Bulk Endpoints

    /******************* Endpoint In Descriptor *******************/
    0x07,                                       // Interface Descriptor size - always 7 bytes
    USB_DESC_TYPE_ENDPOINT,                     // Endpoint descriptor type (0x05)
    VENDOR_IN_EP,                               // Endpoint Address (IN)
    USBD_EP_TYPE_BULK,                          // Attributes: Bulk data endpoint
    LOBYTE(VENDOR_ENDPOINT_PACKET_SIZE), HIBYTE(VENDOR_ENDPOINT_PACKET_SIZE), // Maximum Packet Size this endpoint is capable of sending
    0x00, // Interval for polling endpoint data transfers, Ignored for Bulk Endpoints
};

/////////////////////////////// Private Functions ///////////////////////////////

static uint8_t USBD_Vendor_Init(USBD_HandleTypeDef * pdev, uint8_t cfgidx)
{
    // Open EP IN
    USBD_LL_OpenEP(pdev,
                   VENDOR_IN_EP,
                   USBD_EP_TYPE_BULK,
                   VENDOR_ENDPOINT_PACKET_SIZE);

    // Open EP OUT
    USBD_LL_OpenEP(pdev,
                   VENDOR_OUT_EP,
                   USBD_EP_TYPE_BULK,
                   VENDOR_ENDPOINT_PACKET_SIZE);

    // USBD_LL_OpenEP sets the endpoint to valid when actually we want to to be NAK to
    // prevent the client from accepting a write before a syshal_usb_receive() call
    PCD_HandleTypeDef * hpcd = pdev->pData;
    PCD_SET_EP_RX_STATUS(hpcd->Instance, VENDOR_OUT_EP & 0x7FU, USB_EP_RX_NAK); // We must manually set this instead

    pdev->pClassData = USBD_malloc(sizeof (USBD_Vendor_HandleTypeDef_t)); // NOTE Malloc may not be a good idea. Consider static define

    if (pdev->pClassData == NULL)
        return USBD_FAIL; // Malloc failed so return error

    USBD_Vendor_HandleTypeDef_t * hVendor;
    hVendor = (USBD_Vendor_HandleTypeDef_t *) pdev->pClassData;

    // Init Xfer states
    hVendor->TxPending = false;
    hVendor->RxPending = false;

    return USBD_OK;
}

static uint8_t USBD_Vendor_DeInit(USBD_HandleTypeDef * pdev, uint8_t cfgidx)
{
    // Close EP IN
    USBD_LL_CloseEP(pdev, VENDOR_IN_EP);

    // Close EP OUT
    USBD_LL_CloseEP(pdev, VENDOR_OUT_EP);

    // DeInit physical Interface components
    if (pdev->pClassData == NULL)
    {
        // NOTE potentially we may want to free up the RX/TX buffers here
        USBD_free(pdev->pClassData);
        pdev->pClassData = NULL;
    }

    return USBD_OK;
}

/**
 * @brief      Handle USB requests from the pc
 *
 * @param      pdev  The instance
 * @param      req   The usb requests
 *
 * @return     status
 */
static uint8_t USBD_Vendor_Setup(USBD_HandleTypeDef * pdev, USBD_SetupReqTypedef * req)
{
    USBD_Vendor_HandleTypeDef_t * hVendor;
    hVendor = (USBD_Vendor_HandleTypeDef_t *) pdev->pClassData;

    UNUSED(hVendor); // Temporary suppression of Compiler Warning

    DEBUG_PR_TRACE("USB Request %d", req->bmRequest & USB_REQ_TYPE_MASK);

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
        case USB_REQ_TYPE_CLASS:

            break;

        case USB_REQ_TYPE_STANDARD:

            break;

        default:
            break;
    }

    return USBD_OK;
}

/**
  * @brief      Return configuration descriptor
  *
  * @param      length  pointer data length
  *
  * @return     pointer to descriptor buffer
  */
static uint8_t * USBD_Vendor_GetCfgDesc(uint16_t * length)
{
    *length = sizeof(USBD_Vendor_CfgDesc);
    return USBD_Vendor_CfgDesc;
}

/**
  * @brief      Data sent on non-control IN endpoint
  *
  * @param      pdev   device instance
  * @param      epnum  endpoint number
  *
  * @return     status
  */
static uint8_t USBD_Vendor_DataIn(USBD_HandleTypeDef * pdev, uint8_t epnum)
{
    //DEBUG_PR_TRACE("%u: %s()", __LINE__, __FUNCTION__);

    USBD_Vendor_HandleTypeDef_t * hVendor;
    hVendor = (USBD_Vendor_HandleTypeDef_t *) pdev->pClassData;

    if (pdev->pClassData == NULL)
        return USBD_FAIL;

    hVendor->TxPending = false; // Set TX state flag to sent

    // Generate event for syshal_usb
    syshal_usb_event_t event;
    event.id = SYSHAL_USB_EVENT_SEND_COMPLETE;

    event.send.buffer = &hVendor->TxBuffer[0];
    event.send.size = hVendor->TxLength;

    syshal_usb_event_handler(&event);

    return USBD_OK;
}

/**
  * @brief      Data received on non-control Out endpoint
  *
  * @param      pdev   device instance
  * @param      epnum  endpoint number
  *
  * @return     status
  */
static uint8_t USBD_Vendor_DataOut(USBD_HandleTypeDef * pdev, uint8_t epnum)
{
    USBD_Vendor_HandleTypeDef_t * hVendor;
    hVendor = (USBD_Vendor_HandleTypeDef_t *) pdev->pClassData;

    // Get the received data length
    hVendor->RxLength = USBD_LL_GetRxDataSize(pdev, epnum);

    // USB data will be immediately processed, this allows the next USB traffic being
    // NAKed till the end of the application Xfer
    if (pdev->pClassData == NULL)
        return USBD_FAIL;

    hVendor->RxPending = false;

    // Generate event for syshal_usb
    syshal_usb_event_t event;
    event.id = SYSHAL_USB_EVENT_RECEIVE_COMPLETE;
    event.receive.buffer = &hVendor->RxBuffer[0];
    event.receive.size = hVendor->RxLength;

    syshal_usb_event_handler(&event);
    return (USBD_OK);
}

/////////////////////////////// Exposed Functions ///////////////////////////////

/**
  * @brief      Sets the transmit buffer to be used for USB comms
  *
  * @param      pdev    device instance
  * @param      pbuff   Tx Buffer
  * @param[in]  length  The length
  *
  * @return     status
  */
uint8_t USBD_Vendor_SetTxBuffer(USBD_HandleTypeDef * pdev, uint8_t * pbuff, uint32_t length)
{
    USBD_Vendor_HandleTypeDef_t * hVendor;
    hVendor = (USBD_Vendor_HandleTypeDef_t *) pdev->pClassData;

    hVendor->TxBuffer = pbuff;
    hVendor->TxLength = length;

    return USBD_OK;
}

/**
  * @brief      Sets the receive buffer to be used for USB comms
  *
  * @param      pdev   device instance
  * @param      pbuff  Rx Buffer
  *
  * @return     status
  */
uint8_t USBD_Vendor_SetRxBuffer(USBD_HandleTypeDef * pdev, uint8_t * pbuff, uint32_t length)
{
    USBD_Vendor_HandleTypeDef_t * hVendor;
    hVendor = (USBD_Vendor_HandleTypeDef_t *) pdev->pClassData;

    hVendor->RxBuffer = pbuff;
    hVendor->RxLength = length;

    return USBD_OK;
}

/**
  * @brief      data received on non-control Out endpoint
  *
  * @param      pdev  device instance
  *
  * @return     status
  */
uint8_t USBD_Vendor_TransmitPacket(USBD_HandleTypeDef * pdev)
{
    USBD_Vendor_HandleTypeDef_t * hVendor;
    hVendor = (USBD_Vendor_HandleTypeDef_t *) pdev->pClassData;

    //DEBUG_PR_TRACE("%u: %s()", __LINE__, __FUNCTION__);

    if (pdev->pClassData == NULL)
        return USBD_FAIL;

    //DEBUG_PR_TRACE("%u: %s()", __LINE__, __FUNCTION__);

    if (hVendor->TxPending)
        return USBD_BUSY;

    // Queue a transfer
    hVendor->TxPending = true;

    //DEBUG_PR_TRACE("%u: %s()", __LINE__, __FUNCTION__);

    // Transmit next packet
    USBD_LL_Transmit(pdev,
                     VENDOR_IN_EP,
                     hVendor->TxBuffer,
                     hVendor->TxLength);

    //DEBUG_PR_TRACE("%u: %s()", __LINE__, __FUNCTION__);

    return USBD_OK;
}

/**
  * @brief      prepare OUT Endpoint for reception
  *
  * @param      pdev  device instance
  *
  * @return     status
  */
uint8_t USBD_Vendor_ReceivePacket(USBD_HandleTypeDef * pdev)
{
    USBD_Vendor_HandleTypeDef_t * hVendor;
    hVendor = (USBD_Vendor_HandleTypeDef_t *) pdev->pClassData;

    if (pdev->pClassData == NULL)
        return USBD_FAIL;

    if (hVendor->RxPending)
        return USBD_BUSY;

    hVendor->RxPending = true;

    // Prepare Out endpoint to receive next packet
    USBD_LL_PrepareReceive(pdev,
                           VENDOR_OUT_EP,
                           hVendor->RxBuffer,
                           hVendor->RxLength);

    return USBD_OK;
}