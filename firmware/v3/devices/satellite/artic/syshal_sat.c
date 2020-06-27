/* syshal_sat.c - Abstraction layer for satallite comms
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

#include <stdint.h>
#include <string.h>

#include "syshal_spi.h"
#include "syshal_gpio.h"
#include "syshal_sat.h"
#include "syshal_time.h"
#include "syshal_rtc.h"
#include "prepas.h"
#include "ARTIC.h"
#include "bsp.h"
#include "debug.h"

#define SYSHAL_SAT_ARTIC_DELAY_TICK_INTERRUPT_MS 10
#define INVALID_MEM_SELECTION (0xFF)

static syshal_sat_config_t config;

// Internal function prototypes
static int send_fw_files(fs_handle_t handle, firmware_header_t *firmware_header);
static int burst_access(mem_id_t mode, uint32_t start_address, const uint8_t *tx_data, uint8_t *rx_data, size_t size, bool read);
static int send_burst(const uint8_t *tx_data, uint8_t *rx_data, size_t size, uint8_t length_transfer, bool read);
static int configure_burst(mem_id_t mode, bool read, uint32_t start_address);
static int spi_read(mem_id_t mode, uint32_t start_address, uint8_t *buffer_read, size_t size);
static int clear_interrupt(uint8_t interrupt_num);
static int send_command(uint8_t command);
static int check_crc(firmware_header_t firmware_header);
static inline mem_id_t convert_mode(uint8_t mem_sel);
static inline uint8_t convert_mem_sel(mem_id_t mode);
static void reverse_memcpy(uint8_t *dst, const uint8_t *src, size_t size);

#ifndef DEBUG_DISABLED
static int print_status(void);

static const char *const status_string[] =
{
    "IDLE",                                   //The firmware is idle and ready to accept commands.
    "RX_IN_PROGRESS",                         // The firmware is receiving.
    "TX_IN_PROGRESS",                         // The firmware is transmitting.
    "BUSY",                                   // The firmware is changing state.
    //      Interrupt 1 flags
    "RX_VALID_MESSAGE",                       // A message has been received.
    "RX_SATELLITE_DETECTED",                  // A satellite has been detected.
    "TX_FINISHED"                            // The transmission was completed.
    "MCU_COMMAND_ACCEPTED",                   // The configuration command has been accepted.
    "CRC_CALCULATED",                         // CRC calculation has finished.
    "IDLE_STATE",                             // Firmware returned to the idle state.
    "RX_CALIBRATION_FINISHED",                // RX offset calibration has completed.
    "RESERVED_11",                            //
    "RESERVED_12",                            //
    //      Interrupt 2 flags
    "RX_TIMEOUT",                             // The specified reception time has been exceeded.
    "SATELLITE_TIMEOUT",                      // No satellite was detected within the specified time.
    "RX_BUFFER_OVERFLOW",                     // A received message is lost. No buffer space left.
    "TX_INVALID_MESSAGE",                     // Incorrect TX payload length specified.
    "MCU_COMMAND_REJECTED",                   // Incorrect command send or Firmware is not in idle.
    "MCU_COMMAND_OVERFLOW",                   // Previous command was not yet processed.
    "RESERVED_19",                            //
    "RESERVER_20",                            //
    // Others
    "INTERNAL_ERROR",                         // An internal error has occurred.
    "dsp2mcu_int1",                           // Interrupt 1 pin status
    "dsp2mcu_int2",
};
#endif

static enum
{
    STATE_UNINITIALIZED,
    STATE_INITIALIZED,
    STATE_ON,
    STATE_PROGRAMMED,
} sat_state = STATE_UNINITIALIZED;

#ifdef GTEST
void reset_state(void) {sat_state = STATE_UNINITIALIZED;}
void force_programmed(void) {sat_state = STATE_PROGRAMMED;}
#endif

/*! \brief Internal Function to convert from mem_sel 0,1,2,3 to mem_id_t
 *         Only change the order of the number
 */
static inline uint8_t convert_mem_sel(mem_id_t mode)
{
    switch (mode)
    {
        case PMEM:
            return 0;
            break;
        case XMEM:
            return 1;
            break;
        case YMEM:
            return 2;
            break;
        case IOMEM:
            return 3;
            break;
        default:
            return INVALID_MEM_SELECTION;
            break;
    }
}

/*! \brief Internal Function to convert from mem_sel 0,1,2,3 to mem_id_t
 *         Only change the order of the number
 */
static inline mem_id_t convert_mode(uint8_t mem_sel)
{
    switch (mem_sel)
    {
        case 0:
            return PMEM;
            break;
        case 1:
            return XMEM;
            break;
        case 2:
            return YMEM;
            break;
        case 3:
            return IOMEM;
            break;
        default:
            return INVALID_MEM;
            break;
    }
}

/*! \brief Internal Function to copy in the reverse order
 *
 * \param dst[out] ptr to the destination
 * \param src[in] ptr to the source
 * \param size[in] number of bytes to copy
 *
 */
static void reverse_memcpy(uint8_t *dst, const uint8_t *src, size_t size)
{
    for (uint32_t i = 0; i < size; ++i)
        dst[i] = src[size - 1 - i];
}

/*! \brief Internal Function to send basic artic command
 *
 * \param cmd[in] type of command.
 * \param response[out] pte to store the response
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR...
 */
static int send_artic_command(artic_cmd_t cmd, uint32_t *response)
{
    uint8_t buffer_rx[4] = {0};
    uint8_t buffer_tx[4] = {0};

    switch (cmd)
    {
        case DSP_STATUS:
            buffer_tx[0] = ARTIC_READ_ADDRESS(ADDRESS_DSP);
            break;

        case DSP_CONFIG:
            buffer_tx[0] = ARTIC_WRITE_ADDRESS(ADDRESS_DSP);
            break;

        default:
            return SYSHAL_SAT_ERROR_INVALID_PARAM;
    }

    int ret = syshal_spi_transfer(SPI_SATALLITE, buffer_tx, buffer_rx, 4);
    if (ret)
        return ret;

    if (response)
        *response = (buffer_rx[1] << 16) | (buffer_rx[2] << 8) | buffer_rx[3];

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal Function to program artic device from a file
 *
 * \param handle[in] handle to the firmware ARTIC file.
 * \param firmware_header[out] struct to store lenght and CRC read from the file
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_FILE if the bytes read doesn't match with the expected
 */
static int send_fw_files(fs_handle_t handle, firmware_header_t *firmware_header)
{
    uint8_t buffer[MAXIMUM_READ_FIRMWARE_OPERATION];
    uint32_t bytes_read;

    // Read the header of the firmware file
    fs_read(handle, firmware_header, sizeof(firmware_header_t), &bytes_read);
    if (bytes_read != sizeof(firmware_header_t))
        return SYSHAL_SAT_ERROR_FILE;

    mem_id_t order_mode[NUM_FIRMWARE_FILES_ARTIC] = {XMEM, YMEM, PMEM};
    for (uint8_t mem_sel = 0; mem_sel < NUM_FIRMWARE_FILES_ARTIC; mem_sel++)
    {
        int ret;
        uint32_t size;
        uint8_t length_transfer;
        uint8_t rx_buffer[4];
        uint32_t start_address = 0;
        uint8_t write_buffer[MAX_BURST];
        uint32_t bytes_written = 0;
        uint32_t last_address = 0;
        uint32_t bytes_total_read = 0;
        uint32_t address;
        uint32_t data;
        mem_id_t mode;

        mode = order_mode[mem_sel];

        // select number of transfer needed and the size of each transfer
        switch (mode)
        {
            case PMEM:
                size = firmware_header->PMEM_length;
                length_transfer = SIZE_SPI_REG_PMEM;
                break;
            case XMEM:
                size = firmware_header->XMEM_length;
                length_transfer = SIZE_SPI_REG_XMEM_YMEM_IOMEM;
                break;
            case YMEM:
                size = firmware_header->YMEM_length;
                length_transfer = SIZE_SPI_REG_XMEM_YMEM_IOMEM;
                break;
            default:
                return SYSHAL_SAT_ERROR_DEBUG;
                break;
        }

        while (bytes_total_read < size)
        {
            // Read 3 bytes of address and 3/4 bytes of data
            ret = fs_read(handle, buffer, FIRMWARE_ADDRESS_LENGTH + length_transfer, &bytes_read);
            if (ret || (bytes_read != (uint8_t)( FIRMWARE_ADDRESS_LENGTH + length_transfer)))
                return SYSHAL_SAT_ERROR_FILE;

            // Check next address and next data
            address = 0;
            data = 0;
            memcpy(&address, buffer, FIRMWARE_ADDRESS_LENGTH);
            memcpy(&data, buffer + FIRMWARE_ADDRESS_LENGTH, length_transfer);

            // Sum bytes read from the file.
            bytes_total_read += FIRMWARE_ADDRESS_LENGTH + length_transfer;

            // If there is a memory discontinuity or the buffer is full send the whole buffer
            if (last_address + 1 < address || (bytes_written + length_transfer) >= MAX_BURST)
            {
                // Configure and send the buffer content
                ret = burst_access(mode, start_address, write_buffer, rx_buffer, bytes_written, false );
                if (ret)
                    return ret;
                start_address = address;
                bytes_written = 0;
            }

            // Copy next data in the buffer
            reverse_memcpy(&(write_buffer[bytes_written]), (uint8_t *) &data, length_transfer);
            last_address = address;
            bytes_written += length_transfer;
            data = 0;
        }

        // If there is data to be sent, (it has to be, otherwise the number of data is multiple of MAX_BURST.
        if (bytes_written > 0)
        {
            // Wait few mellisecond to continue the operations 13 ms, Just in case we send very small amount of bytes
            syshal_time_delay_ms(SYSHAL_SAT_ARTIC_DELAY_FINISH_BURST);

            ret = burst_access(mode, start_address, write_buffer, rx_buffer, bytes_written, false);
            if (ret)
                return ret;
        }
    }

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal Function to setup a burst transfer
 *
 * \param mode[in] Register to send could be XMEM, YMEM, PMEM or IOMEM.
 * \param read[in] select mode: 1 read 0 write
 * \param start_address[in]  Start address to read.
 *
 * \return \ref SYSHAL_CELLULAR_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_NOT_INIT program firmware called before syshal_sat_init.
 * \return \ref SYSHAL_SAT_ERROR_INT_TIMEOUT Interrupt has not been fired.
 */
static int configure_burst(mem_id_t mode, bool read, uint32_t start_address)
{
    uint8_t buffer_rx[4] = {0};
    uint8_t buffer_tx[4] = {0};
    uint32_t burst_reg = 0;
    uint8_t mem_sel;
    int ret;

    mem_sel = convert_mem_sel(mode);
    if (INVALID_MEM_SELECTION == mem_sel)
        return SYSHAL_SAT_ERROR_INCORRECT_MEM_SEL;

    // Config burst register
    burst_reg |= BURST_MODE_SHIFT_BITMASK;
    burst_reg |= ((mem_sel << MEM_SEL_SHIFT) & MEM_SEL_BITMASK);
    if (read)
        burst_reg |= BURST_R_NW_MODE_BITMASK;

    burst_reg |= (start_address & BURST_START_ADDRESS_BITMASK);

    buffer_tx[0] = ARTIC_WRITE_ADDRESS(BURST_ADDRESS);
    buffer_tx[1] = burst_reg >> 16;
    buffer_tx[2] = burst_reg >> 8;
    buffer_tx[3] = burst_reg;

    ret = syshal_spi_transfer(SPI_SATALLITE, buffer_tx, buffer_rx, 4);
    if (ret)
        return ret;

    syshal_time_delay_ms(SYSHAL_SAT_ARTIC_DELAY_SET_BURST);

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal Function to configure and send a burst trasnfer
 *
 * \param mode[in] Register to send could be XMEM, YMEM, PMEM or IOMEM.
 * \param start_address[in] Start address to read.
 * \param tx_data[in] pointer to the array to the data to transmit.
 * \param rx_data[out] pointer to the array to stored the result
 * \param size[in] total number of bytes to transmit
 * \param read[in] select mode: 1 read 0 write
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR
 */
static int burst_access(mem_id_t mode, uint32_t start_address, const uint8_t *tx_data, uint8_t *rx_data, size_t size, bool read )
{
    uint8_t length_transfer;
    int ret;

    // Set burst register to configure a burst transfer
    ret = configure_burst(mode, read, start_address);
    if (ret)
        return ret;

    
    if (mode == PMEM) // PMEM is a 4 byte register
        length_transfer = 4;
    else // The rest are 3
        length_transfer = 3;

    ret = send_burst(tx_data, rx_data, size, length_transfer, read);
    if (ret)
        return ret;

    // Deactivate SSN pin
    syshal_spi_finish_transfer(SPI_SATALLITE);
    syshal_time_delay_ms(SYSHAL_SAT_ARTIC_DELAY_FINISH_BURST);

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal Function to a burst spi transfer
 *
 * Send data without realising the ss_pin, control manually the time betwen transfer
 * The device has to stop 50us after each transfer and 5ms after 60 bytes transmitted
 *
 * \param tx_data[in] pointer to the array to the data to transmit.
 * \param rx_data[out] pointer to the array to stored the result
 * \param size[in] total number of bytes to transmit
 * \param length_transfer[in] number of bytes to trasnmit every transfer
 * \param read[in] select mode: 1 read 0 write
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR
 */
static int send_burst(const uint8_t *tx_data, uint8_t *rx_data, size_t size, uint8_t length_transfer, bool read)
{
    uint16_t num_transfer = size / length_transfer;

    // Write in chunk of 60 bytes and wait 5 ms
    uint32_t delay_count = 0;
    uint8_t buffer[SIZE_SPI_REG_PMEM];
    int ret;

    // buffer for saving memory when not needed
    for (int i = 0; i < SIZE_SPI_REG_PMEM; ++i)
    {
        buffer[i] = 0x00;
    }

    if (read)
    {
        for (uint32_t i = 0; i < num_transfer; ++i)
        {
            ret = syshal_spi_transfer_continous(SPI_SATALLITE, buffer, &rx_data[i * length_transfer], length_transfer);
            if (ret)
                return ret;

            delay_count += length_transfer;
            syshal_time_delay_us(SYSHAL_SAT_ARTIC_DELAY_TRANSFER);
            if (delay_count > NUM_BYTES_BEFORE_WAIT)
            {
                delay_count = 0;
                syshal_time_delay_ms(SYSHAL_SAT_ARTIC_DELAY_BURST);
            }

        }
    }
    else
    {
        for (uint32_t i = 0; i < num_transfer; ++i)
        {
            ret = syshal_spi_transfer_continous(SPI_SATALLITE, &tx_data[i * length_transfer], buffer, length_transfer);
            if (ret)
                return ret;
            delay_count += length_transfer;
            syshal_time_delay_us(SYSHAL_SAT_ARTIC_DELAY_TRANSFER);
            if (delay_count > NUM_BYTES_BEFORE_WAIT)
            {
                delay_count = 0;
                syshal_time_delay_ms(SYSHAL_SAT_ARTIC_DELAY_BURST);
            }
        }
    }

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal Function to read from spi
 *
 * \param mode[in] Register to send could be XMEM, YMEM, PMEM or IOMEM.
 * \param start_address[in] Start address to read.
 * \param buffer_read[out] pointer to the array to stored the result
 * \param size[in] total number of bytes to read

 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_SPI ARTIC device only can read 2048bytes per transfer.
 * \return \ref SYSHAL_SAT_ERROR....
 */
static int spi_read(mem_id_t mode, uint32_t start_address, uint8_t *buffer_read, size_t size)
{
    uint32_t index;
    int ret;

    if (size > SPI_MAX_BYTE_READ)
        return SYSHAL_SAT_ERROR_SPI;

    // Clear buffer_read
    for (index = 0; index < size; index++)
    {
        buffer_read[index] = 0;
    }

    ret = burst_access(mode, start_address, NULL, buffer_read, size, true);
    if (ret)
        return ret;

    return SYSHAL_SAT_NO_ERROR;
}


#ifndef DEBUG_DISABLED

/*! \brief Internal Function to print the contents of the status register
 *
 * \return \ref SYSHAL_CELLULAR_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR... .
 */
static int print_status(void)
{
    uint32_t status;
    int ret = syshal_sat_get_status_register(&status);
    if (ret)
        return ret;

    for (int i = 0; i < TOTAL_NUMBER_STATUS_FLAG; ++i)
    {
        if (status & (1 << i))
            DEBUG_PR_TRACE("%s", (char *)status_string[i]);
    }
    return SYSHAL_SAT_NO_ERROR;
}

#endif

/*! \brief Internal Function to clear the interrupt register
 *
 * \param interrupt_num[in] 1 or 2, which correspond to the 2 interrupt GPIO artic has
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR
 */
static int clear_interrupt(uint8_t interrupt_num)
{
    int ret;

    switch (interrupt_num)
    {
        case 1:
            ret = send_command(ARTIC_CMD_CLEAR_INT1);
            return ret;

        case 2:
            ret = send_command(ARTIC_CMD_CLEAR_INT2);
            return ret;

        default:
            return SYSHAL_SAT_ERROR_INTERRUPT;
    }
}

/*! \brief Internal Function to send basic artic command
 *
 * \param command[in] ARTIC command
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_SPI SPI error performing the spi transfer
 */
static int send_command(uint8_t command)
{
    uint8_t buffer_read;
    int ret;

    ret = syshal_spi_transfer(SPI_SATALLITE, &command, &buffer_read, sizeof(command));

    if (ret)
        return SYSHAL_SAT_ERROR_SPI;

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal Function to wait for an interrupt
 *
 * \param timeout[in] maximum time to wait for the interrupt.
 * \param interrupt_num[in] 1 or 2, which correspond to the 2 interrupt GPIO artic has
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_INT_TIMEOUT timeour expired.
 */
static int wait_interrupt(uint32_t timeout, uint8_t interrupt_num)
{
    uint32_t init_time;
    uint32_t time_running = 0;
    uint8_t gpio_port;
    bool int_status;

    if (interrupt_num == 1)
        gpio_port = SYSHAL_SAT_GPIO_INT_1;
    else
        gpio_port = SYSHAL_SAT_GPIO_INT_2;

    init_time = syshal_time_get_ticks_ms();
    do
    {
        syshal_time_delay_ms(SYSHAL_SAT_ARTIC_DELAY_TICK_INTERRUPT_MS);
        time_running =  syshal_time_get_ticks_ms() - init_time;
        int_status = syshal_gpio_get_input(gpio_port);
    }
    while (time_running < timeout && !int_status);

    if (int_status == false)
        return SYSHAL_SAT_ERROR_INT_TIMEOUT;

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal Function to check CRC
 *
 * \param firmware_header_t[in] struct which contains the precalculated CRC
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_CRC One CRC doesnt match with the precalculated one.
 */
static int check_crc(firmware_header_t firmware_header)
{
    uint32_t crc;
    uint8_t crc_buffer[NUM_FIRMWARE_FILES_ARTIC * SIZE_SPI_REG_XMEM_YMEM_IOMEM]; // 3 bytes CRC for each firmware file
    int ret;

    ret = spi_read(XMEM, CRC_ADDRESS, crc_buffer, NUM_FIRMWARE_FILES_ARTIC * SIZE_SPI_REG_XMEM_YMEM_IOMEM ); // 3 bytes CRC for each firmware local_file_id
    if (ret)
        return ret;

    // Check CRC Value PMEM
    reverse_memcpy((uint8_t *)&crc, crc_buffer, SIZE_SPI_REG_XMEM_YMEM_IOMEM);
    if (firmware_header.PMEM_CRC != crc)
    {
        DEBUG_PR_ERROR("PMEM CRC 0x%08lX DOESN'T MATCH EXPECTED 0x%08lX", crc, firmware_header.PMEM_CRC);
        return SYSHAL_SAT_ERROR_CRC;
    }

    // Check CRC Value XMEM
    crc = 0;
    reverse_memcpy((uint8_t *)&crc, &crc_buffer[SIZE_SPI_REG_XMEM_YMEM_IOMEM], SIZE_SPI_REG_XMEM_YMEM_IOMEM);
    if (firmware_header.XMEM_CRC != crc)
    {
        DEBUG_PR_ERROR("PMEM CRC 0x%08lX DOESN'T MATCH EXPECTED 0x%08lX", crc, firmware_header.PMEM_CRC);
        return SYSHAL_SAT_ERROR_CRC;
    }

    // Check CRC Value YMEM
    reverse_memcpy((uint8_t *)&crc, &crc_buffer[SIZE_SPI_REG_XMEM_YMEM_IOMEM * 2], SIZE_SPI_REG_XMEM_YMEM_IOMEM);
    if (firmware_header.YMEM_CRC != crc)
    {
        DEBUG_PR_ERROR("PMEM CRC 0x%08lX DOESN'T MATCH EXPECTED 0x%08lX", crc, firmware_header.PMEM_CRC);
        return SYSHAL_SAT_ERROR_CRC;
    }

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal function to agregate the smple comunication with the DSP
 *
 *  Function to agregate all the functionality involve on leaning the interrupt, sending a specific command, wait for the interrupt
 *  Check if the status has the expected flag with the expected value and clean the interrupt.
 *
 * \param command[in] specific command to send.
 * \param interrupt_number[in] 1 or 2, which correspond to the 2 interrupt GPIO artic has
 * \param status_flag_number[in] status flag to check
 * \param value[in] expected value for the flag expected
 * \param interrupt_timeout[in] maximum time to wait for the interrupt.
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_STATUS The expected status flag value doesn't match with the read in the status register.
 * \return \ref SYSHAL_SAT_ERROR.
 */
static int send_command_check_clean(uint8_t command, uint8_t interrupt_number, uint8_t status_flag_number, bool value, uint32_t interrupt_timeout)
{
    uint32_t status = 0;
    int ret;

    ret = clear_interrupt(interrupt_number);
    if (ret)
        return ret;

    ret = send_command(command);
    if (ret)
        return ret;

    ret = wait_interrupt(interrupt_timeout, interrupt_number);
    if (ret)
        return ret;

    ret = syshal_sat_get_status_register(&status);
    if (ret)
        return ret;

    if ((status & (1 << status_flag_number)) == !value)
        return SYSHAL_SAT_ERROR_STATUS;

    ret = clear_interrupt(interrupt_number);
    if (ret)
        return ret;

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal Function to initialize the syshal_sat layer
 *
 *  Function to initialize GPIO ports and copy in the layer iot_prepass_sats_t
 *
 * \param iot_prepass_sats_t[in] syshal_sat configuration
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 */
int syshal_sat_init(syshal_sat_config_t sat_config)
{
    config = sat_config;

    sat_state = STATE_INITIALIZED;

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Function to power on ARTIC device
 *
 * \param fs[in] Reference to the file system.
 * \param local_file_id[in] local_file_id which identified the location of ARTIC software.
 *
 * \return \ref SYSHAL_CELLULAR_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_NOT_INIT program firmware called before syshal_sat_init.
 */
int syshal_sat_power_on(void)
{
    if (sat_state < STATE_INITIALIZED)
        return SYSHAL_SAT_ERROR_NOT_INIT;

    syshal_gpio_init(SYSHAL_SAT_GPIO_RESET);
    syshal_gpio_init(SYSHAL_SAT_GPIO_INT_1);
    syshal_gpio_init(SYSHAL_SAT_GPIO_INT_2);
    syshal_gpio_init(SYSHAL_SAT_GPIO_POWER_ON);

    syshal_gpio_set_output_high(SYSHAL_SAT_GPIO_POWER_ON);

    // Power off Artic device
    syshal_gpio_set_output_low(SYSHAL_SAT_GPIO_RESET);

    // Wait few ms until the device is already power off.
    syshal_time_delay_ms(SYSHAL_SAT_ARTIC_DELAY_RESET);

    // Power on the device to enter in boot mode.
    syshal_gpio_set_output_high(SYSHAL_SAT_GPIO_RESET);

    sat_state = STATE_ON;
    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Function to power off the ARTIC device
 *
 *  Function to power off the artic device
 *
 * \return \ref SYSHAL_CELLULAR_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_NOT_INIT IF the syshal_sat havent been initialize
 */
int syshal_sat_power_off(void)
{
    if (sat_state < STATE_INITIALIZED)
        return SYSHAL_SAT_ERROR_NOT_INIT;

#ifndef GTEST
    // Set the now unused pins to high impedance to save power
    nrf_gpio_cfg_default(GPIO_Inits[SYSHAL_SAT_GPIO_RESET].pin_number);
    nrf_gpio_cfg_default(GPIO_Inits[SYSHAL_SAT_GPIO_INT_1].pin_number);
    nrf_gpio_cfg_default(GPIO_Inits[SYSHAL_SAT_GPIO_INT_2].pin_number);
    nrf_gpio_cfg_default(GPIO_Inits[SYSHAL_SAT_GPIO_POWER_ON].pin_number);
#endif

    sat_state = STATE_INITIALIZED;
    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Function to program firmware of ARTIC
 *
 *  Function to reset the device in boot mode, read a file with the content of the three
 *  part of the software, and program the artic device through SPI.
 *
 * \param fs[in] Reference to the file system.
 * \param local_file_id[in] local_file_id which identified the location of ARTIC software.
 *
 * \return \ref SYSHAL_CELLULAR_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_NOT_INIT program firmware called before syshal_sat_init.
 * \return \ref SYSHAL_SAT_ERROR_INT_TIMEOUT Interrupt has not been fired.
 */
int syshal_sat_program_firmware(fs_t fs, uint32_t local_file_id)
{
    fs_handle_t handle;
    firmware_header_t firmware_header;
    uint32_t artic_response = 0;
    int ret;

    if (sat_state < STATE_ON)
        return SYSHAL_SAT_ERROR_NOT_POWERED_ON;

    DEBUG_PR_TRACE("WAIT DSP IN RESET");

    // Wait until the device's status register contains 85
    // NOTE: This 85 value is undocumentated but can be seen in the supplied Artic_evalboard.py file
    int retries = SYSHAL_SAT_MAX_RETRIES;
    do
    {
        syshal_time_delay_ms(SYSHAL_SAT_ARTIC_DELAY_BOOT);

        ret = send_artic_command(DSP_STATUS, &artic_response);
        if (ret)
            return ret;

        if (artic_response == 85)
            break;

        retries--;
    }
    while (retries >= 0);

    if (retries < 0)
    {
        DEBUG_PR_ERROR("ARTIC ERROR BOOT");
        return SYSHAL_SAT_ERROR_BOOT;
    }

    // Open firmware file
    DEBUG_PR_TRACE("Open firmware file");
    ret = fs_open(fs, &handle, local_file_id, FS_MODE_READONLY, NULL);
    if (ret)
        return SYSHAL_SAT_ERROR_FILE;

    DEBUG_PR_TRACE("Uploading firmware to ARTIC");
    ret = send_fw_files(handle, &firmware_header);
    fs_close(handle);
    if (ret)
        return ret;

    // Bring DSP out of reset
    DEBUG_PR_TRACE("Bringing Artic out of reset");
    ret = send_artic_command(DSP_CONFIG, NULL);
    if (ret)
        return ret;

    DEBUG_PR_TRACE("Waiting for interrupt 1");

    // Interrupt 1 will be high when start-up is complete
    ret = wait_interrupt(SYSHAL_SAT_ARTIC_DELAY_INTERRUPT, INTERRUPT_1);
    if (ret)
        return SYSHAL_SAT_ERROR_INT_TIMEOUT;

    DEBUG_PR_TRACE("Artic booted");

    ret = clear_interrupt(INTERRUPT_1);
    if (ret)
        return SYSHAL_SAT_ERROR_INTERRUPT;

    if (ret)
        return SYSHAL_SAT_ERROR_SPI;

    DEBUG_PR_TRACE("Checking CRC values");
    ret = check_crc(firmware_header);
    if (ret)
        return ret;

    sat_state = STATE_PROGRAMMED;
    return SYSHAL_SAT_NO_ERROR;
}

static void construct_PPT_A3_header(uint8_t *buffer, uint32_t *index, uint32_t total_len_bits, uint8_t len_bitmask)
{
    buffer[0] = total_len_bits >> 16;
    buffer[1] = total_len_bits >> 8;
    buffer[2] = total_len_bits;

    buffer[3] = len_bitmask | (config.artic->contents.device_identifier & ARTIC_MSG_ID_BITMASK) >> 24;
    buffer[4] = (config.artic->contents.device_identifier & ARTIC_MSG_ID_BITMASK) >> 16;
    buffer[5] = (config.artic->contents.device_identifier & ARTIC_MSG_ID_BITMASK) >> 8;
    buffer[6] = (config.artic->contents.device_identifier & ARTIC_MSG_ID_BITMASK);

    *index += 7;
}

static void construct_ZTE_header(uint8_t *buffer, uint32_t total_len_bits)
{
    buffer[0] = total_len_bits >> 16;
    buffer[1] = total_len_bits >> 8;
    buffer[2] = total_len_bits;

    buffer[3] = (config.artic->contents.device_identifier & ARTIC_MSG_ID_BITMASK) >> 24;
    buffer[4] = (config.artic->contents.device_identifier & ARTIC_MSG_ID_BITMASK) >> 16;
    buffer[5] = (config.artic->contents.device_identifier & ARTIC_MSG_ID_BITMASK) >> 8;
    buffer[6] = (config.artic->contents.device_identifier & ARTIC_MSG_ID_BITMASK);
}

static void construct_artic_message_buffer(const uint8_t *buffer, size_t buffer_size, uint8_t *send_buffer, uint32_t *bytes_to_send)
{
    uint32_t index = 0;
    *bytes_to_send = 0;

    memset(send_buffer, 0, ARTIC_MSG_MAX_SIZE);

    if (buffer_size <= ARTIC_ZTE_MAX_USER_BYTES)
    {
        *bytes_to_send = ARTIC_ZTE_BYTES_TO_SEND;
        construct_ZTE_header(send_buffer, ARTIC_ZTE_MSG_TOTAL_BITS);
    }
    else if (buffer_size <= ARTIC_PTT_A3_24_MAX_USER_BYTES)
    {
        *bytes_to_send = ARTIC_PTT_A3_24_BYTES_TO_SEND;
        construct_PPT_A3_header(send_buffer, &index, ARTIC_PTT_A3_24_MSG_TOTAL_BITS, ARTIC_PTT_A3_24_MSG_LEN_FIELD);
    }
    else if (buffer_size <= ARTIC_PTT_A3_56_MAX_USER_BYTES)
    {
        *bytes_to_send = ARTIC_PTT_A3_56_BYTES_TO_SEND;
        construct_PPT_A3_header(send_buffer, &index, ARTIC_PTT_A3_56_MSG_TOTAL_BITS, ARTIC_PTT_A3_56_MSG_LEN_FIELD);
    }
    else if (buffer_size <= ARTIC_PTT_A3_88_MAX_USER_BYTES)
    {
        *bytes_to_send = ARTIC_PTT_A3_88_BYTES_TO_SEND;
        construct_PPT_A3_header(send_buffer, &index, ARTIC_PTT_A3_88_MSG_TOTAL_BITS, ARTIC_PTT_A3_88_MSG_LEN_FIELD);
    }
    else if (buffer_size <= ARTIC_PTT_A3_120_MAX_USER_BYTES)
    {
        *bytes_to_send = ARTIC_PTT_A3_120_BYTES_TO_SEND;
        construct_PPT_A3_header(send_buffer, &index, ARTIC_PTT_A3_120_MSG_TOTAL_BITS, ARTIC_PTT_A3_120_MSG_LEN_FIELD);
    }
    else if (buffer_size <= ARTIC_PTT_A3_152_MAX_USER_BYTES)
    {
        *bytes_to_send = ARTIC_PTT_A3_152_BYTES_TO_SEND;
        construct_PPT_A3_header(send_buffer, &index, ARTIC_PTT_A3_152_MSG_TOTAL_BITS, ARTIC_PTT_A3_152_MSG_LEN_FIELD);
    }
    else if (buffer_size <= ARTIC_PTT_A3_184_MAX_USER_BYTES)
    {
        *bytes_to_send = ARTIC_PTT_A3_184_BYTES_TO_SEND;
        construct_PPT_A3_header(send_buffer, &index, ARTIC_PTT_A3_184_MSG_TOTAL_BITS, ARTIC_PTT_A3_184_MSG_LEN_FIELD);
    }
    else if (buffer_size <= ARTIC_PTT_A3_216_MAX_USER_BYTES)
    {
        *bytes_to_send = ARTIC_PTT_A3_216_BYTES_TO_SEND;
        construct_PPT_A3_header(send_buffer, &index, ARTIC_PTT_A3_216_MSG_TOTAL_BITS, ARTIC_PTT_A3_216_MSG_LEN_FIELD);
    }
    else if (buffer_size <= ARTIC_PTT_A3_248_MAX_USER_BYTES)
    {
        *bytes_to_send = ARTIC_PTT_A3_248_BYTES_TO_SEND;
        construct_PPT_A3_header(send_buffer, &index, ARTIC_PTT_A3_248_MSG_TOTAL_BITS, ARTIC_PTT_A3_248_MSG_LEN_FIELD);
    }

    // Copy the data payload into the message
    memcpy(&send_buffer[index], buffer, buffer_size);
}

/*! \brief Function to send a message using ARGOS A3 standard
 *
 * \param buffer[in] Array with the data to send.
 * \param  buffer_size[in] total bytes of data to send.
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_NOT_PROGRAMMED ARTIC device has to be programmed everytime before send data.
 * \return \ref SYSHAL_SAT_ERROR_MAX_SIZE Maximum size of user data is 31 bytes.
 * \return ref SYSHAL_SAT...
 */
int syshal_sat_send_message(const uint8_t *buffer, size_t buffer_size)
{
    uint8_t send_buffer[ARTIC_MSG_MAX_SIZE] = {0};
    uint32_t bytes_to_send;
    uint8_t artic_packet_type;
    int ret;

    if (sat_state != STATE_PROGRAMMED)
        return SYSHAL_SAT_ERROR_NOT_PROGRAMMED;

    if (buffer_size > MAX_TX_SIZE_BYTES)
        return SYSHAL_SAT_ERROR_MAX_SIZE;

    if (buffer_size == 0)
        artic_packet_type = ARTIC_CMD_SET_PTT_ZE_TX_MODE;
    else
        artic_packet_type = ARTIC_CMD_SET_PTT_A3_TX_MODE;

    // Set ARGOS TX MODE in ARTIC device and wait for the status response
    ret = send_command_check_clean(artic_packet_type, 1, MCU_COMMAND_ACCEPTED, true, SYSHAL_SAT_ARTIC_DELAY_INTERRUPT);
    if (ret)
        return ret;

    construct_artic_message_buffer(buffer, buffer_size, send_buffer, &bytes_to_send);

    for (size_t i = 0; i < bytes_to_send; ++i)
    {
        DEBUG_PR_TRACE("Byte[%u]: %02X", (unsigned int) i, (unsigned int) send_buffer[i]);
    }

    // It could be a problem if we set less data than we are already sending, just be careful and in case change TOTAL
    // Burst transfer the tx payload
    ret = burst_access(XMEM, TX_PAYLOAD_ADDRESS, send_buffer, NULL, bytes_to_send, false);
    if (ret)
        return SYSHAL_SAT_ERROR_SPI;

#ifndef DEBUG_DISABLED
    print_status();
#endif

    // Send to ARTIC the command for sending only one packet and wait for the response TX_FINISHED
    ret = send_command_check_clean(ARTIC_CMD_START_TX_1M_SLEEP, 1, TX_FINISHED, true, SYSHAL_SAT_ARTIC_TIMEOUT_SEND_TX);

    if (ret)
    {
        // If there is a problem wait until interrupt 2 is launched and get the status response
        if (!wait_interrupt(SYSHAL_SAT_ARTIC_DELAY_INTERRUPT, INTERRUPT_2))
        {
#ifndef DEBUG_DISABLED
            print_status();
#endif
            clear_interrupt(INTERRUPT_2);
            return SYSHAL_SAT_ERROR_TX;
        }
        else
        {
            return ret;
        }
    }

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Internal Function to read status register
 *
 * \param status[out] Contents of status register
 *
 * \return \ref SYSHAL_CELLULAR_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR... .
 */
int syshal_sat_get_status_register(uint32_t *status)
{
    int ret;
    uint8_t buffer[SIZE_SPI_REG_XMEM_YMEM_IOMEM];

    if (sat_state < STATE_ON)
        return SYSHAL_SAT_ERROR_NOT_POWERED_ON;

    ret = burst_access(IOMEM, INTERRUPT_ADDRESS, NULL, buffer, SIZE_SPI_REG_XMEM_YMEM_IOMEM,  true);
    if (ret)
        return ret;
    *status = 0;
    reverse_memcpy((uint8_t *) status, buffer, SIZE_SPI_REG_XMEM_YMEM_IOMEM);

    return SYSHAL_SAT_NO_ERROR;
}

/*! \brief Funtion to get the next time that a satellite will pass over the device
 *
 * \param iot_last_gps_location_t[in] Last gps location of the device
 * \param  iot_prepass_result_t[in] struct to store the result.
 *
 * \return \ref SYSHAL_SAT_NO_ERROR on success.
 * \return \ref SYSHAL_SAT_ERROR_NOT_INIT ARTIC device has to be initialized before.
 * \return ref SYSHAL_SAT...
 */
int syshal_sat_calc_prepass(const iot_last_gps_location_t *gps, iot_prepass_result_t *result)
{
    if (sat_state < STATE_INITIALIZED)
        return SYSHAL_SAT_ERROR_NOT_INIT;

    uint8_t num_sats = 0;
    while (num_sats < SYSHAL_SAT_MAX_NUM_SATS)
    {
        if (config.artic->contents.bulletin_data[num_sats].sat[0] || config.artic->contents.bulletin_data[num_sats].sat[1])
            num_sats++;
        else
            break;
    }

    *result = next_predict(config.artic->contents.bulletin_data,(float) config.artic->contents.min_elevation, num_sats, gps->longitude, gps->latitude, gps->timestamp);

    return SYSHAL_SAT_NO_ERROR;
}
