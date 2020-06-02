/* M8N.c - HAL for gps device Ublox Neo-M8N
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

#include <string.h>
#include "M8N.h"
#include "syshal_gpio.h"
#include "syshal_gps.h"
#include "syshal_uart.h"
#include "syshal_timer.h"
#include "sys_config.h"
#include "debug.h"
#include "bsp.h"

#define M8N_TIMEOUT_MS (200)

static UBX_ACK_t last_ack;
static UBX_NACK_t last_nack;

#define EXPECTED_BAUD_RATE_INDEX (7) // The expected starting baudrate of the GPS under normal operation
static const uint32_t supported_baudrates_priv[] = {4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800};

static syshal_gps_state_t state = STATE_UNINIT;

#define MAX_BAUD_RATE (460800)
#define MIN_TIME_BETWEEN_SHUTDOWN_AND_WAKEUP_MS (200)

#define GPS_RESTART_TIME_MS (10) // How long to hold the reset line low on the GPS device if it is unresponsive

#define SYSHAL_GPS_GPIO_POWER_ON (GPIO_GPS_EXT_INT)

// Private functions
static int syshal_gps_parse_rx_buffer_priv(UBX_Packet_t * packet);
static void syshal_gps_process_nav_status_priv(const UBX_Packet_t * packet);
static void syshal_gps_process_nav_pvt_priv(const UBX_Packet_t * packet);
static void syshal_gps_process_ack_priv(UBX_Packet_t * packet);
static void syshal_gps_process_nack_priv(UBX_Packet_t * packet);
static void syshal_gps_set_checksum_priv(UBX_Packet_t * packet);
static void syshal_gps_send_packet_priv(UBX_Packet_t * ubx_packet);
static bool syshal_gps_device_responsive_priv(void);
static void syshal_gps_look_for_ack_or_nack_priv(void);

// HAL to SYSHAL error code mapping table
static int hal_error_map[] =
{
    SYSHAL_GPS_NO_ERROR,
    SYSHAL_GPS_ERROR_DEVICE,
    SYSHAL_GPS_ERROR_BUSY,
    SYSHAL_GPS_ERROR_TIMEOUT,
};

int syshal_gps_init(void)
{
    bool is_responsive;
    uint32_t baud_start_index;
    int total_retries = 5;

    syshal_gpio_init(SYSHAL_GPS_GPIO_POWER_ON);
    syshal_gpio_set_output_high(SYSHAL_GPS_GPIO_POWER_ON);

auto_baud:
    is_responsive = false;

    DEBUG_PR_INFO("Trying to detect M8N baud rate using auto-baud...");

    const size_t num_of_supported_baudrates = sizeof(supported_baudrates_priv) / sizeof(supported_baudrates_priv[0]);

    if (num_of_supported_baudrates > EXPECTED_BAUD_RATE_INDEX)
        baud_start_index = EXPECTED_BAUD_RATE_INDEX;
    else
        baud_start_index = 0; // Protect from a potential buffer overrun

    uint32_t baud_index = 0;

    // Go through every baudrate and try to detect what is currently being used
    for (uint32_t i = 0; i < num_of_supported_baudrates; i++)
    {
        baud_index = (i + baud_start_index) % num_of_supported_baudrates;

        syshal_uart_change_baud(UART_GPS, supported_baudrates_priv[baud_index]);

        for (unsigned int j = 0; j < 2; j++)
        {
            if (syshal_gps_device_responsive_priv())
            {
                DEBUG_PR_INFO("Detected %i baud rate using auto-baud", (unsigned int)supported_baudrates_priv[baud_index]);
                is_responsive = true;
                goto done;
            }
        }

        // Empty the UART RX buffer before proceeding
        syshal_uart_flush(UART_GPS);
    }

done:
    if (!is_responsive)
    {
        DEBUG_PR_ERROR("Failed to detect GPS baud rate using auto-baud");

        total_retries--;
        if (total_retries > 0)
            goto auto_baud;

        return SYSHAL_GPS_ERROR_TIMEOUT;
    }

    if (is_responsive && MAX_BAUD_RATE != supported_baudrates_priv[baud_index])
    {
        DEBUG_PR_INFO("Changing baud rate to %u", MAX_BAUD_RATE);

        UBX_Packet_t ubx_packet;
        memset(&ubx_packet, 0, sizeof(ubx_packet));
        UBX_SET_PACKET_HEADER(&ubx_packet, UBX_MSG_CLASS_CFG, UBX_MSG_ID_CFG_PRT, sizeof(UBX_CFG_PRT2_t));
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_PRT2)->portID = 1;
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_PRT2)->txReady = 0;
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_PRT2)->mode = (3 << 6) | (4 << 9);
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_PRT2)->inProtoMask = 1;
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_PRT2)->outProtoMask = 1;
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_PRT2)->flags = 0;
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_PRT2)->baudRate = MAX_BAUD_RATE;

        DEBUG_PR_TRACE("Sending UBX:CFG_PRT2");
        syshal_gps_send_packet_priv(&ubx_packet);
        syshal_uart_change_baud(UART_GPS, MAX_BAUD_RATE);

        memset(&ubx_packet, 0, sizeof(ubx_packet));
        UBX_SET_PACKET_HEADER(&ubx_packet, UBX_MSG_CLASS_CFG, UBX_MSG_ID_CFG_CFG, sizeof(UBX_CFG_CFG2_t));
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_CFG2)->clearMask = 0;
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_CFG2)->saveMask = UBX_CFG_CFG_MASK_IOPORT;
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_CFG2)->loadMask = 0;
        UBX_PAYLOAD(&ubx_packet, UBX_CFG_CFG2)->deviceMask = UBX_CFG_CFG_DEV_FLASH;

        last_ack.clsID = 0xFF;
        last_ack.msgID = 0xFF;
        last_nack.clsID = 0xFF;
        last_nack.msgID = 0xFF;

        // Empty the UART RX buffer before proceeding
        syshal_uart_flush(UART_GPS);

        DEBUG_PR_TRACE("Sending UBX:UBX_CFG_CFG2");
        syshal_gps_send_packet_priv(&ubx_packet);

        is_responsive = false;
        uint32_t start_time = syshal_time_get_ticks_ms();
        while (syshal_time_get_ticks_ms() - start_time < M8N_TIMEOUT_MS)
        {
            syshal_gps_look_for_ack_or_nack_priv();

            // Look for a ACK response to this message
            if (last_ack.clsID == UBX_MSG_CLASS_CFG &&
                last_ack.msgID == UBX_MSG_ID_CFG_CFG)
            {
                DEBUG_PR_TRACE("UBX:UBX_CFG_CFG2 ACKed");
                last_ack.clsID = 0xFF;
                last_ack.msgID = 0xFF;
                is_responsive = true;
                break;
            }
            else if (last_nack.clsID == UBX_MSG_CLASS_CFG &&
                     last_nack.msgID == UBX_MSG_ID_CFG_CFG)
            {
                DEBUG_PR_TRACE("UBX:UBX_CFG_CFG2 NACKed");
                last_nack.clsID = 0xFF;
                last_nack.msgID = 0xFF;
                break;
            }
        }

        if (is_responsive)
        {
            if (syshal_gps_device_responsive_priv())
            {
                DEBUG_PR_INFO("Successfully changed baud to %u", MAX_BAUD_RATE);
            }
            else
            {
                DEBUG_PR_ERROR("Device unresponsive after changing baud to %u", MAX_BAUD_RATE);
                total_retries--;
                if (total_retries > 0)
                    goto auto_baud;

                return SYSHAL_GPS_ERROR_TIMEOUT;
            }
        }
        else
        {
            DEBUG_PR_WARN("Failed to change baud to %u", MAX_BAUD_RATE);
            total_retries--;
            if (total_retries > 0)
                goto auto_baud;

            return SYSHAL_GPS_ERROR_TIMEOUT;
        }
    }

    // Make sure the device is asleep
    syshal_gps_shutdown();

    // Empty the UART RX buffer
    syshal_uart_flush(UART_GPS);

    return SYSHAL_GPS_NO_ERROR;
}

/**
 * @brief      GPS callback stub, should be overriden by the user application
 *
 * @param[in]  event  The event that occured
 */
__attribute__((weak)) void syshal_gps_callback(const syshal_gps_event_t * event)
{
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);
}

/**
 * @brief      Turn the GPS off
 */
int syshal_gps_shutdown(void)
{
    if (STATE_ASLEEP == state)
        return SYSHAL_GPS_ERROR_INVALID_STATE;

    syshal_gpio_set_output_low(SYSHAL_GPS_GPIO_POWER_ON);
    syshal_time_delay_ms(GPS_RESTART_TIME_MS);

    syshal_gps_tick(); // Process any messages that might be remaining in the UART buffer

    DEBUG_PR_TRACE("Shutdown GPS %s()", __FUNCTION__);

    state = STATE_ASLEEP;

    syshal_gps_event_t event;
    event.id = SYSHAL_GPS_EVENT_POWERED_OFF;
    syshal_gps_callback(&event);
    
    return SYSHAL_GPS_NO_ERROR;
}

/**
 * @brief      Wake up the GPS device from a shutdown
 */
int syshal_gps_wake_up(void)
{
    if (STATE_ASLEEP != state)
        return SYSHAL_GPS_ERROR_INVALID_STATE;

    DEBUG_PR_TRACE("Wakeup GPS %s()", __FUNCTION__);

    state = STATE_ACQUIRING;

    syshal_uart_flush(UART_GPS);

    syshal_gpio_set_output_high(SYSHAL_GPS_GPIO_POWER_ON);
    syshal_time_delay_ms(GPS_RESTART_TIME_MS);

    syshal_gps_event_t event;
    event.id = SYSHAL_GPS_EVENT_POWERED_ON;
    syshal_gps_callback(&event);

    return SYSHAL_GPS_NO_ERROR;
}

/**
 * @brief      Process the UART Rx buffer looking for any and all packets, if
 *             any are valid process them and generate a callback event
 */
int syshal_gps_tick(void)
{
    UBX_Packet_t ubx_packet;
    int error;

    if (STATE_ASLEEP == state)
        return SYSHAL_GPS_NO_ERROR; // Ignore messages received after shutdown

    do
    {
        error = syshal_gps_parse_rx_buffer_priv(&ubx_packet);

        if (GPS_UART_ERROR_CHECKSUM == error)
        {
            DEBUG_PR_TRACE("GPS Checksum error");
        }
        else if (GPS_UART_ERROR_MSG_TOO_BIG == error)
        {
            //DEBUG_PR_TRACE("GPS Message too big");
        }
        else if (GPS_UART_ERROR_INSUFFICIENT_BYTES == error)
        {
            //DEBUG_PR_TRACE("GPS Uart insufficient bytes");
            break;
        }
        else if (GPS_UART_ERROR_MISSING_SYNC1 == error)
        {
            //DEBUG_PR_TRACE("GPS missing Sync1");
        }
        else if (GPS_UART_ERROR_MISSING_SYNC2 == error)
        {
            //DEBUG_PR_TRACE("GPS missing Sync2");
        }
        else if (GPS_UART_ERROR_MSG_PENDING == error)
        {
            //DEBUG_PR_TRACE("GPS message not fully received");
        }
        else if (GPS_UART_NO_ERROR != error)
        {
            DEBUG_PR_TRACE("GPS Generic comm error");
        }

        // Correct packet so process it
        if (GPS_UART_NO_ERROR == error)
        {
            if (UBX_IS_MSG(&ubx_packet, UBX_MSG_CLASS_NAV, UBX_MSG_ID_NAV_PVT))
                syshal_gps_process_nav_pvt_priv(&ubx_packet);
            else if (UBX_IS_MSG(&ubx_packet, UBX_MSG_CLASS_NAV, UBX_MSG_ID_NAV_STATUS))
                syshal_gps_process_nav_status_priv(&ubx_packet);
            else
                DEBUG_PR_WARN("Unexpected GPS message class: (0x%02X) id: (0x%02X)", ubx_packet.msgClass, ubx_packet.msgId);
        }

    }
    while ( error == GPS_UART_NO_ERROR ); // Repeat this for every packet correctly received

    return SYSHAL_GPS_NO_ERROR;
}

/**
 * @brief      Change the baudrate used to communicate with the GPS module
 *
 * @param[in]  baudrate  The baudrate to change to
 */
int syshal_gps_set_baud(uint32_t baudrate)
{
    syshal_uart_change_baud(UART_GPS, baudrate);
    return SYSHAL_GPS_NO_ERROR;
}

/**
 * @brief      Sends raw unedited data to the GPS module
 *
 * @param[in]  data  The data to be transmitted
 * @param[in]  size  The size of the data in bytes
 *
 * @return     Error code
 */
int syshal_gps_send_raw(uint8_t * data, uint32_t size)
{
    return hal_error_map[syshal_uart_send(UART_GPS, data, size)];
}

/**
 * @brief      Receives raw unedited data from the GPS module. syshal_gps_tick()
 *             should not be called when the user is expecting a raw message
 *
 * @param[in]  data  The received data
 * @param[in]  size  The max size of the data to be read in bytes
 *
 * @return     Actual number of bytes read
 */
int syshal_gps_receive_raw(uint8_t * data, uint32_t size)
{
    return syshal_uart_receive(UART_GPS, data, size);
}

/**
 * @brief      Returns the number of bytes in the GPS receive buffer
 *
 * @return     Number of bytes
 */
uint32_t syshal_gps_available_raw(void)
{
    return syshal_uart_available(UART_GPS);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// Private functions //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void syshal_gps_send_packet_priv(UBX_Packet_t * ubx_packet)
{
    syshal_gps_set_checksum_priv(ubx_packet);

    // The packet is not arranged contiguously in RAM, so we have to transmit
    // it using two passes i.e., header then payload + CRC
    syshal_uart_send(UART_GPS, (uint8_t *) ubx_packet, UBX_HEADER_LENGTH);
    syshal_uart_send(UART_GPS, ubx_packet->payloadAndCrc, ubx_packet->msgLength + UBX_CRC_LENGTH);
}

static void syshal_gps_process_nav_status_priv(const UBX_Packet_t * packet)
{
    // Populate the event and pass it to the main application
    syshal_gps_event_t event;
    event.id = SYSHAL_GPS_EVENT_STATUS;
    memcpy(&event.status, &packet->UBX_NAV_STATUS, sizeof(syshal_gps_event_status_t));

    if (event.status.gpsFix > 0)
        state = STATE_FIXED;
    else
        state = STATE_ACQUIRING;

    syshal_gps_callback(&event);
}

static void syshal_gps_process_nav_pvt_priv(const UBX_Packet_t * packet)
{
    // Populate the event and pass it to the main application
    syshal_gps_event_t event;
    event.id = SYSHAL_GPS_EVENT_PVT;

    event.pvt.iTOW = packet->UBX_NAV_PVT.iTOW;
    event.pvt.gpsFix = packet->UBX_NAV_PVT.fixType;
    event.pvt.lon = packet->UBX_NAV_PVT.lon;
    event.pvt.lat = packet->UBX_NAV_PVT.lat;
    event.pvt.hMSL = packet->UBX_NAV_PVT.hMSL;
    event.pvt.hAcc = packet->UBX_NAV_PVT.hAcc;
    event.pvt.vAcc = packet->UBX_NAV_PVT.vAcc;

    event.pvt.date_time_valid = packet->UBX_NAV_PVT.valid & (UBX_NAV_PVT_VALID_FLAGS_DATE | UBX_NAV_PVT_VALID_FLAGS_TIME);

    if (event.pvt.date_time_valid)
    {
        event.pvt.date_time.year = packet->UBX_NAV_PVT.year;
        event.pvt.date_time.month = packet->UBX_NAV_PVT.month;
        event.pvt.date_time.day = packet->UBX_NAV_PVT.day;
        event.pvt.date_time.hours = packet->UBX_NAV_PVT.hour;
        event.pvt.date_time.minutes = packet->UBX_NAV_PVT.min;
        event.pvt.date_time.seconds = packet->UBX_NAV_PVT.sec;
        event.pvt.date_time.milliseconds = 0;
    }

    if (event.pvt.gpsFix > 0)
        state = STATE_FIXED;
    else
        state = STATE_ACQUIRING;

    syshal_gps_callback(&event);
}

static void syshal_gps_process_ack_priv(UBX_Packet_t * packet)
{
    last_ack.clsID = packet->UBX_ACK.clsID;
    last_ack.msgID = packet->UBX_ACK.msgID;
}

static void syshal_gps_process_nack_priv(UBX_Packet_t * packet)
{
    last_nack.clsID = packet->UBX_NACK.clsID;
    last_nack.msgID = packet->UBX_NACK.msgID;
}

static void syshal_gps_compute_checksum_priv(UBX_Packet_t * packet, uint8_t ck[2])
{
    ck[0] = ck[1] = 0;
    uint8_t * buffer = &packet->msgClass;

    /* Taken directly from Section 29 UBX Checksum of the u-blox8
     * receiver specification
     */
    for (unsigned int i = 0; i < 4; i++)
    {
        ck[0] = (uint8_t) (ck[0] + buffer[i]);
        ck[1] = (uint8_t) (ck[1] + ck[0]);
    }

    buffer = packet->payloadAndCrc;
    for (unsigned int i = 0; i < packet->msgLength; i++)
    {
        ck[0] = (uint8_t) (ck[0] + buffer[i]);
        ck[1] = (uint8_t) (ck[1] + ck[0]);
    }
}

static void syshal_gps_set_checksum_priv(UBX_Packet_t * packet)
{
    uint8_t ck[2];

    //assert(packet->msgLength <= UBX_MAX_PACKET_LENGTH);
    syshal_gps_compute_checksum_priv(packet, ck);
    packet->payloadAndCrc[packet->msgLength]   = ck[0];
    packet->payloadAndCrc[packet->msgLength + 1] = ck[1];
}

/**
 * @brief      Process the UART Rx buffer looking for any NACKS
 */
void syshal_gps_look_for_ack_or_nack_priv(void)
{
    UBX_Packet_t ubx_packet;
    int error;

    error = syshal_gps_parse_rx_buffer_priv(&ubx_packet);

    // Correct packet so process it
    if (GPS_UART_NO_ERROR == error)
    {
        if (UBX_IS_MSG(&ubx_packet, UBX_MSG_CLASS_ACK, UBX_MSG_ID_ACK_NACK))
        {
            syshal_gps_process_nack_priv(&ubx_packet);
        }
        else if (UBX_IS_MSG(&ubx_packet, UBX_MSG_CLASS_ACK, UBX_MSG_ID_ACK_ACK))
        {
            syshal_gps_process_ack_priv(&ubx_packet);
        }
    }
}

static bool syshal_gps_device_responsive_priv(void)
{
    // Check to see if GPS is responsive or not
    // Send a UBX-CFG-MSG request and look for a NACK response
    UBX_Packet_t ubx_packet;
    UBX_SET_PACKET_HEADER(&ubx_packet, UBX_MSG_CLASS_CFG, UBX_MSG_ID_CFG_MSG, sizeof(UBX_CFG_MSG_POLL_t));
    UBX_PAYLOAD(&ubx_packet, UBX_CFG_MSG_POLL)->msgClass = 0;
    UBX_PAYLOAD(&ubx_packet, UBX_CFG_MSG_POLL)->msgID = 0;

    // Clear any previous NACK we may have got
    last_nack.clsID = 0xFF;
    last_nack.msgID = 0xFF;

    // Empty the UART RX buffer
    syshal_uart_flush(UART_GPS);

    syshal_gps_send_packet_priv(&ubx_packet);

    uint32_t start_time = syshal_time_get_ticks_ms();
    while (syshal_time_get_ticks_ms() - start_time < M8N_TIMEOUT_MS)
    {
        syshal_gps_look_for_ack_or_nack_priv();

        // Look for a NACK response to this message
        if (last_nack.clsID == UBX_MSG_CLASS_CFG &&
            last_nack.msgID == UBX_MSG_ID_CFG_MSG)
        {
            last_nack.clsID = 0xFF;
            last_nack.msgID = 0xFF;
            return true;
        }
    }

    return false;
}

// Return 0 on CRC match
static int syshal_gps_check_checksum_priv(UBX_Packet_t * packet)
{
    uint8_t ck[2];

    //assert(packet->msgLength <= UBX_MAX_PACKET_LENGTH);
    syshal_gps_compute_checksum_priv(packet, ck);
    return (ck[0] == packet->payloadAndCrc[packet->msgLength] &&
            ck[1] == packet->payloadAndCrc[packet->msgLength + 1]) ? 0 : -1;
}

static int syshal_gps_parse_rx_buffer_priv(UBX_Packet_t * packet)
{
    uint32_t bytesInRxBuffer = syshal_uart_available(UART_GPS);

    // Check for minimum allowed message size
    if (bytesInRxBuffer < UBX_HEADER_AND_CRC_LENGTH)
        return GPS_UART_ERROR_INSUFFICIENT_BYTES;

    // Look for SYNC1 byte
    for (uint32_t i = 0; i < bytesInRxBuffer; ++i)
    {
        if (!syshal_uart_peek_at(UART_GPS, &packet->syncChars[0], 0))
            return GPS_UART_ERROR_INSUFFICIENT_BYTES;

        if (UBX_PACKET_SYNC_CHAR1 == packet->syncChars[0])
            goto label_sync_start;
        else
            syshal_uart_receive(UART_GPS, &packet->syncChars[0], 1); // remove this character
    }

    // No SYNC1 found
    return GPS_UART_ERROR_MISSING_SYNC1;

label_sync_start:

    // Get the next character and see if it is the expected SYNC2
    if (!syshal_uart_peek_at(UART_GPS, &packet->syncChars[1], 1))
        return GPS_UART_ERROR_INSUFFICIENT_BYTES;

    if (UBX_PACKET_SYNC_CHAR2 != packet->syncChars[1])
    {
        // Okay so SYNC1 is valid but SYNC2 is not
        // We should dispose of both to prevent the program locking
        uint8_t dumpBuffer[2];
        syshal_uart_receive(UART_GPS, &dumpBuffer[0], 2);
        return GPS_UART_ERROR_MISSING_SYNC2; // Invalid SYNC2
    }

    // Extract length field and check it is received fully
    uint8_t lengthLower, lengthUpper;

    if (!syshal_uart_peek_at(UART_GPS, &lengthLower, 4))
        return GPS_UART_ERROR_INSUFFICIENT_BYTES;

    if (!syshal_uart_peek_at(UART_GPS, &lengthUpper, 5))
        return GPS_UART_ERROR_INSUFFICIENT_BYTES;

    uint16_t payloadLength = (uint16_t)lengthLower | ((uint16_t)lengthUpper << 8);
    uint16_t totalLength = payloadLength + UBX_HEADER_AND_CRC_LENGTH;

    if (totalLength > UART_RX_BUF_SIZE)
    {
        // Message is too big to store so throw it all away
        syshal_uart_flush(UART_GPS);

        return GPS_UART_ERROR_MSG_TOO_BIG;
    }

    // Check message is fully received and is in the RX buffer
    if (totalLength > bytesInRxBuffer)
        return GPS_UART_ERROR_MSG_PENDING;

    // Message is okay, lets grab the lot and remove it from the buffer
    uint8_t * buffer = (uint8_t *) packet;

    if (UBX_HEADER_LENGTH != syshal_uart_receive(UART_GPS, buffer, UBX_HEADER_LENGTH))
        return GPS_UART_ERROR_INSUFFICIENT_BYTES;

    buffer = packet->payloadAndCrc; // Now lets get the payload

    uint32_t totalToRead = payloadLength + UBX_CRC_LENGTH;

    int uart_ret = syshal_uart_receive(UART_GPS, buffer, totalToRead);
    if (uart_ret < 0)
        return GPS_UART_ERROR_BACKEND;

    if (totalToRead != (uint32_t) uart_ret)
        return GPS_UART_ERROR_INSUFFICIENT_BYTES;

    // Compute CRC and return
    if (syshal_gps_check_checksum_priv(packet) != 0)
        return GPS_UART_ERROR_CHECKSUM;

    return GPS_UART_NO_ERROR;
}

/**
 * @brief      Determines if the GPS module is present
 *
 * @return     true if detected
 * @return     false if not detected
 */
bool syshal_gps_is_present(void)
{
    syshal_gpio_set_output_high(SYSHAL_GPS_GPIO_POWER_ON);
    bool present = syshal_gps_device_responsive_priv();

    // If device was asleep then return it to that state
    if (STATE_ASLEEP == state)
    {
        syshal_time_delay_ms(GPS_RESTART_TIME_MS);
        syshal_gpio_set_output_low(SYSHAL_GPS_GPIO_POWER_ON);
    }

    return present;
}

syshal_gps_state_t syshal_gps_get_state(void)
{
    return state;
}