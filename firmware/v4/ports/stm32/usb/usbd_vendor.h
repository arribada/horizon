/**
  ******************************************************************************
  * @file    usbd_vendor.h
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   Header file for the usbd_hid_core.c file.
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

#ifndef _USB_VENDOR_H
#define _USB_VENDOR_H

#include "usbd_ioreq.h"
#include <stdbool.h>

#define VENDOR_ENDPOINT_PACKET_SIZE (USB_FS_MAX_PACKET_SIZE) // Endpoint IN & OUT Packet size, Limited to 64 as a maximum due to only running on USB-FS

typedef struct
{
    uint8_t * RxBuffer;
    uint8_t * TxBuffer;
    uint32_t RxLength;
    uint32_t TxLength;

    volatile bool TxPending;
    volatile bool RxPending;
}
USBD_Vendor_HandleTypeDef_t;

uint8_t USBD_Vendor_SetTxBuffer(USBD_HandleTypeDef * pdev, uint8_t * pbuff, uint32_t length);
uint8_t USBD_Vendor_SetRxBuffer(USBD_HandleTypeDef * pdev, uint8_t * pbuff, uint32_t length);
uint8_t USBD_Vendor_TransmitPacket(USBD_HandleTypeDef * pdev);
uint8_t USBD_Vendor_ReceivePacket(USBD_HandleTypeDef * pdev);

extern USBD_ClassTypeDef USBD_VENDOR;
#define USBD_VENDOR_CLASS &USBD_VENDOR

#endif  /* _USB_VENDOR_H */