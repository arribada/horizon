// test_syshal_sat.cpp - syshal_sat unit tests
//
// Copyright (C) 2019 Arribada
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

extern "C" {
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "unity.h"
#include "Mocksyshal_uart.h"
#include "fs.h"
#include "fs_priv.h"
#include "at.h"
#include "syshal_cellular.h"
#include "syshal_sat.h"
#include "prepas.h"
#include "ARTIC.h"
#include "Mocksyshal_gpio.h"
#include "Mocksyshal_flash.h"
#include "Mocksyshal_time.h"
#include "Mocksyshal_rtc.h"
#include "Mocksyshal_spi.h"
#include "bsp.h"
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cmath>
#include <fstream>
#include <string>
using namespace std;
#define INTERNAL_ID 0xFAFAFAF
fs_t file_system;
#define FILE_ID_FIRWMWARE 1

#define MAX_READ 256

#define DEBUG

#define SYSHAL_SAT_ARTIC_DELAY_TICK_INTERRUPT_MS 10

// syshal_flash
#define FLASH_SIZE          (FS_PRIV_SECTOR_SIZE * FS_PRIV_MAX_SECTORS)

int total_number = 0;

const uint32_t crc_values[3] = {0x493982, 0x32DBBA, 0x65CC81}; // VERSION 4 ARTIC FIRMWARE CRC

sys_config_iot_sat_artic_settings_t artic_settings;
syshal_sat_config_t config = {&artic_settings};

char flash_ram[FLASH_SIZE];
int syshal_flash_init_callback(uint32_t drive, uint32_t device, int cmock_num_calls) {return SYSHAL_FLASH_NO_ERROR;}

int syshal_flash_read_callback(uint32_t device, void * dest, uint32_t address, uint32_t size, int cmock_num_calls)
{
    for (unsigned int i = 0; i < size; i++)
        ((char *)dest)[i] = flash_ram[address + i];

    return 0;
}

int syshal_flash_write_callback(uint32_t device, const void * src, uint32_t address, uint32_t size, int cmock_num_calls)
{
    for (unsigned int i = 0; i < size; i++)
    {
        /* Ensure no new bits are being set */
        if ((((char *)src)[i] & flash_ram[address + i]) ^ ((char *)src)[i])
        {
            printf("syshal_flash_write: Can't set bits from 0 to 1 (%08x: %02x => %02x)\n", address + i,
                   (uint8_t)flash_ram[address + i], (uint8_t)((char *)src)[i]);
            assert(0);
        }
        flash_ram[address + i] = ((char *)src)[i];
    }

    return 0;
}

int syshal_flash_erase_callback(uint32_t device, uint32_t address, uint32_t size, int cmock_num_calls)
{
    /* Make sure address is sector aligned */
    if (address % FS_PRIV_SECTOR_SIZE || size % FS_PRIV_SECTOR_SIZE)
    {
        printf("syshal_flash_erase: Non-aligned address %08x", address);
        assert(0);
    }

    for (unsigned int i = 0; i < size; i++)
        flash_ram[address + i] = 0xFF;

    return 0;
}

class SatTest : public ::testing::Test
{
public:

    virtual void SetUp()
    {
        Mocksyshal_uart_Init();
        total_number = 0;
        Mocksyshal_gpio_Init();
        Mocksyshal_time_Init();
        Mocksyshal_rtc_Init();
        Mocksyshal_spi_Init();

        syshal_rtc_soft_watchdog_refresh_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);
        syshal_rtc_date_time_to_timestamp_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);


        syshal_time_delay_ms_Ignore();
        syshal_time_delay_us_Ignore();

        syshal_gpio_init_IgnoreAndReturn(SYSHAL_GPIO_NO_ERROR);

        // syshal_flash
        syshal_flash_init_StubWithCallback(syshal_flash_init_callback);
        syshal_flash_read_StubWithCallback(syshal_flash_read_callback);
        syshal_flash_write_StubWithCallback(syshal_flash_write_callback);
        syshal_flash_erase_StubWithCallback(syshal_flash_erase_callback);

        memset(flash_ram, 0xFF, sizeof(flash_ram));
        EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
        EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
        //EXPECT_EQ(FS_NO_ERROR, fs_delete(file_system, FILE_ID_FIRWMWARE));

    }

    virtual void TearDown()
    {
        reset_state();
        Mocksyshal_uart_Verify();
        Mocksyshal_uart_Destroy();
        Mocksyshal_gpio_Verify();
        Mocksyshal_gpio_Destroy();
        Mocksyshal_time_Verify();
        Mocksyshal_time_Destroy();
        Mocksyshal_rtc_Verify();
        Mocksyshal_rtc_Destroy();
        Mocksyshal_spi_Verify();
        Mocksyshal_spi_Destroy();
    }
public:

    void fill_internal_prepas(void)
    {
        artic_settings.contents.device_identifier = INTERNAL_ID;
        artic_settings.contents.min_elevation = 45;

        FILE    *ul_bull;       /* orbit file       */
        po  *pt_po;             /* pointer on tab_po            */
        pp  *pt_pp;             /* pointer on tab_pp            */

        int isat;

        std::string nf_bull = "../prepas/bulletin_syshal_sat.dat";

        char    ligne [MAXLU];

        /*-------- Read Orbit Parameter ---------*/
        struct tm t_bul;
        ul_bull = fopen(nf_bull.c_str(), "r");
        ASSERT_TRUE(ul_bull);

        time_t conver_time;
        fgets(ligne, MAXLU, ul_bull);
        sscanf(ligne, "%s%ld%f%f%f%f%f%f",
               &artic_settings.contents.bulletin_data[0].sat,       &artic_settings.contents.bulletin_data[0].time_bulletin,
               &artic_settings.contents.bulletin_data[0].params[0], &artic_settings.contents.bulletin_data[0].params[1], 
               &artic_settings.contents.bulletin_data[0].params[2], &artic_settings.contents.bulletin_data[0].params[3],
               &artic_settings.contents.bulletin_data[0].params[4], &artic_settings.contents.bulletin_data[0].params[5]);

        isat = 0;

        while (!feof(ul_bull))
        {

            isat++;
            fgets(ligne, MAXLU, ul_bull);
            sscanf(ligne, "%s%ld%f%f%f%f%f%f",
                   &artic_settings.contents.bulletin_data[isat].sat,       &artic_settings.contents.bulletin_data[isat].time_bulletin,
                   &artic_settings.contents.bulletin_data[isat].params[0], &artic_settings.contents.bulletin_data[isat].params[1], 
                   &artic_settings.contents.bulletin_data[isat].params[2], &artic_settings.contents.bulletin_data[isat].params[3], 
                   &artic_settings.contents.bulletin_data[isat].params[4], &artic_settings.contents.bulletin_data[isat].params[5]);
        }

        fclose (ul_bull);
    }

    void print_internal_prepas_struct(void)
    {
        printf("id: %d\n", artic_settings.contents.device_identifier);
        printf("num_sat: %d\n", SYSHAL_SAT_MAX_NUM_SATS);
        for (int i = 0; i < SYSHAL_SAT_MAX_NUM_SATS; ++i)
        {
            printf("Sat %d, name %c%c, epoch %ld\t", i, artic_settings.contents.bulletin_data[i].sat[0], artic_settings.contents.bulletin_data[i].sat[1],  artic_settings.contents.bulletin_data[i].time_bulletin);
            printf("semi-major axis (km): %f\t orbit inclination (deg) %f\t longitude of ascending node (deg) %f\t", artic_settings.contents.bulletin_data[i].params[0], artic_settings.contents.bulletin_data[i].params[1], artic_settings.contents.bulletin_data[i].params[2]);
            printf("asc. node drift during one revolution (deg): %f\t orbit period (min) %f\t  drift of semi-major axis (m/jour) %f\n", artic_settings.contents.bulletin_data[i].params[3], artic_settings.contents.bulletin_data[i].params[4], artic_settings.contents.bulletin_data[i].params[5]);
        }
    }

    void Inject_mock_syshal_sat_init(int fail_step, int inject_error_code)
    {
    }

    void Inject_mock_syshal_sat_power_on(int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        if (error_code) return;
        syshal_gpio_set_output_high_ExpectAndReturn(SYSHAL_SAT_GPIO_POWER_ON, error_code);
        syshal_gpio_set_output_low_ExpectAndReturn(SYSHAL_SAT_GPIO_RESET, error_code);
        syshal_gpio_set_output_high_ExpectAndReturn(SYSHAL_SAT_GPIO_RESET, error_code);
    }

    void Inject_mock_syshal_sat_power_off(int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        if (error_code) return;
    }

    void Inject_mock_syshal_sat_send_message(uint8_t *tx_msg, uint32_t size, int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;

        uint8_t tx_message[ARTIC_MSG_MAX_SIZE];

        for (size_t i = 0; i < ARTIC_MSG_MAX_SIZE; i++)
            tx_message[i] = 0;

        Mock_send_command_check_clean(ARTIC_CMD_SET_PTT_A3_TX_MODE, 1,  MCU_COMMAND_ACCEPTED, SYSHAL_SAT_ARTIC_DELAY_INTERRUPT,  &step_counter, fail_step, inject_error_code);
        if (fail_step != 0 && fail_step <= step_counter) return;

        Mock_burst_access(XMEM, false, TX_PAYLOAD_ADDRESS, tx_message, ARTIC_MSG_MAX_SIZE , NULL, &step_counter, fail_step, inject_error_code);
        if (fail_step != 0 && fail_step <= step_counter) return;

        Mock_send_command_check_clean(ARTIC_CMD_START_TX_1M_SLEEP, 1,  TX_FINISHED, SYSHAL_SAT_ARTIC_TIMEOUT_SEND_TX,  &step_counter, fail_step, inject_error_code);
        if (fail_step != 0 && fail_step <= step_counter)
        {
            Mock_wait_interrupt(2, false, SYSHAL_SAT_ARTIC_DELAY_INTERRUPT);
            return;
        }

    }

    uint8_t buffer_rx[4] = {0};
    void Mock_send_artic_command(artic_cmd_t cmd, uint32_t result_desired, int inject_error_code)
    {
        uint8_t buffer_tx[4] = {0};
        reverse_memcpy(&buffer_rx[1], (uint8_t *)&result_desired, 3);
        switch (cmd)
        {
            case DSP_STATUS:
                buffer_tx[0] = ARTIC_READ_ADDRESS(ADDRESS_DSP);
                break;
            case DSP_CONFIG:
                buffer_tx[0] = ARTIC_WRITE_ADDRESS(ADDRESS_DSP);
            default:
                break;
        }

        syshal_spi_transfer_ExpectAndReturn(SPI_SATALLITE, buffer_tx, buffer_rx, 4, inject_error_code);
        syshal_spi_transfer_ReturnMemThruPtr_rx_data(buffer_rx, 4);
        syshal_spi_transfer_IgnoreArg_rx_data();
        syshal_spi_transfer_IgnoreArg_tx_data();
    }

    void Mock_send_fw_files(fs_handle_t handle, firmware_header_t *firmware_header, int *step_counter, int fail_step, int inject_error_code)
    {
        fs_handle_t file_system_handle;

        char firmware_file[] = "../prepas/artic.bin";
        char path_file_XMEM[] = "../prepas/ARTIC.XMEM";
        char path_file_YMEM[] =  "../prepas/ARTIC.YMEM";
        char path_file_PMEM[] = "../prepas/ARTIC.PMEM";


        uint32_t bytes_written_fs = 0;
        uint32_t bytes_read = 0 ;
        uint32_t bytes_total_read = 0;


        uint8_t data[MAXIMUM_READ_FIRMWARE_OPERATION];
        streampos size = 0;

        uint8_t buffer[MAX_BURST];


        write_firmware_file(firmware_file);

        ifstream file(firmware_file, ios::in | ios::binary | ios::ate);

        if (!file.is_open())
        {
            printf("%s - path: %s\n", "ERROR FILE NOT OPENNED", firmware_file);
            return;
        }
        size = file.tellg();
        file.seekg (0, ios::beg);
        file.read  ((char * ) &firmware_header, sizeof(firmware_header ));
        file.close( );

        uint32_t last_address = 0;
        uint32_t current_address = 0;
        string line  = "";
        uint32_t address = 0 ;
        uint32_t start_address = 0;

        ifstream file_XMEM(path_file_XMEM);
        if (file_XMEM.is_open())
        {
            int bytes_data = 0;
            last_address = 0;
            while ( getline (file_XMEM, line) )
            {
                bytes_data += SIZE_SPI_REG_XMEM_YMEM_IOMEM;
                address = 0;
                sscanf( line.c_str(), "@%x%x", &address, &data);
                if (bytes_data + SIZE_SPI_REG_XMEM_YMEM_IOMEM > MAX_BURST || (last_address + 1) < address)
                {
                    Mock_burst_access(XMEM, false, start_address, buffer, bytes_data, NULL, step_counter, fail_step, inject_error_code);
                    start_address = address + 1;
                    bytes_data = 0;
                }
                last_address = address;
                memcpy(&buffer[bytes_data], data, SIZE_SPI_REG_XMEM_YMEM_IOMEM);
            }
            if (bytes_data > 0)
            {
                Mock_burst_access( XMEM, false, start_address, buffer, bytes_data, NULL, step_counter, fail_step, inject_error_code);
            }
            file_XMEM.close();
        }
        else
        {
            printf("%s - path: %s\n", "ERROR FILE NOT OPENNED", path_file_PMEM);
        }

        ifstream file_YMEM(path_file_YMEM);
        if (file_YMEM.is_open())
        {
            int bytes_data = 0;
            last_address = 0;
            start_address = 0;
            while ( getline (file_YMEM, line) )
            {
                bytes_data += SIZE_SPI_REG_XMEM_YMEM_IOMEM;
                address = 0;
                sscanf( line.c_str(), "@%x%x", &address, &data);
                if (bytes_data + SIZE_SPI_REG_XMEM_YMEM_IOMEM > MAX_BURST || (last_address + 1) < address)
                {
                    Mock_burst_access(XMEM, false, start_address, buffer, bytes_data, NULL, step_counter, fail_step, inject_error_code);
                    start_address = address + 1;
                    bytes_data = 0;
                }
                last_address = address;
                memcpy(&buffer[bytes_data], data, SIZE_SPI_REG_XMEM_YMEM_IOMEM);
            }
            if (bytes_data > 0)
            {
                Mock_burst_access( YMEM, false, start_address, buffer, bytes_data, NULL, step_counter, fail_step, inject_error_code);
            }
            file_YMEM.close();
        }
        else
        {
            printf("%s - path: %s\n", "ERROR FILE NOT OPENNED", path_file_PMEM);
        }


        ifstream file_PMEM(path_file_PMEM);
        if (file_PMEM.is_open())
        {
            int bytes_data = 0;
            last_address = 0;
            start_address = 0;
            while ( getline (file_PMEM, line) )
            {
                bytes_data += SIZE_SPI_REG_PMEM;
                address = 0;
                sscanf( line.c_str(), "@%x%x", &address, &data);
                if (bytes_data + SIZE_SPI_REG_PMEM >= MAX_BURST || (last_address + 1) < address)
                {
                    Mock_burst_access(PMEM, false, start_address, buffer, bytes_data, NULL, step_counter, fail_step, inject_error_code);
                    start_address = address + 1;
                    bytes_data = 0;
                }
                last_address = address;
                memcpy(&buffer[bytes_data], data, SIZE_SPI_REG_PMEM);
            }
            if (bytes_data > 0)
            {
                Mock_burst_access( PMEM, false, start_address, buffer, bytes_data, NULL, step_counter, fail_step, inject_error_code);
            }
            file_PMEM.close();
        }
        else
        {
            printf("%s - path: %s\n", "ERROR FILE NOT OPENNED", path_file_PMEM);
        }
    }
    void Inject_mock_syshal_sat_program_firmware(fs_t fs, uint32_t local_file_id, firmware_header_t *firmware_header, int fail_step, int inject_error_code)
    {

        fs_handle_t handle;

        uint32_t result_desired = 85;

        int step_counter = 0;
        int error_code = 0;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        Mock_send_artic_command(DSP_STATUS, result_desired, error_code);
        if (error_code) return;

        int fake_step_counter = 0;
        Mock_send_fw_files(handle, firmware_header, &fake_step_counter,  0, SYSHAL_SAT_NO_ERROR);

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        Mock_send_artic_command(DSP_CONFIG, result_desired, error_code);
        if (error_code) return;

        // Interrupt 1 will be high when start-up is completed.
        Mock_wait_interrupt(INTERRUPT_1, true, SYSHAL_SAT_ARTIC_DELAY_INTERRUPT);
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        Mock_clean_interrupt(INTERRUPT_1, error_code);

        if (error_code) return;
        Mock_check_crc(step_counter, fail_step, inject_error_code);
    }

    void Mock_check_crc(int step_counter, int fail_step, int inject_error_code)
    {
        uint8_t *expected_CRC = new uint8_t[NUM_FIRMWARE_FILES_ARTIC * SIZE_SPI_REG_XMEM_YMEM_IOMEM];

        for (int i = 0; i < NUM_FIRMWARE_FILES_ARTIC; ++i)
        {
            reverse_memcpy(&expected_CRC[i * SIZE_SPI_REG_XMEM_YMEM_IOMEM], (uint8_t* ) &crc_values[i], SIZE_SPI_REG_XMEM_YMEM_IOMEM);
        }
        Mock_read_spi(XMEM, CRC_ADDRESS, expected_CRC, NUM_FIRMWARE_FILES_ARTIC * SIZE_SPI_REG_XMEM_YMEM_IOMEM, &step_counter, fail_step, inject_error_code);
    }
    void Mock_read_spi(mem_id_t mode, uint32_t start_address, uint8_t *read_expected, uint32_t size, int *step_counter, int fail_step, int inject_error_code)
    {
        Mock_burst_access(mode, true, start_address, NULL,  size, read_expected, step_counter, fail_step, inject_error_code);
    }

    void write_firmware_file(char *path)
    {
        fs_handle_t file_system_handle;
        uint32_t bytes_written_fs = 0 ;
        uint32_t bytes_read = 0;
        uint32_t bytes_total_read = 0;
        // reading an entire binary file

        using namespace std;

        streampos size;

        char buffer[MAX_READ];
        ifstream file(path, ios::in | ios::binary | ios::ate);
        if (file.is_open())
        {
            EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_FIRWMWARE, FS_MODE_CREATE, NULL));

            size = file.tellg();
            file.seekg (0, ios::beg);
            while (bytes_total_read < size)
            {
                file.read (buffer, MAX_READ);
                bytes_read = file.gcount();
                bytes_total_read += bytes_read;
                EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, buffer, bytes_read, &bytes_written_fs));
                EXPECT_EQ(bytes_written_fs, bytes_read);
            }
            file.close();
            EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));
        }
        else
        {
            printf("File could not be oppened\n");
        }
    }

    void Mock_init(void)
    {
        fill_internal_prepas();

        syshal_sat_init(config);
    }

    void Mock_power_on(void)
    {
        syshal_gpio_set_output_high_ExpectAndReturn(SYSHAL_SAT_GPIO_POWER_ON, 0);
        syshal_gpio_set_output_low_ExpectAndReturn(SYSHAL_SAT_GPIO_RESET, 0);
        syshal_gpio_set_output_high_ExpectAndReturn(SYSHAL_SAT_GPIO_RESET, 0);
        syshal_sat_power_on();
    }

    void Mock_burst_access(mem_id_t mode, bool read, uint32_t start_address, uint8_t *tx_data, uint32_t size, uint8_t *rx_data, int *step_counter, int fail_step, int inject_error_code)
    {
        int error_code = 0;

        if (++(*step_counter) == fail_step)
            error_code = inject_error_code;
        Mock_configure_burst(error_code);
        if (error_code) return;

        int lenght_transfer = 3;
        if (mode == PMEM) lenght_transfer = 4;
        Mock_send_burst(read, rx_data, size, lenght_transfer, fail_step, step_counter, inject_error_code);
        if (fail_step != 0 && fail_step <= *step_counter) return;

        syshal_spi_finish_transfer_ExpectAndReturn(SPI_SATALLITE, SYSHAL_SAT_NO_ERROR);
    }

    void Mock_configure_burst(int error_code)
    {
        syshal_spi_transfer_ExpectAndReturn(SPI_SATALLITE, NULL, NULL, 4, error_code);
        syshal_spi_transfer_IgnoreArg_tx_data();
        syshal_spi_transfer_IgnoreArg_rx_data();
    }

    void Mock_send_burst(bool read, uint8_t *rx_data, uint32_t size, uint32_t lenght_transfer, int fail_step, int *step_counter, int inject_error_code)
    {
        uint16_t num_transfer = size / lenght_transfer;
        int error_code = 0;
        uint8_t *var_tx = new uint8_t[2048];
        uint8_t *var_rx = new uint8_t[2048];
        if (read)
        {
            memcpy(var_rx, rx_data, size);
            for (uint32_t i = 0; i < num_transfer; ++i)
            {
                if (++(*step_counter) == fail_step)
                    error_code = inject_error_code;
                total_number++;
                syshal_spi_transfer_continous_ExpectAndReturn(SPI_SATALLITE, var_tx, var_rx, lenght_transfer, error_code);
                syshal_spi_transfer_continous_ReturnMemThruPtr_rx_data(&var_rx[i * lenght_transfer], lenght_transfer);
                syshal_spi_transfer_continous_IgnoreArg_tx_data();
                syshal_spi_transfer_continous_IgnoreArg_rx_data();
                if (error_code) return;
            }
        }
        else
        {
            for (uint32_t i = 0; i < num_transfer; ++i)
            {
                if (++(*step_counter) == fail_step)
                    error_code = inject_error_code;
                total_number++;
                //printf("total number continuous %d\n", total_number);
                syshal_spi_transfer_continous_ExpectAndReturn(SPI_SATALLITE, var_tx, NULL, lenght_transfer, error_code);
                syshal_spi_transfer_continous_IgnoreArg_tx_data();
                syshal_spi_transfer_continous_IgnoreArg_rx_data();
                if (error_code) return;
            }
        }

    }

    void Mock_send_command(uint8_t command, int inject_error_code)
    {
        uint8_t * var = new uint8_t;
        memcpy(var, &command, 1);
        syshal_spi_transfer_ExpectAndReturn(SPI_SATALLITE, var, NULL, 1, inject_error_code);
        syshal_spi_transfer_IgnoreArg_tx_data();
        syshal_spi_transfer_IgnoreArg_rx_data();
    }

    void Mock_wait_interrupt(uint8_t interrupt_number, bool value, int timeout)
    {
        uint8_t sat_interrupt_value;
        uint8_t gpio_port;
        uint32_t time_running = 0;

        if (interrupt_number == 1)
            gpio_port = SYSHAL_SAT_GPIO_INT_1;
        else
            gpio_port = SYSHAL_SAT_GPIO_INT_2;

        syshal_time_get_ticks_ms_ExpectAndReturn(time_running);
        do
        {
            time_running += SYSHAL_SAT_ARTIC_DELAY_TICK_INTERRUPT_MS;
            syshal_time_get_ticks_ms_ExpectAndReturn(time_running);
            sat_interrupt_value = value;
            syshal_gpio_get_input_ExpectAndReturn(gpio_port, sat_interrupt_value);
        }
        while (time_running < timeout && sat_interrupt_value == false);
    }

    void Mock_clean_interrupt(uint8_t interrupt_number, int  inject_error_code)
    {
        if (interrupt_number == 1)
            Mock_send_command(ARTIC_CMD_CLEAR_INT1, inject_error_code);
        else
            Mock_send_command(ARTIC_CMD_CLEAR_INT2, inject_error_code);
    }

    void Mock_read_status(uint32_t status_expect, int *step_counter,  int fail_step, int inject_error_code)
    {
        uint8_t *var = new uint8_t[3];
        reverse_memcpy(var, (uint8_t * ) &status_expect, 3);
        Mock_burst_access(IOMEM, true, INTERRUPT_ADDRESS, NULL, 3, var, step_counter,  fail_step, inject_error_code);
    }
    void  Mock_send_command_check_clean(uint8_t command, uint8_t interrupt_number,  uint8_t status_flag_number, int timeout,  int *step_counter,  int fail_step, int inject_error_code)
    {
        int ret;
        int error_code = 0;
        uint32_t status = 0;
        bool gpio_result = true;


        if (++(*step_counter) == fail_step)
            error_code = inject_error_code;
        Mock_clean_interrupt(interrupt_number, error_code);
        if (error_code) return;


        if (++(*step_counter) == fail_step)
            error_code = inject_error_code;
        Mock_send_command(command, error_code);
        if (error_code) return;

        if (++(*step_counter) == fail_step)
        {
            error_code = inject_error_code;
            gpio_result = false;
        }
        Mock_wait_interrupt (interrupt_number, gpio_result, timeout);
        if (fail_step != 0 && fail_step <= *step_counter) return;


        uint32_t status_expect = (1 << status_flag_number);
        Mock_read_status(status_expect, step_counter,  fail_step, inject_error_code);
        if (fail_step != 0 && fail_step <= *step_counter) return;


        if (++(*step_counter) == fail_step)
            error_code = inject_error_code;
        Mock_clean_interrupt(interrupt_number, error_code);
        if (error_code) return;


    }
    /*! \brief Internal Function to copy in the reverse order
    *
    *  Function to send a basic artic command
    *
    * \param dst[out] ptr to the destination
    * \param src[in] ptr to the source
    * \param size[in] number of bytes to copy
    *
    */
    void reverse_memcpy(uint8_t *dst, uint8_t *src, size_t size)
    {
        for (uint32_t i = 0; i < size; ++i)
        {
            dst[i] = src[size - 1 - i];
        }
    }



};
TEST_F(SatTest, syshal_sat_init_complete)
{
    /* Arrange */
    fill_internal_prepas();
    Inject_mock_syshal_sat_init(0, SYSHAL_SAT_NO_ERROR);


    /* Act */
    int ret = syshal_sat_init(config);


    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret);
}



TEST_F(SatTest, syshal_sat_init_power_on)
{
    /* Arrange */
    Mock_init();
    Inject_mock_syshal_sat_power_on(0, SYSHAL_SAT_NO_ERROR);

    /* Act */
    int ret =  syshal_sat_power_on();

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret);
}

TEST_F(SatTest, syshal_sat_init_power_on_fail_not_init)
{
    /* Arrange */
    fill_internal_prepas();
    Inject_mock_syshal_sat_power_on(1, SYSHAL_SAT_ERROR_NOT_INIT);

    /* Act */

    int ret =  syshal_sat_power_on();

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_ERROR_NOT_INIT, ret);
}

TEST_F(SatTest, syshal_sat_power_off_success)
{
    /* Arrange */
    Mock_init();
    Mock_power_on();
    Inject_mock_syshal_sat_power_off(0, 0 );
    /* Act */
    int ret = syshal_sat_power_off();

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret);
}

TEST_F(SatTest, syshal_sat_power_off_not_init)
{
    /* Arrange */
    Inject_mock_syshal_sat_power_off(1, SYSHAL_SAT_ERROR_NOT_POWERED_ON);
    /* Act */
    int ret = syshal_sat_power_off();

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_ERROR_NOT_INIT, ret);
}

TEST_F(SatTest, syshal_sat_calc_prepass_fail_not_init)
{
    /* Arrange */

    iot_last_gps_location_t gps;
    iot_prepass_result_t result;

    uint32_t current_timestamp = 1552586400;
    gps.longitude = 1;
    gps.latitude  = 52;
    gps.timestamp = 0;


    /* Act */
    int ret = syshal_sat_calc_prepass(&gps, &result);

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_ERROR_NOT_INIT, ret);


}
TEST_F(SatTest, syshal_sat_calc_prepass_success_)
{
    /* Arrange */
    fill_internal_prepas();
    Inject_mock_syshal_sat_init(0, SYSHAL_SAT_NO_ERROR);
    iot_last_gps_location_t gps;
    iot_prepass_result_t result;
    uint32_t current_timestamp = 1552586400;
    gps.longitude = 1;
    gps.latitude  = 52;
    gps.timestamp = current_timestamp;

    /* Act */
    int ret1 = syshal_sat_init(config);

    int ret2 = syshal_sat_calc_prepass(&gps, &result);

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret1);
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret2);
    EXPECT_EQ(result, 1552590795);
}

TEST_F(SatTest, syshal_sat_calc_prepass_success_SimplePrediction_3_month)
{
    /* Arrange */
    fill_internal_prepas();
    Inject_mock_syshal_sat_init(0, SYSHAL_SAT_NO_ERROR);
    iot_last_gps_location_t gps;
    iot_prepass_result_t result;
    uint32_t current_timestamp = 1560531600; // 14/06/2019 @ 6:00pm (UTC)
    gps.longitude = 1;
    gps.latitude  = 52;
    gps.timestamp = current_timestamp;

    /* Act */
    int ret1 = syshal_sat_init(config);

    int ret2 = syshal_sat_calc_prepass(&gps, &result);

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret1);
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret2);
    EXPECT_EQ(result, 1560536875);
}

TEST_F(SatTest, syshal_sat_calc_prepass_success_SimplePrediction_12_month)
{
    /* Arrange */
    fill_internal_prepas();
    Inject_mock_syshal_sat_init(0, SYSHAL_SAT_NO_ERROR);
    iot_last_gps_location_t gps;
    iot_prepass_result_t result;
    uint32_t current_timestamp = 1584208800; // 14/03/2020 @ 6:00pm (UTC)
    gps.longitude = 1;
    gps.latitude  = 52;
    gps.timestamp = current_timestamp;

    /* Act */
    int ret1 = syshal_sat_init(config);
    int ret2 = syshal_sat_calc_prepass(&gps, &result);

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret1);
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret2);
    EXPECT_EQ(result,  1584210543);
}

TEST_F(SatTest, syshal_sat_download_firmware)
{
    /* Arrange */
    firmware_header_t firmware_header;

    Inject_mock_syshal_sat_program_firmware(file_system, FILE_ID_FIRWMWARE, &firmware_header,  0, 0);
    Mock_init();
    Mock_power_on();

    /* Act */
    int ret1 = syshal_sat_program_firmware(file_system, FILE_ID_FIRWMWARE);

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_download_firmware_fail1)
{
    /* Arrange */
    firmware_header_t firmware_header;

    Inject_mock_syshal_sat_program_firmware(file_system, FILE_ID_FIRWMWARE, &firmware_header,  1, -1);
    Mock_init();
    Mock_power_on();
    /* Act */
    int ret1 = syshal_sat_program_firmware(file_system, FILE_ID_FIRWMWARE);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_download_firmware_fail2)
{
    /* Arrange */
    firmware_header_t firmware_header;

    Inject_mock_syshal_sat_program_firmware(file_system, FILE_ID_FIRWMWARE, &firmware_header,  2, -1);
    Mock_init();
    Mock_power_on();

    /* Act */
    int ret1 = syshal_sat_program_firmware(file_system, FILE_ID_FIRWMWARE);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_download_firmware_fail3)
{
    /* Arrange */
    firmware_header_t firmware_header;

    Inject_mock_syshal_sat_program_firmware(file_system, FILE_ID_FIRWMWARE, &firmware_header,  3, -1);
    Mock_init();
    Mock_power_on();

    /* Act */
    int ret1 = syshal_sat_program_firmware(file_system, FILE_ID_FIRWMWARE);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message)
{
    /* Arrange */

    uint8_t msg[ARTIC_MSG_MAX_SIZE];
    for (int i = 0; i < ARTIC_MSG_MAX_SIZE; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, ARTIC_MSG_MAX_SIZE, 0, SYSHAL_SAT_NO_ERROR);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_EQ(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail1)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 1, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail2)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 2, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail3)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 3, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail4)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 4, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail5)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 5, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail6)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 6, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail7)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 7, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail8)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 8, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail9)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 9, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail10)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 10, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail11)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 11, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail12)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 12, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail13)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 13, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail14)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 14, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_15)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 15, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_16)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 16, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_17)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 17, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_18)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 18, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_19)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 19, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_20)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 20, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_21)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 21, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_22)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 22, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_23)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 23, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_24)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 24, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}

TEST_F(SatTest, syshal_sat_send_message_fail_25)
{
    /* Arrange */

    uint8_t msg[MAX_TX_SIZE_BYTES];
    for (int i = 0; i < MAX_TX_SIZE_BYTES; ++i)
    {
        msg[i] = i;
    }
    Mock_init();
    Mock_power_on();
    force_programmed();
    Inject_mock_syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES, 25, SYSHAL_SAT_ERROR_SPI);

    /* Act */
    int ret1 = syshal_sat_send_message(msg, MAX_TX_SIZE_BYTES);

    /* Assert */
    EXPECT_NE(SYSHAL_SAT_NO_ERROR, ret1);
}