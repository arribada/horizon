// test_syshal_cellular.cpp - syshal_cellular unit tests
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
#include "Mockfs.h"
#include "at.h"
#include "syshal_cellular.h"
#include "Mocksyshal_gpio.h"
#include "Mocksyshal_time.h"
#include "Mocksyshal_rtc.h"
#include "bsp.h"
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cmath>

#define DEBUG
#define SET_TIMEOUT 1000
#define MICROSECOND 1000
#define BITS_PER_CHARACTER (10)
#define EXPECT_ERROR_CODE 111

#define SYSHAL_CELLULAR_GPIO_POWER_ON  (GPIO_EXT1_GPIO1)

static const uint8_t OK[] = "\rOK";
static const uint8_t OK_HTTP[] = {'O', 'K', '\r', '\n', '\0'};
static const uint8_t AT[] = "AT";
static const uint8_t CPWROFF[] = "CPWROFF";
static const uint8_t CIMI[] = "CIMI";
static const uint8_t CIMI_RESPONSE_EXPECTED[] = "\r\n222107701772423";
static const uint8_t clean_CPRF[] = "USECPRF=0";
static const uint8_t sec_level_CPRF[] = "USECPRF=0.0.0";
static const uint8_t mim_tls_CPRF[] = "USECPRF=0.1.3";
static const uint8_t root_CA_CPRF[] = "USECPRF=0.3.\"%s\"";
static const uint8_t device_cert_CPRF[] = "USECPRF=0.5.\"%s\"";
static const uint8_t device_key_CPRF[] = "USECPRF=0.6.\"%s\"";
static const uint8_t send_root_CA_CPRF[] = "USECPRF=0.3.\"root-CA.pem\"";
static const uint8_t send_device_cert_CPRF[] = "USECPRF=0.5.\"deviceCert.pem\"";
static const uint8_t send_device_key_CPRF[] = "USECPRF=0.6.\"deviceCert.key\"";
static const uint8_t set_URAT[] = "URAT=%u";
static const uint8_t send_set_URAT2G[] = "URAT=0";
static const uint8_t send_set_URAT3G[] = "URAT=2";
static const uint8_t send_set_URATAUTO[] = "URAT=1";
static const uint8_t set_COPS5[] = "COPS=5";
static const uint8_t set_COPS0[] = "COPS=0";
static const uint8_t set_COPS2[] = "COPS=2";
static const uint8_t expected_resp_COPS5[] = "MCC:255";
static const uint8_t abort__cpp[] = "abort";
static const uint8_t aborted__cpp[] = "ABORTED";
static const uint8_t set_UPSD_APN[] = "UPSD=0,1,\"%s\"";
static const uint8_t set_UPSD_APN_expected[] = "UPSD=0,1,\"everywhere\"";
static const uint8_t set_UPSD_username[] = "UPSD=0,2,\"%s\"";
static const uint8_t set_UPSD_username_expected[] = "UPSD=0,2,\"wap\"";
static const uint8_t set_UPSD_password[] = "UPSD=0,3,\"%s\"";
static const uint8_t set_UPSD_password_expected[] = "UPSD=0,3,\"wap\"";
static const uint8_t set_auto_security[] = "UPSD=0,6,3";
static const uint8_t activate_UPSD[] = "UPSDA=0,3";

static const uint8_t clean_HTTP0[] = "UHTTP=0";
static const uint8_t send_set_domain_HTTP0[] = "UHTTP=0,1,\"icoteq-test.s3.amazonaws.com\"";

static const uint8_t set_sec_HTTP0[] = "UHTTP=0,6,1,0";
static const uint8_t send_set_port_HTTP0[] = "UHTTP=0,5,443";
static const uint8_t send_get_HTTP0[] = "UHTTPC=0,1,\"/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D\",\"TEMP.DAT\"";
static const uint8_t send_post_HTTP0[] = "UHTTPC=0,4,\"/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D\",\"RESULT.DAT\",\"TEMP.DAT\",0";
static const uint8_t send_read_file[] = "URDFILE=\"TEMP.DAT\"";
//static const uint8_t expected_respond_read_file[] = "TEEEE+URDFILE: \"TEMP.DAT\",2093,\"";
static const uint8_t expected_respond_read_file[] = "+URDFILE: \"TEMP.DAT\",90,\"";
static const uint8_t read_file_mock_success[] = "HTTP/1.1 200";
static const uint8_t read_file_mock_fail[] = "HTTP/1.1 400\0";
static const uint8_t read_header_mock[] = "OK\r\nContent-Type: application/json\r\nContent-Length: 1574\r\n\r\n";
static const uint8_t read_rest_file_mock[] = "{\"state\":{\"desired\":{\"device_status\":{\"last_log_file_read_pos\":0},\"device_update\":{\"firmware_update\":{\"url\":{\"path\":\"/f6fb3202-29c2-4717-baef-2ab1c3f4153f?AWSAccessKeyId=AKIATBYHTG6APQ3LW75N&Expires=1585816420&Signature=g9BqKgyQyIWS0QzExRglyDi9tbY%3D\",\"domain\":\"arribada.s3.amazonaws.com\",\"port\":443},\"version\":1},\"configuration_update\":{\"url\":{\"path\":\"/3f541579-524e-46a3-be05-6eb26b5a87f6?AWSAccessKeyId=AKIATBYHTG6APQ3LW75N&Expires=1585825028&Signature=0JE57CG%2FQHu0G%2FzmWJQmSWtPbRE%3D\",\"domain\":\"arribada.s3.amazonaws.com\",\"port\":443},\"version\":1}}},\"delta\":{\"device_status\":{\"last_log_file_read_pos\":0},\"device_update\":{\"firmware_update\":{\"url\":{\"path\":\"/f6fb3202-29c2-4717-baef-2ab1c3f4153f?AWSAccessKeyId=AKIATBYHTG6APQ3LW75N&Expires=1585816420&Signature=g9BqKgyQyIWS0QzExRglyDi9tbY%3D\",\"domain\":\"arribada.s3.amazonaws.com\",\"port\":443},\"version\":1},\"configuration_update\":{\"url\":{\"path\":\"/3f541579-524e-46a3-be05-6eb26b5a87f6?AWSAccessKeyId=AKIATBYHTG6APQ3LW75N&Expires=1585825028&Signature=0JE57CG%2FQHu0G%2FzmWJQmSWtPbRE%3D\",\"domain\":\"arribada.s3.amazonaws.com\",\"port\":443},\"version\":1}}}},\"metadata\":{\"desired\":{\"device_status\":{\"last_log_file_read_pos\":{\"timestamp\":1554199123}},\"device_update\":{\"firmware_update\":{\"url\":{\"path\":{\"timestamp\":1554280421},\"domain\":{\"timestamp\":1554280421},\"port\":{\"timestamp\":1554280421}},\"version\":{\"timestamp\":1554280421}},\"configuration_update\":{\"url\":{\"path\":{\"timestamp\":1554289029},\"domain\":{\"timestamp\":1554289029},\"port\":{\"timestamp\":1554289029}},\"version\":{\"timestamp\":1554289029}}}}},\"version\":5,\"timestamp\":1554297722}";
static const uint8_t delete_temp[] = "UDELFILE=\"TEMP.DAT\"";
static const uint8_t send_download_temp[] = "UDWNFILE=\"TEMP.DAT\",140";
static const uint8_t start_data[] = ">";



static const uint8_t ask_COPS[] = "COPS?";
static const uint8_t read_COPS_expected[] = "+COPS: 0,0,\"vodafone UK\",2";
static const uint8_t ask_CSQ[]  = "CSQ";
static const uint8_t read_CSQ_expected[]  = "+CSQ: 29,3";
static const uint8_t ask_CREG[]  = "CREG?";
static const uint8_t read_CREG_expected[] = "+CREG: 2,5,\"011E\",\"2F0FA78\",6";
static const uint8_t activate_CREG[]  = "CREG=2";
static const uint8_t deactivate_CREG[]  = "CREG=0";



static const uint8_t get_sucess[] = "+UUHTTPCR: 0,1,1";
static const uint8_t post_sucess[] = "+UUHTTPCR: 0,4,1";

static const uint8_t DIS_ERR_VERB[] = "ATE0";
static const uint8_t DISABLE_UMWI[] = "UMWI=0";
static const uint8_t ENABLE_VERB_ERR[] = "CMEE=1";

static const uint8_t ref_apn[] = "everywhere";
static const uint8_t ref_username[] = "wap";
static const uint8_t ref_password[] = "wap";


syshal_cellular_apn_t apn;
syshal_cellular_apn_t apn_user;
uint32_t length_skip;


uint16_t error_code_skip;

class CellularTest : public ::testing::Test
{
public:
    uint32_t uart_device_priv = UART_CELLULAR;
    uint32_t char_timeout_microsecond;
    uint32_t char_timeout_millisecond;

    virtual void SetUp()
    {
        Mocksyshal_uart_Init();
        Mockfs_Init();
        Mocksyshal_gpio_Init();
        Mocksyshal_time_Init();
        Mocksyshal_rtc_Init();

        syshal_rtc_soft_watchdog_refresh_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);
        syshal_rtc_date_time_to_timestamp_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);

        uint32_t baudrate = UART_CELLULAR_BAUDRATE;
        memcpy(apn.name, ref_apn, sizeof(ref_apn));
        apn.username[0] = '\0';
        apn.password[0] = '\0';
        memcpy(apn_user.name, ref_apn, sizeof(ref_apn));
        memcpy(apn_user.username, ref_username, sizeof(ref_username));
        memcpy(apn_user.password, ref_password, sizeof(ref_password));

        char_timeout_microsecond = (BITS_PER_CHARACTER * TIMES_CHAR_TIMEOUT * 1000000) / baudrate;
        char_timeout_millisecond = (BITS_PER_CHARACTER * TIMES_CHAR_TIMEOUT * 1000) / baudrate;
        if (char_timeout_millisecond == 0)
            char_timeout_millisecond = 1;

        syshal_time_delay_ms_Ignore();

        syshal_uart_get_baud_ExpectAndReturn(uart_device_priv, &baudrate, SYSHAL_UART_NO_ERROR);
        syshal_uart_get_baud_ReturnThruPtr_baudrate(&baudrate);
        syshal_uart_get_baud_IgnoreArg_baudrate();
        at_init(UART_CELLULAR);
        //Inject_mock_syshal_cellular_setcom(0, SYSHAL_CELLULAR_NO_ERROR);

    }

    virtual void TearDown()
    {
        Mocksyshal_uart_Verify();
        Mocksyshal_uart_Destroy();
        Mockfs_Verify();
        Mockfs_Destroy();
        Mocksyshal_gpio_Verify();
        Mocksyshal_gpio_Destroy();
        Mocksyshal_time_Verify();
        Mocksyshal_time_Destroy();
        Mocksyshal_rtc_Verify();
        Mocksyshal_rtc_Destroy();
    }

public:

    int conver_status(int status)
    {
        switch (status)
        {
            case SYSHAL_UART_NO_ERROR:
                return SYSHAL_CELLULAR_NO_ERROR;
                break;
            case SYSHAL_UART_ERROR_INVALID_SIZE:
                return SYSHAL_CELLULAR_ERROR_DEVICE;
                break;
            case SYSHAL_UART_ERROR_INVALID_INSTANCE:
                return SYSHAL_CELLULAR_ERROR_DEVICE;
                break;
            case SYSHAL_UART_ERROR_BUSY:
                return SYSHAL_CELLULAR_ERROR_DEVICE;
                break;
            case SYSHAL_UART_ERROR_TIMEOUT:
                return SYSHAL_CELLULAR_ERROR_TIMEOUT;
                break;
            case SYSHAL_UART_ERROR_DEVICE:
                return SYSHAL_CELLULAR_ERROR_DEVICE;
                break;
            default:
                return SYSHAL_CELLULAR_ERROR_DEVICE;
                break;
        }
    }
    int command_lenght(const uint8_t*command)
    {
        int len = 0;
        while (command[len] != 0)len++;
        return len;
    }

    void send_simple_mock(const uint8_t *command, int status = SYSHAL_UART_NO_ERROR)
    {
        uint8_t *command_received = new uint8_t[MAX_AT_COMMAND_SIZE];
        uint32_t length = command_lenght(command);
        memcpy(command_received, "AT+", 3);
        memcpy(command_received + 3, command, length);
        length += 3 + 1   ;
        //strcat((char*) command_received, (char*)command);
        //uint32_t len = strlen((char*)command_received) + 1;
        //command_received[len] = '\r';
        syshal_uart_send_ExpectAndReturn(uart_device_priv, command_received, length, status);
    }

    void send_raw_simple_mock(const uint8_t *command, int status = SYSHAL_UART_NO_ERROR)
    {

        uint32_t length = command_lenght(command);
        uint8_t *command_received = new uint8_t[MAX_AT_COMMAND_SIZE];
        uint8_t *last_character = new uint8_t;
        *last_character = '\r';
        memcpy(command_received, command, length);
        syshal_uart_send_ExpectAndReturn(uart_device_priv, command_received, length, SYSHAL_UART_NO_ERROR);
        syshal_uart_send_ExpectAndReturn(uart_device_priv, last_character, 1, status);

    }

    int send_simple_string(uint8_t *command, char *string, uint8_t *command_received)
    {
        uint32_t len = strlen((char*)command_received) + 1;
        command_received[len] = '\r';
        syshal_uart_send_ExpectAndReturn(uart_device_priv, command_received, len, SYSHAL_UART_NO_ERROR);
        return at_send(command, string);
    }

    int send_simple_num(uint8_t *command, uint32_t value, uint8_t *command_received)
    {
        uint32_t len = strlen((char*)command_received) + 1;
        command_received[len] = '\r';
        syshal_uart_send_ExpectAndReturn(uart_device_priv, command_received, len, SYSHAL_UART_NO_ERROR);
        return at_send(command, value);
    }

    int expect_simple(uint8_t *command_expected, uint8_t *command_received_un, uint32_t timeout)
    {
        uint8_t *ptr_received = command_received_un;
        uint32_t temp_timeout = timeout * MICROSECOND;
        uint32_t bytes_received = 1;
        uint32_t length;
        int status;

        uint32_t len = strlen((char*)command_expected);
        command_received_un[strlen((char*)command_received_un)] = '\r';
        for (int i = 0; i < len; ++i)
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_received, 1, temp_timeout, char_timeout_microsecond, &bytes_received, SYSHAL_UART_NO_ERROR);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_received, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(&bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            temp_timeout = char_timeout_microsecond;
            ptr_received++;
        }
        status = at_expect(command_expected, timeout, &length);
        return status;
    }

    int expect_simple_mock(const uint8_t *command, uint32_t timeout, int status = SYSHAL_UART_NO_ERROR)
    {
        uint32_t temp_timeout = timeout * MICROSECOND;
        //uint32_t bytes_received = 1;
        uint32_t *bytes_received = new uint32_t;
        int len = strlen((char*)command);
        *bytes_received = 1;
        uint8_t *ptr_received = (uint8_t *)command;
        for (int i = 0; i < len; ++i)
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_received, 1, temp_timeout, char_timeout_microsecond, bytes_received, status);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_received, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            if (status != SYSHAL_UART_NO_ERROR)
            {
                break;
            }
            temp_timeout = char_timeout_microsecond;

            ptr_received++;
        }
    }
    int expect_simple_with_cmee(uint32_t timeout)
    {
        uint32_t temp_timeout = timeout * MICROSECOND;

        static uint8_t CMEE[] = "+CME ERROR: 111\r";
        //uint32_t bytes_received = 1;
        uint32_t *bytes_received = new uint32_t;
        int len = strlen((char*)CMEE);
        *bytes_received = 1;
        uint8_t *ptr_received = CMEE;
        //printf("PTR SIMPLE CMEE %s\n", ptr_received);
        for (int i = 0; i < len ; ++i)
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_received, 1, temp_timeout, char_timeout_microsecond, bytes_received, SYSHAL_UART_NO_ERROR);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_received, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            temp_timeout = char_timeout_microsecond;
            ptr_received++;
        }
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_received, 1, temp_timeout, char_timeout_microsecond, bytes_received, SYSHAL_UART_ERROR_TIMEOUT);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(--ptr_received, 1);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();

    }

    int expect_simple_mock_num(const uint8_t *command, uint32_t timeout, int status = SYSHAL_UART_NO_ERROR)
    {
        uint8_t *ptr_received = (uint8_t *)command;
        uint32_t temp_timeout = timeout * MICROSECOND;
        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;

        int len = strlen((char*)command);

        for (int i = 0; i < len + 1; ++i)
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_received, 1, temp_timeout, char_timeout_microsecond, bytes_received, status);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_received, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            if (status != SYSHAL_UART_NO_ERROR)
            {
                break;
            }
            temp_timeout = char_timeout_microsecond;
            ptr_received++;
        }
    }
    int expect_simple_mock_length(const uint8_t *command, uint32_t timeout, uint32_t len_extra, int status = SYSHAL_UART_NO_ERROR)
    {
        uint8_t *ptr_received = (uint8_t *)command;
        uint32_t temp_timeout = timeout * MICROSECOND;
        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;

        int len = strlen((char*)command) + len_extra;

        for (int i = 0; i < len ; ++i)
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_received, 1, temp_timeout, char_timeout_microsecond, bytes_received, status);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_received, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            if (status != SYSHAL_UART_NO_ERROR)
            {
                break;
            }
            temp_timeout = char_timeout_microsecond;
            ptr_received++;
        }
    }

    void expect_simple_num_mock(uint8_t *command_expected, uint32_t *value, uint8_t *command_received_un, uint32_t timeout, uint32_t complete_len)
    {
        uint8_t *ptr_received = command_received_un;
        uint32_t temp_timeout = timeout * MICROSECOND;

        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;
        int status;

        command_received_un[strlen((char*)command_received_un)] = '\r';

        for (int i = 0; i < complete_len; ++i)
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_received, 1, temp_timeout, char_timeout_microsecond, bytes_received, SYSHAL_UART_NO_ERROR);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_received, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            temp_timeout = char_timeout_microsecond;
            ptr_received++;
        }
    }

    void read_buffer_simple_mock(uint8_t *buffer, uint32_t len, int status = SYSHAL_CELLULAR_NO_ERROR)
    {

        static uint32_t bytes_received = len;

        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, NULL, len, SYSHAL_CELLULAR_FILE_TIMEOUT_MS * MICROSECOND, char_timeout_microsecond, &bytes_received, status);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(buffer, len);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(&bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
    }
    void fs_write_simple_mock(uint8_t *buffer, uint32_t len, fs_handle_t handle, int status = FS_NO_ERROR)
    {
        uint32_t *written = new uint32_t;
        *written = len;


        fs_write_ExpectAndReturn(handle, buffer, len, NULL, status);
        fs_write_ReturnThruPtr_written(written);
        fs_write_IgnoreArg_written();

    }

    void mock_flush_simple(int status = SYSHAL_UART_NO_ERROR )
    {
        syshal_uart_flush_ExpectAndReturn(uart_device_priv, status);
        return;
    }
    void discard_mock(uint32_t length, int status = SYSHAL_UART_NO_ERROR    )
    {
        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;
        uint8_t buffer = 'A';
        uint32_t internal_counter = 0;
        while (internal_counter++ < length)
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, NULL, 1, char_timeout_microsecond, char_timeout_microsecond, bytes_received, status);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(&buffer, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            if (status != SYSHAL_UART_NO_ERROR)
                return;
        }

    }


    void Inject_mock_syshal_cellular_setcom(int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;
        static uint32_t baudrate = UART_CELLULAR_BAUDRATE;

        if (++step_counter == fail_step)
            error_code = inject_error_code;

        if (++step_counter == fail_step)
            error_code = inject_error_code;

        if (++step_counter == fail_step)
            error_code = inject_error_code;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_raw_simple_mock(DIS_ERR_VERB, error_code);
        if (error_code) return;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(DISABLE_UMWI, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(ENABLE_VERB_ERR, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;
    }


    void Inject_mock_syshal_cellular_power_off()
    {
        syshal_gpio_set_output_low_ExpectAndReturn(SYSHAL_CELLULAR_GPIO_POWER_ON, SYSHAL_GPIO_NO_ERROR);
    }

    void Inject_mock_syshal_cellular_check_sim(int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(CIMI, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock_num(CIMI_RESPONSE_EXPECTED, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;
    }

    void Inject_mock_syshal_cellular_create_secure_profile(int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;


        /* Clear secure profile 0 */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(clean_CPRF, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;


        /* Set secure profile 0  security level 0 (No certificate validation)*/
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(sec_level_CPRF, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Set secure profile 0 minimum STL/TLS version 0 (TLS 1.2)*/
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(mim_tls_CPRF, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;


        /* Select "root-CA.crt as device root certificate in secure profile 0")*/
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_root_CA_CPRF, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Select "deviceCert.pem" as device device certificate in secure profile 0")*/
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_device_cert_CPRF, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Select "deviceCert.key" as device private key in secure profile 0")*/
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_device_key_CPRF, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

    }


    void Inject_mock_syshal_cellular_set_rat(int fail_step, int inject_error_code, uint32_t timeout)
    {
        int error_code = 0;
        int step_counter = 0;


        /* Set RAT technology */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_set_URAT2G, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
        {
            error_code = inject_error_code;
            expect_simple_with_cmee(timeout);
        }
        else
        {
            expect_simple_mock(OK, timeout, error_code);
        }
        if (error_code) return;
    }


    void Inject_mock_syshal_cellular_scan_succes(int fail_step, int inject_error_code, uint32_t timeout)
    {
        int error_code = 0;
        int step_counter = 0;


        /* Set  extended network search */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_COPS5, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock_num(expected_resp_COPS5, timeout, error_code);
        if (error_code != SYSHAL_UART_ERROR_TIMEOUT and error_code != SYSHAL_UART_NO_ERROR) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_raw_simple_mock(abort__cpp , error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(aborted__cpp, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code != SYSHAL_UART_NO_ERROR) return;
    }


    void Inject_mock_syshal_cellular_attach(int fail_step, int inject_error_code, uint32_t timeout)
    {
        int error_code = 0;
        int step_counter = 0;


        /* Set RAT technology */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_COPS0, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, timeout, error_code);
        if (error_code) return;

    }

    void Inject_mock_syshal_cellular_detach(int fail_step, int inject_error_code, uint32_t timeout)
    {
        int error_code = 0;
        int step_counter = 0;

        /* Set RAT technology */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_COPS2, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, timeout, error_code);
        if (error_code) return;

    }
    void Inject_mock_syshal_cellular_network_info(int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;


        /* Read operator name" */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(ask_COPS, error_code);

        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock_length(read_COPS_expected, SYSHAL_CELLULAR_FILE_TIMEOUT_MS, 1,  error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;


        /* Sread quality and signal power*/
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(ask_CSQ, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock_length(read_CSQ_expected, SYSHAL_CELLULAR_FILE_TIMEOUT_MS, 1,  error_code);
        if (error_code) return;


        /* Activate URC for receiving the LAC */

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(activate_CREG, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;


        /* Read CREG Register for LAC AND Cell ID*/
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(ask_CREG, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock_length(read_CREG_expected, SYSHAL_CELLULAR_FILE_TIMEOUT_MS, 1,  error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;



        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(deactivate_CREG, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;
    }


    void Inject_mock_syshal_cellular_activate_pdp_no_user(int fail_step, int inject_error_code, uint32_t timeout)
    {
        int error_code = 0;
        int step_counter = 0;


        /* Set Packet data profile 1 with APN "everywhere" */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_UPSD_APN_expected, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Set security Level Packet switch data Context */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_auto_security, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;


        /* Activate Packet switch data Context */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(activate_UPSD, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, timeout, error_code);
        if (error_code) return;


    }

    void Inject_mock_syshal_cellular_activate_pdp_user(int fail_step, int inject_error_code, uint32_t timeout)
    {
        int error_code = 0;
        int step_counter = 0;


        /* Set Packet data profile 1 with APN "everywhere" */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_UPSD_APN_expected, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Set Packet data profile 1 with username "wap" */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_UPSD_username_expected, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Set Packet data profile 1 with password "wap" */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_UPSD_password_expected, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;


        /* Set security Level Packet switch data Context */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_auto_security, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;

        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Activate Packet switch data Context */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(activate_UPSD, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, timeout, error_code);
        if (error_code) return;


    }

    void Inject_mock_syshal_cellular_https_config(int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;


        /* Clean HTTP profile 0 */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(clean_HTTP0, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Set Domain for HTTP profile 0 */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_set_domain_HTTP0, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Connect Secure profile 0 */

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(set_sec_HTTP0, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

        /* Set port for HTTP profile 0 */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_set_port_HTTP0, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;

    }

    void Inject_mock_syshal_cellular_https_get(int fail_step, int inject_error_code, uint32_t timeout)
    {
        int error_code = 0;
        int step_counter = 0;
        if (fail_step == -1) return;
        /* Perform GET HTTP command in HTTP profile 0 */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_get_HTTP0, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK_HTTP, timeout, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(get_sucess, timeout, error_code);
        if (error_code) return;
    }
    void Inject_mock_syshal_cellular_https_post(int fail_step, int inject_error_code, uint32_t timeout)
    {
        int error_code = 0;
        int step_counter = 0;
        if (fail_step == -1) return;
        /* Perform POST HTTP command in HTTP profile 0 */
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_post_HTTP0, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK_HTTP, timeout, error_code);
        if (error_code) return;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(post_sucess, timeout, error_code);
        if (error_code) return;
    }

    void Inject_mock_syshal_cellular_read_from_file_to_fs(int fail_step, int inject_error_code, fs_handle_t handle)
    {

        int error_code = 0;
        int step_counter = 0;
        int header_size = 73;
        int total_size = 90;
        int rest_size = total_size - header_size;
        static uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

        if (fail_step == -1) return;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_read_file, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(expected_respond_read_file, SYSHAL_CELLULAR_FILE_TIMEOUT_MS, error_code);
        if (error_code) return;


        if (++step_counter == fail_step)
            error_code = inject_error_code;

        expect_simple_mock_num(read_file_mock_success, char_timeout_millisecond  , error_code);
        if (error_code)
        {
            discard_mock(total_size);
            return;
        }

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(read_header_mock, char_timeout_millisecond, error_code);
        if (error_code)
        {
            rest_size = total_size;
            return;
        }

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        read_buffer_simple_mock(buffer, rest_size, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        fs_write_simple_mock(buffer, rest_size, handle, error_code);
        if (error_code) return;
    }

    void Inject_mock_syshal_cellular_read_from_file_to_buffer(int fail_step, int inject_error_code, int b_overflow = 0, int discard_error = SYSHAL_UART_NO_ERROR)
    {

        int error_code = 0;
        int step_counter = 0;
        int header_size = 73;
        int total_size = 90;
        int rest_size = total_size - header_size;
        static uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\n\r\n\"";

        if (fail_step == -1) return;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_read_file, error_code);
        if (error_code) return;
        
        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(expected_respond_read_file, SYSHAL_CELLULAR_FILE_TIMEOUT_MS, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
        {
            expect_simple_mock_num(read_file_mock_fail, char_timeout_millisecond, error_code);
            discard_mock(total_size - strlen((char*) read_file_mock_fail) - 1 , discard_error) ; //
            return;
        }

        expect_simple_mock_num(read_file_mock_success, char_timeout_millisecond, error_code);

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(read_header_mock, char_timeout_millisecond, error_code);
        if (error_code)
        {
            rest_size = total_size;
            return;
        }
        if (!b_overflow)
        {
            if (++step_counter == fail_step)
                error_code = inject_error_code;
            read_buffer_simple_mock(buffer, rest_size, error_code);
            if (error_code) return;
        }
    }


    void Inject_mock_syshal_cellular_write_from_buffer_to_file(int fail_step, int inject_error_code, uint8_t *buffer)
    {

        int error_code = 0;
        int step_counter = 0;
        //const uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

        if (fail_step == -1) return;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(delete_temp, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_TIMEOUT_MS, error_code);
        if (error_code) return;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        mock_flush_simple(error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_simple_mock(send_download_temp, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(start_data, SYSHAL_CELLULAR_FILE_TIMEOUT_MS, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        send_raw_simple_mock(buffer, error_code);
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        expect_simple_mock(OK, SYSHAL_CELLULAR_FILE_TIMEOUT_MS, error_code);
        if (error_code) return;
    }

};

/*          syshal_cellular_poweron             */

TEST_F(CellularTest, syshal_cellular_poweron)
{
    syshal_gpio_set_output_high_ExpectAndReturn(SYSHAL_CELLULAR_GPIO_POWER_ON, SYSHAL_GPIO_NO_ERROR);
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, syshal_cellular_power_on());
}

TEST_F(CellularTest, syshal_cellular_setcom_succes)
{
    Inject_mock_syshal_cellular_setcom(0, SYSHAL_CELLULAR_NO_ERROR);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}

TEST_F(CellularTest, syshal_cellular_setcom_fail_uart_init)
{
    Inject_mock_syshal_cellular_setcom(1, SYSHAL_UART_ERROR_INVALID_INSTANCE);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_setcom_fail_change_baud)
{
    Inject_mock_syshal_cellular_setcom(2, SYSHAL_UART_ERROR_DEVICE);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_setcom_fail_set_baud)
{
    Inject_mock_syshal_cellular_setcom(3, SYSHAL_UART_ERROR_DEVICE);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_setcom_fail_flush)
{
    Inject_mock_syshal_cellular_setcom(4, SYSHAL_UART_ERROR_DEVICE);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_setcom_fail_sent_at)
{
    Inject_mock_syshal_cellular_setcom(5, SYSHAL_UART_ERROR_DEVICE);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_setcom_fail_expect_ok)
{
    Inject_mock_syshal_cellular_setcom(6, SYSHAL_UART_ERROR_DEVICE);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_setcom_fail_flush_2 )
{
    Inject_mock_syshal_cellular_setcom(7, SYSHAL_UART_ERROR_DEVICE);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_setcom_fail_sent_UWMI)
{
    Inject_mock_syshal_cellular_setcom(8, SYSHAL_UART_ERROR_DEVICE);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_setcom_fail_expect_ok_2)
{
    Inject_mock_syshal_cellular_setcom(9, SYSHAL_UART_ERROR_DEVICE);
    int status = syshal_cellular_sync_comms(&error_code_skip);

    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


/*              syshal_cellular_power_off_success               */

TEST_F(CellularTest, syshal_cellular_power_off_success)
{
    /* Arrange */
    Inject_mock_syshal_cellular_power_off();

    /* Act */
    int status = syshal_cellular_power_off();

    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}

/*              syshal_cellular_check_sim_success               */

TEST_F(CellularTest, syshal_cellular_check_sim_success)
{
    uint8_t imsi[15];

    /* Arrange */
    Inject_mock_syshal_cellular_check_sim(0, SYSHAL_UART_NO_ERROR);

    /* Act */
    int status = syshal_cellular_check_sim(imsi, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}

TEST_F(CellularTest, syshal_cellular_check_sim_fail_flush)
{
    uint8_t imsi[15];

    /* Arrange */
    Inject_mock_syshal_cellular_check_sim(1, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_check_sim(imsi, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_check_sim_fail_send)
{
    uint8_t imsi[15];

    /* Arrange */
    Inject_mock_syshal_cellular_check_sim(2, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_check_sim(imsi, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_check_sim_fail_expect_num)
{
    uint8_t imsi[15];

    /* Arrange */
    Inject_mock_syshal_cellular_check_sim(3, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_check_sim(imsi, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_check_sim_fail_expect_ok)
{
    uint8_t imsi[15];

    /* Arrange */
    Inject_mock_syshal_cellular_check_sim(4, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = syshal_cellular_check_sim(imsi, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_UART_ERROR_TIMEOUT, status);
}






/*                                                                  */
/*              syshal_cellular_create_secure_profile               */
/*                                                                  */
TEST_F(CellularTest, syshal_cellular_create_secure_profile_success)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(0, SYSHAL_CELLULAR_NO_ERROR);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}



TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_flush1)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(1, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_send1)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(2, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_expect1)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(3, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);

}





TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_flush2)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(4, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_send2)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(5, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}





TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_expect2)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(6, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}





TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_flush3)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(7, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}



TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_send3)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(8, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_expect3)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(9, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}


TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_flsuh4)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(10, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_send4)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(11, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}



TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_expect4)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(12, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}





TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_flush5)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(13, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_send5)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(14, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_expect5)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(15, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}

TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_flush6)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(16, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_create_secure_profile_fail_send6)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(17, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_create_secure_profile_expect6)
{
    /* Arrange */
    Inject_mock_syshal_cellular_create_secure_profile(18, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = syshal_cellular_create_secure_profile(&error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}








/*                                                                  */
/*              syshal_cellular_set_rat                            */
/*                                                                  */

TEST_F(CellularTest, syshal_cellular_set_rat_succes)
{


    /* Arrange */
    uint32_t timeout = 1000;
    uint16_t error_code;
    Inject_mock_syshal_cellular_set_rat(0, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_set_rat(timeout, SCAN_MODE_2G, &error_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);

}
TEST_F(CellularTest, syshal_cellular_set_rat_fail_flush1)
{

    /* Arrange */
    uint32_t timeout = 1000;
    uint16_t error_code;
    Inject_mock_syshal_cellular_set_rat(1, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_set_rat(timeout, SCAN_MODE_2G, &error_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_set_rat_fail_send1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    uint16_t error_code;
    Inject_mock_syshal_cellular_set_rat(2, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_set_rat(timeout, SCAN_MODE_2G, &error_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_set_rat_fail_expect1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    uint16_t error_code;
    Inject_mock_syshal_cellular_set_rat(3, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_set_rat(timeout, SCAN_MODE_2G, &error_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
    EXPECT_EQ(error_code, EXPECT_ERROR_CODE);
}


/*                                                                  */
/*              syshal_cellular_scan                            */
/*                                                                  */

TEST_F(CellularTest, syshal_cellular_scan_succes)
{

    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_scan_succes(0, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_scan(timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}

TEST_F(CellularTest, syshal_cellular_scan_fail_flush1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_scan_succes(1, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_scan(timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_scan_fail_send1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_scan_succes(2, SYSHAL_CELLULAR_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_scan(timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_scan_fail_expect_timeout)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_scan_succes(5, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_scan(timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}
TEST_F(CellularTest, syshal_cellular_scan_fail_expect_device)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_scan_succes(3, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_scan(timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_scan_fail_sendraw1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_scan_succes(4, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_scan(timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_scan_fail_expect_aborted)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_scan_succes(5, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_scan(timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}







/*                                                                  */
/*              syshal_cellular_attach                            */
/*                                                                  */
TEST_F(CellularTest, syshal_cellular_attach_succes)
{
    /* Arrange */
    uint32_t timeout = 1000;
    uint16_t error_code;
    Inject_mock_syshal_cellular_attach(0, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_attach(timeout, &error_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}

TEST_F(CellularTest, syshal_cellular_attach_fail_flush1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    uint16_t error_code;
    Inject_mock_syshal_cellular_attach(1, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_attach(timeout, &error_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_attach_fail_send1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    uint16_t error_code;
    Inject_mock_syshal_cellular_attach(2, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_attach(timeout, &error_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_attach_expect1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    uint16_t error_code;
    Inject_mock_syshal_cellular_attach(3, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_attach(timeout, &error_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}



/*                                                                */
/*              syshal_cellular_detach                            */
/*                                                                */
TEST_F(CellularTest, syshal_cellular_detach_succes)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_detach(0, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_detach(timeout, &error_code_skip);


    /* Assert */

    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}

TEST_F(CellularTest, syshal_cellular_detach_fail_flush )
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_detach(1, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_detach(timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_detach_fail_send)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_detach(2, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_detach(timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_detach_fail_expect)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_detach(3, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_detach(timeout, &error_code_skip);

    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_success)
{
    syshal_cellular_info_t network_info;

    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;
    uint8_t LAC[] = "011E";
    uint8_t CL[] = "2F0FA78";
    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(0, SYSHAL_UART_NO_ERROR);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);

    /* Assert */
    EXPECT_EQ(network_info.signal_power, signal_power);
    EXPECT_EQ(network_info.technology, tech);
    EXPECT_EQ(network_info.quality, quality);
    EXPECT_STREQ((const char*)network_info.network_operator, (const char*)OPERATOR);
    EXPECT_STREQ((const char*)network_info.local_area_code, (const char*)LAC);
    EXPECT_STREQ((const char*)network_info.cell_id, (const char*)CL);
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_flush1)
{
    syshal_cellular_info_t network_info;

    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(1, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);

    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_network_info_fail_send1)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(2, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_expect11)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(3, SYSHAL_UART_ERROR_TIMEOUT);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_expect12)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(4, SYSHAL_UART_ERROR_TIMEOUT);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_flush2)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(5, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_send2)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(6, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_expect2)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(7, SYSHAL_UART_ERROR_TIMEOUT);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}



TEST_F(CellularTest, syshal_cellular_network_info_fail_flush3)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(8, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_send3)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(9, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_expect3)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(10, SYSHAL_UART_ERROR_TIMEOUT);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_flush4)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(11, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_send4)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(12, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_expect41)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(13, SYSHAL_UART_ERROR_TIMEOUT);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}
TEST_F(CellularTest, syshal_cellular_network_info_fail_expect42)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(14, SYSHAL_UART_ERROR_TIMEOUT);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_flush5)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(15, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_send5)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(16, SYSHAL_UART_ERROR_DEVICE);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_network_info_fail_expect5)
{
    syshal_cellular_info_t network_info;


    /* Arrange */
    uint8_t OPERATOR[] = "vodafone UK";
    uint8_t tech = 2;
    uint8_t signal_power = 29;
    uint8_t quality = 3;

    uint32_t timeout = 1000;

    Inject_mock_syshal_cellular_network_info(17, SYSHAL_UART_ERROR_TIMEOUT);
    /* Act */
    int status = syshal_cellular_network_info(&network_info, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}






/*                                                                  */
/*     syshal_cellular_create_activate_pdp without user   required  */
/*                                                                  */
TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_success)
{

    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(0, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);

}



TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_fail_flush1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(1, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_fail_send1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(2, SYSHAL_UART_ERROR_DEVICE , timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_fail_expect1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(3, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);

}





TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_fail_flush2)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(4, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_fail_send2)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(5, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}





TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_fail_expect2)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(6, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}

TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_fail_flush3)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(7, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_fail_send3)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(8, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}





TEST_F(CellularTest, syshal_cellular_activate_pdp_no_user_fail_expect3)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_no_user(9, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}







/*                                                                  */
/*     syshal_cellular_create_activate_pdp with user   required  */
/*                                                                  */
TEST_F(CellularTest, syshal_cellular_activate_pdp_user_success)
{

    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(0, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);

}



TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_flush1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(1, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_send1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(2, SYSHAL_UART_ERROR_DEVICE , timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_expect1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(3, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);

}





TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_flush2)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(4, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_send2)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(5, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}





TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_expect2)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(6, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}





TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_flush3)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(7, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}



TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_send3)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(8, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_expect3)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(9, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}


TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_flsuh4)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(10, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_send4)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(11, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}



TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_expect4)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(12, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}


TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_flsuh5)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(13, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_send5)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(14, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}



TEST_F(CellularTest, syshal_cellular_activate_pdp_user_fail_expect5)
{
    /* Arrange */
    uint32_t timeout = 1000;
    Inject_mock_syshal_cellular_activate_pdp_user(15, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_activate_pdp(&apn_user, timeout, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}


TEST_F(CellularTest, syshal_cellular_get_HTTP_success)
{

    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_get(0, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}


TEST_F(CellularTest, syshal_cellular_https_config_fail_flush1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(1, SYSHAL_UART_ERROR_DEVICE);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);
    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_https_config_fail_send1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(2, SYSHAL_UART_ERROR_DEVICE);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_https_config_fail_expect1)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(3, SYSHAL_UART_ERROR_TIMEOUT);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);

}





TEST_F(CellularTest, syshal_cellular_https_config_fail_flush2)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(4, SYSHAL_UART_ERROR_DEVICE);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_https_config_fail_send2)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(5, SYSHAL_UART_ERROR_DEVICE);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}





TEST_F(CellularTest, syshal_cellular_https_config_fail_expect2)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(6, SYSHAL_UART_ERROR_TIMEOUT);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}





TEST_F(CellularTest, syshal_cellular_https_config_fail_flush3)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(7, SYSHAL_UART_ERROR_DEVICE);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}



TEST_F(CellularTest, syshal_cellular_https_config_fail_send3)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(8, SYSHAL_UART_ERROR_DEVICE);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_https_config_fail_expect3)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(9, SYSHAL_UART_ERROR_TIMEOUT);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}


TEST_F(CellularTest, syshal_cellular_https_config_fail_flsuh4)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(10, SYSHAL_UART_ERROR_DEVICE);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_https_config_fail_send4)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(11, SYSHAL_UART_ERROR_DEVICE);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}



TEST_F(CellularTest, syshal_cellular_https_config_fail_expect4)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(12, SYSHAL_UART_ERROR_TIMEOUT);
    Inject_mock_syshal_cellular_https_get(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}





TEST_F(CellularTest, syshal_cellular_https_config_fail_flush5)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_get(1, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}





TEST_F(CellularTest, syshal_cellular_https_config_fail_send5)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_get(2, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_https_config_fail_expect5)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_get(3, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}
TEST_F(CellularTest, syshal_cellular_https_config_fail_get_sucess)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_get(4, SYSHAL_UART_ERROR_TIMEOUT, timeout);

    /* Act */
    int status = syshal_cellular_https_get(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}
TEST_F(CellularTest, syshal_cellular_post_HTTP_success)
{

    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_post(0, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_post(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);

}


TEST_F(CellularTest, syshal_cellular_post_HTTP_fail_flush)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_post(1, SYSHAL_UART_ERROR_DEVICE, timeout);

    /* Act */
    int status = syshal_cellular_https_post(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_post_HTTP_fail_send)
{

    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_post(2, SYSHAL_UART_ERROR_DEVICE , timeout);

    /* Act */
    int status = syshal_cellular_https_post(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_post_HTTP_fail_expect)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_post(3, SYSHAL_UART_ERROR_TIMEOUT , timeout);

    /* Act */
    int status = syshal_cellular_https_post(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}

TEST_F(CellularTest, syshal_cellular_post_HTTP_fail_get_success)
{
    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(0, SYSHAL_UART_NO_ERROR);
    Inject_mock_syshal_cellular_https_post(4, SYSHAL_UART_ERROR_TIMEOUT , timeout);

    /* Act */
    int status = syshal_cellular_https_post(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}

TEST_F(CellularTest, syshal_cellular_post_HTTP_fail_config)
{

    /* Arrange */
    uint32_t timeout = 1000;
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    Inject_mock_syshal_cellular_https_config(1, SYSHAL_UART_ERROR_DEVICE);
    Inject_mock_syshal_cellular_https_post(-1, SYSHAL_UART_NO_ERROR, timeout);

    /* Act */
    int status = syshal_cellular_https_post(timeout, domain, port, path, &error_code_skip);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}


TEST_F(CellularTest, syshal_cellular_read_from_file_to_fs_success)
{

    //CONFIGURE HTTP

    /* Arrange */
    fs_handle_t handle;
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint16_t http_code;


    Inject_mock_syshal_cellular_read_from_file_to_fs(0, SYSHAL_UART_NO_ERROR, handle);

    /* Act */
    int status = syshal_cellular_read_from_file_to_fs(handle, &http_code, NULL);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);
}
TEST_F(CellularTest, syshal_cellular_read_from_file_to_fs_fail_flush)
{

    //CONFIGURE HTTP

    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout = 1000;
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_fs(1, SYSHAL_UART_ERROR_DEVICE, handle);

    /* Act */
    int status = syshal_cellular_read_from_file_to_fs(handle, &http_code, NULL);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_read_from_file_to_fs_fail_send1)
{

    //CONFIGURE HTTP

    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout = 1000;
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_fs(2, SYSHAL_UART_ERROR_DEVICE, handle);

    /* Act */
    int status = syshal_cellular_read_from_file_to_fs(handle, &http_code, NULL);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_read_from_file_to_fs_fail_expect1)
{


    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout = 1000;
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint16_t http_code;


    Inject_mock_syshal_cellular_read_from_file_to_fs(3, SYSHAL_UART_ERROR_TIMEOUT, handle);

    /* Act */
    int status = syshal_cellular_read_from_file_to_fs(handle, &http_code, NULL);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);
}
TEST_F(CellularTest, syshal_cellular_read_from_file_to_fs_fail_buffer1)
{


    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout = 1000;
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_fs(5, SYSHAL_UART_ERROR_DEVICE, handle);

    /* Act */
    int status = syshal_cellular_read_from_file_to_fs(handle, &http_code, NULL);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}
TEST_F(CellularTest, syshal_cellular_read_from_file_to_fs_fail_fs)
{

    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout = 1000;
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_fs(5, FS_ERROR_BAD_DEVICE, handle);

    /* Act */
    int status = syshal_cellular_read_from_file_to_fs(handle, &http_code, NULL);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}

TEST_F(CellularTest, syshal_cellular_read_from_file_to_fs_fail_read_raw)
{

    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout = 1000;
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_fs(6, SYSHAL_UART_ERROR_DEVICE, handle);

    /* Act */
    int status = syshal_cellular_read_from_file_to_fs(handle, &http_code, NULL);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);
}



TEST_F(CellularTest, syshal_cellular_read_from_file_to_buffer_success)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint8_t buffer_receive[MAX_AT_BUFFER_SIZE];
    uint32_t bytes_received;
    uint16_t http_code;
    Inject_mock_syshal_cellular_read_from_file_to_buffer(0, SYSHAL_UART_NO_ERROR);

    /* Act */
    int status = syshal_cellular_read_from_file_to_buffer(buffer_receive, MAX_AT_BUFFER_SIZE, &bytes_received, &http_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);

}
TEST_F(CellularTest, syshal_cellular_read_from_file_to_buffer_fail_flush1)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint8_t buffer_receive[MAX_AT_BUFFER_SIZE];
    uint32_t bytes_received;
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_buffer(1, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_read_from_file_to_buffer(buffer_receive, MAX_AT_BUFFER_SIZE, &bytes_received, &http_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}

TEST_F(CellularTest, syshal_cellular_read_from_file_to_buffer_fail_send1)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint8_t buffer_receive[MAX_AT_BUFFER_SIZE];
    uint32_t bytes_received;
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_buffer(2, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_read_from_file_to_buffer(buffer_receive, MAX_AT_BUFFER_SIZE, &bytes_received, &http_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}

TEST_F(CellularTest, syshal_cellular_read_from_file_to_buffer_fail_ecpect1)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint8_t buffer_receive[MAX_AT_BUFFER_SIZE];
    uint32_t bytes_received;
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_buffer(3, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = syshal_cellular_read_from_file_to_buffer(buffer_receive, MAX_AT_BUFFER_SIZE, &bytes_received, &http_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);

}
TEST_F(CellularTest, syshal_cellular_read_from_file_to_buffer_fail_http)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint8_t buffer_receive[MAX_AT_BUFFER_SIZE];
    uint32_t bytes_received;
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_buffer(4, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = syshal_cellular_read_from_file_to_buffer(buffer_receive, MAX_AT_BUFFER_SIZE, &bytes_received, &http_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_HTTP, status);

}
TEST_F(CellularTest, syshal_cellular_read_from_file_to_buffer_fail_http_discard)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint8_t buffer_receive[MAX_AT_BUFFER_SIZE];
    uint32_t bytes_received;
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_buffer(4, SYSHAL_UART_ERROR_TIMEOUT, 1, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_read_from_file_to_buffer(buffer_receive, MAX_AT_BUFFER_SIZE, &bytes_received, &http_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}
TEST_F(CellularTest, syshal_cellular_read_from_file_to_buffer_fail_buffer)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint8_t buffer_receive[MAX_AT_BUFFER_SIZE];
    uint32_t bytes_received;
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_buffer(3, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_read_from_file_to_buffer(buffer_receive, MAX_AT_BUFFER_SIZE, &bytes_received, &http_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}

TEST_F(CellularTest, syshal_cellular_read_from_file_to_buffer_fail_buffer_overflow)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint8_t buffer_receive[MAX_AT_BUFFER_SIZE];
    uint32_t bytes_received;
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_buffer(3, SYSHAL_UART_NO_ERROR, 1);

    /* Act */
    int status = syshal_cellular_read_from_file_to_buffer(buffer_receive, 10, &bytes_received, &http_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_BUFFER_OVERFLOW, status);

}



TEST_F(CellularTest, syshal_cellular_read_from_file_to_buffer_fail_buffer_read_raw)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    uint8_t buffer_receive[MAX_AT_BUFFER_SIZE];
    uint32_t bytes_received;
    uint16_t http_code;

    Inject_mock_syshal_cellular_read_from_file_to_buffer(5, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = syshal_cellular_read_from_file_to_buffer(buffer_receive, 10, &bytes_received, &http_code);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}


TEST_F(CellularTest, syshal_cellular_write_from_buffer_to_file)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

    Inject_mock_syshal_cellular_write_from_buffer_to_file(0, SYSHAL_UART_NO_ERROR, buffer);

    /* Act */
    int status = syshal_cellular_write_from_buffer_to_file(buffer, 140);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_NO_ERROR, status);

}

TEST_F(CellularTest, syshal_cellular_write_from_buffer_to_file_fail_flush1)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";


    Inject_mock_syshal_cellular_write_from_buffer_to_file(1, SYSHAL_UART_ERROR_DEVICE, buffer);

    /* Act */
    int status = syshal_cellular_write_from_buffer_to_file(buffer, 140);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}
TEST_F(CellularTest, syshal_cellular_write_from_buffer_to_file_fail_send1)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";


    Inject_mock_syshal_cellular_write_from_buffer_to_file(2, SYSHAL_UART_ERROR_DEVICE, buffer);

    /* Act */
    int status = syshal_cellular_write_from_buffer_to_file(buffer, 140);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}
TEST_F(CellularTest, syshal_cellular_write_from_buffer_to_filefail_expected1)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

    Inject_mock_syshal_cellular_write_from_buffer_to_file(3, SYSHAL_UART_ERROR_TIMEOUT, buffer);

    /* Act */
    int status = syshal_cellular_write_from_buffer_to_file(buffer, 140);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);

}
TEST_F(CellularTest, syshal_cellular_write_from_buffer_to_file_fail_flush2)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

    Inject_mock_syshal_cellular_write_from_buffer_to_file(4, SYSHAL_UART_ERROR_DEVICE, buffer);

    /* Act */
    int status = syshal_cellular_write_from_buffer_to_file(buffer, 140);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}
TEST_F(CellularTest, syshal_cellular_write_from_buffer_to_file_fail_send2)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

    Inject_mock_syshal_cellular_write_from_buffer_to_file(5, SYSHAL_UART_ERROR_DEVICE, buffer);

    /* Act */
    int status = syshal_cellular_write_from_buffer_to_file(buffer, 140);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}
TEST_F(CellularTest, syshal_cellular_write_from_buffer_to_filefail_expected2)
{
    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

    Inject_mock_syshal_cellular_write_from_buffer_to_file(6, SYSHAL_UART_ERROR_TIMEOUT, buffer);

    /* Act */
    int status = syshal_cellular_write_from_buffer_to_file(buffer, 140);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);

}
TEST_F(CellularTest, syshal_cellular_write_from_buffer_to_file_fail_buffer)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

    Inject_mock_syshal_cellular_write_from_buffer_to_file(7, SYSHAL_UART_ERROR_DEVICE, buffer);

    /* Act */
    int status = syshal_cellular_write_from_buffer_to_file(buffer, 140);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_DEVICE, status);

}
TEST_F(CellularTest, syshal_cellular_write_from_buffer_to_file_fail_buffer_expect)
{

    /* Arrange */
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

    Inject_mock_syshal_cellular_write_from_buffer_to_file(8, SYSHAL_UART_ERROR_TIMEOUT, buffer);

    /* Act */
    int status = syshal_cellular_write_from_buffer_to_file(buffer, 140);


    /* Assert */
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, status);

}