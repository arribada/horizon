// test_at.cpp - AT unit tests
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
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cmath>

#define DEBUG
#define MICROSECONDS (1000000)
#define MILLI_TO_MICROSECONDS(x) (x * 1000)
#define BITS_PER_CHARACTER (10)

class ATTest : public ::testing::Test
{
public:
    uint32_t uart_device_priv = 1;
    uint32_t char_timeout_microsecond;
    uint32_t char_timeout_millisecond;
    uint32_t char_timeout_second;

    virtual void SetUp()
    {
        Mocksyshal_uart_Init();
        Mockfs_Init();
        uint32_t baudrate = 115200;
        syshal_uart_get_baud_ExpectAndReturn(uart_device_priv, &baudrate, SYSHAL_UART_NO_ERROR);
        syshal_uart_get_baud_ReturnThruPtr_baudrate(&baudrate);
        syshal_uart_get_baud_IgnoreArg_baudrate();

        syshal_uart_change_baud_IgnoreAndReturn(SYSHAL_UART_NO_ERROR);

        at_init(uart_device_priv);

        char_timeout_microsecond = (BITS_PER_CHARACTER * TIMES_CHAR_TIMEOUT * 1000000) / baudrate;
        char_timeout_millisecond = (BITS_PER_CHARACTER * TIMES_CHAR_TIMEOUT * 1000) / baudrate;
        if (char_timeout_millisecond == 0)
            char_timeout_millisecond = 1;
        char_timeout_second = (BITS_PER_CHARACTER * TIMES_CHAR_TIMEOUT) / baudrate;
    }

    virtual void TearDown()
    {
        Mocksyshal_uart_Verify();
        Mocksyshal_uart_Destroy();
        Mockfs_Verify();
        Mockfs_Destroy();
    }

public:

    int send_simple(uint8_t *command)
    {
        uint8_t command_received[MAX_AT_COMMAND_SIZE] = "AT+";
        strcat((char*) command_received, (char*)command);
        uint32_t len = strlen((char*)command_received) + 1;
        command_received[len] = '\r';
        syshal_uart_send_ExpectAndReturn(uart_device_priv, command_received, len, SYSHAL_UART_NO_ERROR);
        return at_send(command);
    }

    int send_raw_simple(uint8_t *command)
    {
        uint8_t command_received[MAX_AT_COMMAND_SIZE];

        strcat((char*) command_received, (char*)command);

        uint32_t len = strlen((char*)command_received) + 1;

        command_received[len] = '\r';

        syshal_uart_send_ExpectAndReturn(uart_device_priv, command_received, len, SYSHAL_UART_NO_ERROR);

        return at_send_raw_with_cr(command, strlen((char *)command) + 1);
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
        uint32_t temp_timeout = MILLI_TO_MICROSECONDS(timeout);
        uint32_t bytes_received = 1;
        uint32_t length_read;
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
        status = at_expect(command_expected, timeout, &length_read);
        return status;
    }

    int expect_simple_num(uint8_t *command_expected, uint32_t *value, uint8_t *command_received_un, uint32_t timeout, uint32_t complete_len)
    {
        uint8_t *ptr_received = command_received_un;
        uint32_t temp_timeout = MILLI_TO_MICROSECONDS(timeout);
        uint32_t bytes_received = 1;
        uint32_t length_read;
        int status;

        command_received_un[strlen((char*)command_received_un)] = '\r';

        for (int i = 0; i < complete_len; ++i)
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_received, 1, temp_timeout, char_timeout_microsecond, &bytes_received, SYSHAL_UART_NO_ERROR);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_received, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(&bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            temp_timeout = char_timeout_microsecond;
            ptr_received++;
        }
        status = at_expect(command_expected, timeout, value, &length_read);
        return status;
    }

    int read_buffer_simple(uint8_t *buffer, uint32_t len, uint32_t timeout)
    {
        fs_handle_t handle; // It has to be created before
        uint32_t bytes_received = len;
        int status;
        uint32_t fs_written;

        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, NULL, len, MILLI_TO_MICROSECONDS(timeout), char_timeout_microsecond, &bytes_received, SYSHAL_UART_NO_ERROR);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(buffer, len);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(&bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        fs_written = len;

        fs_write_ExpectAndReturn(handle, buffer, len, NULL, FS_NO_ERROR);
        fs_write_ReturnThruPtr_written(&fs_written);
        fs_write_IgnoreArg_written();

        status  = at_read_raw_to_fs( timeout, len, handle);
        return status;
    }

    int flush_simple()
    {
        syshal_uart_flush_ExpectAndReturn(uart_device_priv, SYSHAL_UART_NO_ERROR);
        return at_flush();
    }

    void inject_mock_init(int fail_step, int inject_error_code)
    {

        int error_code = 0;
        int step_counter = 0;

        uint32_t baudrate = 115200;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        syshal_uart_get_baud_ExpectAndReturn(uart_device_priv, &baudrate, error_code);
        syshal_uart_get_baud_ReturnThruPtr_baudrate(&baudrate);
        syshal_uart_get_baud_IgnoreArg_baudrate();
        if (error_code) return;


    }
    void inject_mock_send(int fail_step, int inject_error_code)
    {

        int error_code = 0;
        int step_counter = 0;
        uint8_t data[MAX_AT_COMMAND_SIZE] = "AT+Check1: 123400 Check2: True";
        uint32_t len = strlen((char*)data) + 1;
        data[len] = '\r';

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        if (error_code) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        syshal_uart_send_ExpectAndReturn(uart_device_priv, data, len, error_code);
        if (error_code) return;
    }

    void inject_mock_send_raw(int fail_step, int inject_error_code)
    {

        int error_code = 0;
        int step_counter = 0;

        static uint8_t data[MAX_AT_COMMAND_SIZE] = "Check1: 123400 Check2: True";
        static uint8_t CR_char = '\r';
        uint32_t len = strlen((char*)data);

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        syshal_uart_send_ExpectAndReturn(uart_device_priv, data, len, error_code);
        if (error_code) return;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        syshal_uart_send_ExpectAndReturn(uart_device_priv, &CR_char, sizeof(CR_char), error_code);
        if (error_code) return;

    }

    void inject_mock_read_raw_to_fs(int fail_step, int inject_error_code, fs_handle_t *handle_out, uint32_t *timeout_out, uint32_t *read_len_out)
    {

        int error_code = 0;
        int step_counter = 0;

        uint32_t timeout = 10;
        uint32_t *fs_written = new uint32_t;
        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;
        fs_handle_t handle;

        uint8_t read_data[MAX_AT_BUFFER_SIZE] = "Check1: 123400 Check2: True and other things...";
        uint32_t read_len = strlen((char*)read_data);
        read_data[read_len] = '\r';
        *bytes_received = read_len;


        *handle_out = handle;
        *timeout_out = timeout;
        *read_len_out = read_len;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, read_data, read_len, MILLI_TO_MICROSECONDS(timeout), char_timeout_microsecond, bytes_received, error_code);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(read_data, read_len);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        if (error_code) return;

        *fs_written = read_len;
        /* bytes written not equal bytes read */
        if (++step_counter == fail_step)
        {

            *fs_written = read_len - 1;
        }

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        fs_write_ExpectAndReturn(handle, read_data, read_len, NULL, error_code);
        fs_write_ReturnThruPtr_written(fs_written);
        fs_write_IgnoreArg_written();
        if (error_code) return;

    }

    void inject_mock_read_raw_to_buffer(int fail_step, int inject_error_code, uint32_t *timeout_out, uint32_t *read_len_out)
    {
        int error_code = 0;
        int step_counter = 0;

        int timeout = 10;
        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;


        uint8_t read_data[MAX_AT_BUFFER_SIZE] = "Check1: 123400 Check2: True and other things...";
        uint32_t read_len = 15;

        *timeout_out = timeout;
        *read_len_out = read_len;

        *bytes_received = read_len;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, read_data, read_len, MILLI_TO_MICROSECONDS(timeout), char_timeout_microsecond, bytes_received, error_code);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(read_data, read_len);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        if (error_code) return;
    }

    void  inject_mock_flush(int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;

        if (++step_counter == fail_step)
            error_code = inject_error_code;
        syshal_uart_flush_ExpectAndReturn(uart_device_priv, error_code);
        if (error_code) return;
    }

    void inject_mock_at_discard(int fail_step, int inject_error_code, int iteration_inject,  uint32_t length)
    {

        int error_code = 0;
        int step_counter = 0;

        int internal_counter = 0;
        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;
        uint8_t buffer = 'A';

        step_counter++;
        while (internal_counter++ < length)
        {
            if (step_counter == fail_step && iteration_inject == internal_counter)
                error_code = inject_error_code;
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, &buffer, sizeof(buffer), char_timeout_microsecond, char_timeout_microsecond, bytes_received, error_code);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(&buffer, sizeof(buffer));
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            if (error_code) return;
        }

    }
    void inject_mock_at_expect(int fail_step, int inject_error_code, int number_error,  uint32_t *timeout_out)
    {
        int error_code = 0;
        int step_counter = 0;

        uint8_t data[MAX_AT_BUFFER_SIZE] = "Check1: 123400 Check2: True and other things...";

        uint8_t command_expected[MAX_AT_BUFFER_SIZE] = "Check1: 123400 Check2: True ";
        uint32_t data_length =  strlen((char*)command_expected);



        uint32_t timeout = 10;
        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;
        uint8_t *ptr_data = data;
        int status;
        uint32_t internal_counter = 0;

        *timeout_out = timeout;


        if (++step_counter == fail_step)
            error_code = inject_error_code;
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, MILLI_TO_MICROSECONDS(timeout), char_timeout_microsecond, bytes_received, error_code);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        if (error_code) return;

        step_counter++;

        while (internal_counter++ < data_length - 1)
        {
            if (step_counter == fail_step && number_error == internal_counter)
                error_code = inject_error_code;
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, char_timeout_microsecond, char_timeout_microsecond, bytes_received, error_code);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            if (error_code) return;
        }

    }
    void inject_mock_at_expect_last_line(int fail_step, int inject_error_code, int number_error,  uint32_t *timeout_out)
    {
        int error_code = 0;
        int step_counter = 0;

        uint8_t data[MAX_AT_BUFFER_SIZE] = "+CME ERROR: 111\r";
        uint8_t data_error[MAX_AT_BUFFER_SIZE] = "NO DATA VALID";
        uint32_t data_length =  strlen((char*)data);
        uint32_t data_length_error =  strlen((char*)data_error);


        uint32_t timeout = 10;
        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;
        uint8_t *ptr_data = data;
        int status;
        uint32_t internal_counter = 0;

        *timeout_out = timeout;

        if (++step_counter == fail_step)
        {
            ptr_data = data_error;
            data_length = data_length_error;
        }
        if(++step_counter == fail_step)
        {
            return;
        }
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, MILLI_TO_MICROSECONDS(timeout), char_timeout_microsecond, bytes_received, error_code);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();

        while (internal_counter++ < data_length - 1)
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, char_timeout_microsecond, char_timeout_microsecond, bytes_received, error_code);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
        }
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, char_timeout_microsecond, char_timeout_microsecond, bytes_received, SYSHAL_UART_ERROR_TIMEOUT);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
    }

    void inject_mock_at_expect_http_header(int fail_step, int inject_error_code)
    {
        int error_code = 0;
        int step_counter = 0;

        uint8_t data[] = "HTTP/1.1 200 OK \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\n\r\n\"";
        uint8_t data_wrong[] = "HTTP/1.1 400 OK \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\n\r\n\"";
        uint32_t total_len = strlen((char*) data);
        bool http_code_wrong = false;
        uint8_t *ptr_data = data;
        uint32_t http_code_len = 12;
        uint32_t internal_counter = 0;
        uint32_t *bytes_received = new uint32_t;
        *bytes_received = 1;
        uint8_t command_expected[MAX_AT_BUFFER_SIZE] = "Check1: 123400 Check2: True ";
        uint32_t data_length =  strlen((char*)command_expected);

        if (++step_counter == fail_step)
        {
            ptr_data = data_wrong;
            http_code_wrong = true;
        }


        if (++step_counter == fail_step)
            error_code = inject_error_code;

        // Calculate the nearest millisecond
        uint32_t char_timeout_microsecond_rounded = ceil((float) char_timeout_microsecond / 1000.0f) * 1000 - 1000;

        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, char_timeout_microsecond_rounded, char_timeout_microsecond, bytes_received, error_code);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        if (error_code) return;

        while (internal_counter++ < http_code_len )
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, char_timeout_microsecond, char_timeout_microsecond, bytes_received, error_code);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            if (error_code) return;
        }
        if (http_code_wrong) return;

        if (++step_counter == fail_step)
            error_code = inject_error_code;

        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, char_timeout_microsecond_rounded, char_timeout_microsecond, bytes_received, error_code);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        if (error_code) return;

        while (internal_counter++ < (total_len - 2))
        {
            syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, char_timeout_microsecond, char_timeout_microsecond, bytes_received, error_code);
            syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
            syshal_uart_read_timeout_IgnoreArg_buffer();
            syshal_uart_read_timeout_ReturnThruPtr_bytes_received(bytes_received);
            syshal_uart_read_timeout_IgnoreArg_bytes_received();
            if (error_code) return;
        }

    }
};



TEST_F(ATTest, at_init_success)
{
    /* Arrange */
    inject_mock_init(0, AT_NO_ERROR);

    /* Act */
    int status =  at_init(uart_device_priv);

    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);

}

TEST_F(ATTest, at_init_fail_number_instances)
{
    /* Arrange */
    inject_mock_init(1, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status =  at_init(200);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}

TEST_F(ATTest, at_init_fail_get_baud)
{
    /* Arrange */
    inject_mock_init(2, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status =  at_init(uart_device_priv);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}


TEST_F(ATTest, at_send_success)
{
    /* Arrange */
    inject_mock_send(0, SYSHAL_UART_NO_ERROR);
    uint8_t command[MAX_AT_COMMAND_SIZE] = "Check1: %u Check2: %s";

    /* Act */
    int status = at_send(command, 123400, "True");

    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);
}

TEST_F(ATTest, at_send_fail_format_not_supported)
{
    /* Arrange */
    inject_mock_send(1, AT_ERROR_DEVICE);
    uint8_t command[MAX_AT_COMMAND_SIZE] = "Check1: %d Check2: %s";

    /* Act */
    int status = at_send(command, 123400, "True");

    /* Assert */
    EXPECT_EQ(AT_ERROR_FORMAT_NOT_SUPPORTED, status);
}

TEST_F(ATTest, at_send_fail_sending_error)
{
    /* Arrange */
    inject_mock_send(2, SYSHAL_UART_ERROR_DEVICE);
    uint8_t command[MAX_AT_COMMAND_SIZE] = "Check1: %u Check2: %s";

    /* Act */
    int status = at_send(command, 123400, "True");

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}


TEST_F(ATTest, at_send_raw_success)
{
    /* Arrange */
    inject_mock_send_raw(0, SYSHAL_UART_NO_ERROR);
    uint8_t command[MAX_AT_COMMAND_SIZE] = "Check1: 123400 Check2: True";

    /* Act */
    int status = at_send_raw_with_cr(command, strlen((char *)command) );

    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);
}

TEST_F(ATTest, at_send_raw_fail_first_send)
{
    /* Arrange */
    inject_mock_send_raw(1, SYSHAL_UART_ERROR_DEVICE);
    uint8_t command[MAX_AT_COMMAND_SIZE] = "Check1: 123400 Check2: True";

    /* Act */
    int status = at_send_raw_with_cr(command, strlen((char *)command) );

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}

TEST_F(ATTest, at_send_raw_fail_second_send)
{
    /* Arrange */
    inject_mock_send_raw(2, SYSHAL_UART_ERROR_DEVICE);
    uint8_t command[MAX_AT_COMMAND_SIZE] = "Check1: 123400 Check2: True";

    /* Act */
    int status = at_send_raw_with_cr(command, strlen((char *)command) );

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}




TEST_F(ATTest, at_raw_to_fs)
{
    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout;
    uint32_t read_len;
    inject_mock_read_raw_to_fs(0, SYSHAL_UART_NO_ERROR, &handle, &timeout, &read_len);

    /* Act */
    int status  = at_read_raw_to_fs( timeout, read_len, handle );

    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);
}

TEST_F(ATTest, at_raw_to_fs_fail_read_uart)
{
    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout;
    uint32_t read_len;
    inject_mock_read_raw_to_fs(1, SYSHAL_UART_ERROR_TIMEOUT, &handle, &timeout, &read_len);

    /* Act */
    int status  = at_read_raw_to_fs( timeout, read_len, handle );

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status);
}
TEST_F(ATTest, at_raw_to_fs_fail_read_uart_device)
{
    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout;
    uint32_t read_len;
    inject_mock_read_raw_to_fs(1, SYSHAL_UART_ERROR_DEVICE, &handle, &timeout, &read_len);

    /* Act */
    int status  = at_read_raw_to_fs( timeout, read_len, handle );

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}

TEST_F(ATTest, at_raw_to_fs_fail_fs_write_not_equal)
{
    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout;
    uint32_t read_len;
    inject_mock_read_raw_to_fs(2, SYSHAL_UART_ERROR_DEVICE, &handle, &timeout, &read_len);

    /* Act */
    int status  = at_read_raw_to_fs( timeout, read_len, handle );

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}

TEST_F(ATTest, at_raw_to_fs_fail_fs_write)
{
    /* Arrange */
    fs_handle_t handle;
    uint32_t timeout;
    uint32_t read_len;
    inject_mock_read_raw_to_fs(3, SYSHAL_UART_ERROR_DEVICE, &handle, &timeout, &read_len);

    /* Act */
    int status  = at_read_raw_to_fs( timeout, read_len, handle );

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}






TEST_F(ATTest, at_read_raw_to_fs_large)
{
    /* Arrange */
    int timeout = 10;
    uint32_t fs_written;
    uint32_t bytes_received;
    fs_handle_t handle;

    uint8_t read_data[(MAX_AT_BUFFER_SIZE * 10)];
    for (int i = 0; i < MAX_AT_BUFFER_SIZE * 10; ++i)
    {
        read_data[i] = '0' + (i % 10);
    }
    uint32_t read_len = strlen((char*)read_data) + 1;
    read_data[read_len] = '\r';
    bytes_received = MAX_AT_BUFFER_SIZE;
    for (int i = 0; i < 10; ++i)
    {
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, read_data, MAX_AT_BUFFER_SIZE, MILLI_TO_MICROSECONDS(timeout), char_timeout_microsecond, &bytes_received, SYSHAL_UART_NO_ERROR);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(&read_data[i * MAX_AT_BUFFER_SIZE], MAX_AT_BUFFER_SIZE);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(&bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        fs_written = bytes_received;

        fs_write_ExpectAndReturn(handle, &read_data[i * MAX_AT_BUFFER_SIZE], bytes_received, NULL, FS_NO_ERROR);
        fs_write_ReturnThruPtr_written(&fs_written);
        fs_write_IgnoreArg_written();
    }

    /* Act */
    int status  = at_read_raw_to_fs( timeout, (MAX_AT_BUFFER_SIZE * 10), handle );


    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);
}



TEST_F(ATTest, at_read_raw_to_buffer)
{

    /* Arrange */
    uint32_t timeout;
    uint32_t read_len;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_read_raw_to_buffer(0, SYSHAL_UART_NO_ERROR, &timeout, &read_len);

    /* Act */
    int status  = at_read_raw_to_buffer( timeout, read_len, buffer);

    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);

}
TEST_F(ATTest, at_read_raw_to_buffer_fail_read)
{

    /* Arrange */
    uint32_t timeout;
    uint32_t read_len;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_read_raw_to_buffer(1, SYSHAL_UART_ERROR_DEVICE, &timeout, &read_len);

    /* Act */
    int status  = at_read_raw_to_buffer( timeout, read_len, buffer);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);

}
TEST_F(ATTest, at_read_raw_to_buffer_fail_read_timeout)
{

    /* Arrange */
    uint32_t timeout;
    uint32_t read_len;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_read_raw_to_buffer(1, SYSHAL_UART_ERROR_TIMEOUT, &timeout, &read_len);

    /* Act */
    int status  = at_read_raw_to_buffer( timeout, read_len, buffer);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status);

}

TEST_F(ATTest, at_flush)
{

    /* Arrange */
    inject_mock_flush(0, SYSHAL_UART_NO_ERROR);

    /* Act */
    int status  = at_flush();

    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);

}

TEST_F(ATTest, at_flush_error)
{

    /* Arrange */
    inject_mock_flush(1, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status  = at_flush();

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);

}

TEST_F(ATTest, at_discard)
{

    /* Arrange */
    uint32_t length = 10;
    inject_mock_at_discard(0, SYSHAL_UART_NO_ERROR, 0, length);

    /* Act */
    int status  = at_discard(length);

    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);
}

TEST_F(ATTest, at_discard_error_timeout)
{

    /* Arrange */
    uint32_t length = 10;
    inject_mock_at_discard(1, SYSHAL_UART_ERROR_TIMEOUT, 1, length);

    /* Act */
    int status  = at_discard(length);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status);
}
TEST_F(ATTest, at_discard_error_device)
{

    /* Arrange */
    uint32_t length = 10;
    inject_mock_at_discard(1, SYSHAL_UART_ERROR_DEVICE, 9, length);

    /* Act */
    int status  = at_discard(length);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}


TEST_F(ATTest, at_expect)
{

    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_at_expect(0, SYSHAL_UART_NO_ERROR, 0,  &timeout);

    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);

    EXPECT_STREQ("True", (char*)buffer);

    EXPECT_EQ(123400, value);
}


TEST_F(ATTest, at_expect_last_line)
{

    /* Arrange */
    uint8_t command_expected[MAX_AT_BUFFER_SIZE] = "OK";
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "+CME ERROR: %u";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    inject_mock_at_expect_last_line(0, SYSHAL_UART_NO_ERROR, 0,  &timeout);

    /* Act */
    int status_expect = at_expect(command_expected, timeout, &length_read, &value);

    int status_expect_last_line = at_expect_last_line(command_formated, &value);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status_expect);
    EXPECT_EQ(AT_NO_ERROR, status_expect_last_line);

    EXPECT_EQ(111, value);
}
TEST_F(ATTest, at_expect_last_line_error)
{

    /* Arrange */
    uint8_t command_expected[MAX_AT_BUFFER_SIZE] = "OK";
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "+CME ERROR: %u";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;

    inject_mock_at_expect_last_line(1, SYSHAL_UART_NO_ERROR, 0,  &timeout);

    /* Act */
    int status_expect = at_expect(command_expected, timeout, &length_read, &value);

    int status_expect_last_line = at_expect_last_line(command_formated, &value);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status_expect);
    EXPECT_EQ(AT_ERROR_UNEXPECTED_RESPONSE, status_expect_last_line);

}

TEST_F(ATTest, at_expect_last_line_error_no_expect)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "+CME ERROR: %u";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    inject_mock_at_expect_last_line(2, SYSHAL_UART_NO_ERROR, 0,  &timeout);

    /* Act */

    int status = at_expect_last_line(command_formated, &value);

    /* Assert */
    EXPECT_EQ(AT_ERROR_UNEXPECTED_RESPONSE, status);

}

TEST_F(ATTest, at_expect_fail)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_at_expect(1, SYSHAL_UART_ERROR_DEVICE, 0,  &timeout);

    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}

TEST_F(ATTest, at_expect_fail_timeout)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_at_expect(1, SYSHAL_UART_ERROR_TIMEOUT, 0,  &timeout);

    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status);

}


TEST_F(ATTest, at_expect_fail_num_device)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_at_expect(2, SYSHAL_UART_ERROR_DEVICE, 10,  &timeout);

    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}
TEST_F(ATTest, at_expect_fail_num_timeout)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_at_expect(2, SYSHAL_UART_ERROR_TIMEOUT, 10,  &timeout);

    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status);
}


TEST_F(ATTest, at_expect_fail_string)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_at_expect(2, SYSHAL_UART_ERROR_DEVICE, 24,  &timeout);

    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}

TEST_F(ATTest, at_expect_fail_not_suported)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "%d Check2: %s ";
    uint8_t buffer = 'A';

    uint32_t length_read;
    uint32_t timeout = 10;
    uint32_t value;
    uint32_t bytes_received = 1;

    uint8_t *ptr_data = command_formated;
    syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_data, 1, MILLI_TO_MICROSECONDS(timeout), char_timeout_microsecond, &bytes_received, SYSHAL_UART_NO_ERROR);
    syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_data++, 1);
    syshal_uart_read_timeout_IgnoreArg_buffer();
    syshal_uart_read_timeout_IgnoreArg_bytes_received();
    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);
    /* Assert */
    EXPECT_EQ(AT_ERROR_FORMAT_NOT_SUPPORTED, status);
}





TEST_F(ATTest, at_expect_fail_string_timeout)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_at_expect(2, SYSHAL_UART_ERROR_TIMEOUT, 24,  &timeout);

    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status);
}


TEST_F(ATTest, at_expect_fail_beggining)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_at_expect(2, SYSHAL_UART_ERROR_DEVICE, 3,  &timeout);

    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);

}
TEST_F(ATTest, at_expect_fail_beggining_timeout)
{
    /* Arrange */
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    uint32_t length_read;
    uint32_t timeout;
    uint32_t value;
    uint8_t buffer[MAX_AT_BUFFER_SIZE];
    inject_mock_at_expect(2, SYSHAL_UART_ERROR_TIMEOUT, 3,  &timeout);

    /* Act */
    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status);
}



TEST_F(ATTest, at_expect_simple_numoverflow)
{
    int timeout = 10;
    int temp_timeout = MILLI_TO_MICROSECONDS(timeout);
    uint32_t bytes_received = 1;
    uint32_t length_read;

    uint32_t value;
    char buffer[MAX_AT_BUFFER_SIZE];

    uint8_t read_data[MAX_AT_BUFFER_SIZE] = "Check1: 1121322340000000000 Check2: True and other things...";
    uint32_t read_len = strlen((char*)read_data) + 1;
    read_data[read_len] = '\r';
    uint8_t *ptr_read = read_data;

    uint8_t command_expected[MAX_AT_BUFFER_SIZE] = "Check1: 1121322340000000000 Check2: True ";
    uint32_t expected_len = strlen("Check1: ") + 10 + 1;
    uint32_t command_expected_len =  strlen((char*)command_expected);
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";
    for (int i = 0; i < expected_len; ++i)
    {
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_read, 1, temp_timeout , char_timeout_microsecond, &bytes_received, SYSHAL_UART_NO_ERROR);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_read, 1);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(&bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        temp_timeout = char_timeout_microsecond;
        ptr_read++;
    }

    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);

    EXPECT_EQ(AT_ERROR_BUFFER_OVERFLOW, status);
}


TEST_F(ATTest, at_expect_http_header)
{
    /* Arrange */

    uint32_t length_read;
    uint16_t http_code;

    inject_mock_at_expect_http_header(0, SYSHAL_UART_NO_ERROR);

    /* Act */
    int status = at_expect_http_header(&length_read, &http_code);

    /* Assert */
    EXPECT_EQ(AT_NO_ERROR, status);
    EXPECT_EQ(http_code, 200);
}


TEST_F(ATTest, at_expect_http_header_error_code)
{
    /* Arrange */

    uint32_t length_read;
    uint16_t http_code;

    inject_mock_at_expect_http_header(1, SYSHAL_UART_NO_ERROR);

    /* Act */
    int status = at_expect_http_header(&length_read, &http_code);

    /* Assert */
    EXPECT_EQ(AT_ERROR_HTTP, status);
    EXPECT_EQ(http_code, 400);
}

TEST_F(ATTest, at_expect_http_header_error_timeout)
{
    /* Arrange */

    uint32_t length_read;
    uint16_t http_code;

    inject_mock_at_expect_http_header(2, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = at_expect_http_header(&length_read, &http_code);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status);

}
TEST_F(ATTest, at_expect_http_header_error_device)
{
    /* Arrange */

    uint32_t length_read;
    uint16_t http_code;

    inject_mock_at_expect_http_header(2, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = at_expect_http_header(&length_read, &http_code);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);
}

TEST_F(ATTest, at_expect_http_header_second_error_timeout)
{
    /* Arrange */

    uint32_t length_read;
    uint16_t http_code;

    inject_mock_at_expect_http_header(3, SYSHAL_UART_ERROR_TIMEOUT);

    /* Act */
    int status = at_expect_http_header(&length_read, &http_code);

    /* Assert */
    EXPECT_EQ(AT_ERROR_TIMEOUT, status);

}
TEST_F(ATTest, at_expect_http_header_second_error_device)
{
    /* Arrange */

    uint32_t length_read;
    uint16_t http_code;

    inject_mock_at_expect_http_header(3, SYSHAL_UART_ERROR_DEVICE);

    /* Act */
    int status = at_expect_http_header(&length_read, &http_code);

    /* Assert */
    EXPECT_EQ(AT_ERROR_DEVICE, status);

}





TEST_F(ATTest, DISABLED_at_expect_simple)
{
    int timeout = 10;
    int temp_timeout = MILLI_TO_MICROSECONDS(timeout);
    uint32_t bytes_received = 1;
    uint32_t length_read;

    uint32_t value;
    char buffer[MAX_AT_BUFFER_SIZE];

    uint8_t read_data[MAX_AT_BUFFER_SIZE] = "Check1: 123400 Check2: True and other things...";
    uint32_t read_len = strlen((char*)read_data) + 1;
    read_data[read_len] = '\r';
    uint8_t *ptr_read = read_data;

    uint8_t command_expected[MAX_AT_BUFFER_SIZE] = "Check1: 123400 Check2: True ";
    uint32_t command_expected_len =  strlen((char*)command_expected);
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";


    for (int i = 0; i < command_expected_len; ++i)
    {
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_read, 1, temp_timeout, char_timeout_microsecond, &bytes_received, SYSHAL_UART_NO_ERROR);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_read, 1);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(&bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        temp_timeout = char_timeout_microsecond;
        ptr_read++;
    }

    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, MAX_AT_BUFFER_SIZE);


    EXPECT_EQ(AT_NO_ERROR, status);

    EXPECT_STREQ("True", buffer);

    EXPECT_EQ(123400, value);

}


//CHEKC IMPLEMENTATION STRING
TEST_F(ATTest, DISABLED_at_expect_simple_string_overflow)
{
    int timeout = 10;
    int temp_timeout = MILLI_TO_MICROSECONDS(timeout);
    uint32_t bytes_received = 1;
    uint32_t length_read;

    uint32_t value;
    char buffer[MAX_AT_BUFFER_SIZE];

    uint8_t read_data[MAX_AT_BUFFER_SIZE] = "Check1: 11 Check2: True and other things...";
    uint32_t read_len = strlen((char*)read_data) + 1;
    read_data[read_len] = '\r';
    uint8_t *ptr_read = read_data;

    uint8_t command_expected[MAX_AT_BUFFER_SIZE] = "Check1: 11 Check2: True ";
    uint32_t expected_len = strlen("Check1: 11 Check2: Tru") + 1;
    uint32_t command_expected_len =  strlen((char*)command_expected);
    uint8_t command_formated[MAX_AT_BUFFER_SIZE] = "Check1: %u Check2: %s ";

    for (int i = 0; i < expected_len ; ++i)
    {
        syshal_uart_read_timeout_ExpectAndReturn(uart_device_priv, ptr_read, 1, temp_timeout, char_timeout_microsecond, &bytes_received, SYSHAL_UART_NO_ERROR);
        syshal_uart_read_timeout_ReturnArrayThruPtr_buffer(ptr_read, 1);
        syshal_uart_read_timeout_IgnoreArg_buffer();
        syshal_uart_read_timeout_ReturnThruPtr_bytes_received(&bytes_received);
        syshal_uart_read_timeout_IgnoreArg_bytes_received();
        temp_timeout = char_timeout_microsecond;
        ptr_read++;
    }

    int status = at_expect(command_formated, timeout, &length_read, &value, &buffer, 3);

    EXPECT_EQ(AT_ERROR_BUFFER_OVERFLOW, status);
}

TEST_F(ATTest, at_expect_function)
{
    int timeout = 10;

    uint8_t command_received_un[MAX_AT_BUFFER_SIZE] = "Check1: 123400 Check2: True and other things...";
    uint8_t command_expected[MAX_AT_BUFFER_SIZE] = "Check1: 123400 Check2: True ";

    int status = expect_simple(command_expected, command_received_un, timeout);

    EXPECT_EQ(AT_NO_ERROR, status);
}

TEST_F(ATTest, DISABLED_at_power_on_sequence)
{
    int status;
    uint32_t timeout = 10;
    uint8_t OK_expected[MAX_AT_BUFFER_SIZE] = "OK";
    uint8_t OK_received[MAX_AT_BUFFER_SIZE] = "OK";
    uint8_t command_sent[MAX_AT_BUFFER_SIZE];
    uint8_t command_sent_expected[MAX_AT_BUFFER_SIZE];
    char thing_name[MAX_AT_BUFFER_SIZE] = "purple_cat";

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "USECPRF=0");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "USECPRF=0,0,0");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "USECPRF=0,1,3");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "USECPRF=0,3,\"root-CA.crt\"");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "USECPRF=0,0,0");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "USECPRF=0,5,\"%s.pem\"");
    strcpy((char*)command_sent_expected, "AT+USECPRF=0,5,\"");
    strcat((char*)command_sent_expected, thing_name);
    strcat((char*)command_sent_expected, ".pem\"");
    status =  send_simple_string(command_sent, thing_name, command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "USECPRF=0,6,\"%s.key\"");
    strcpy((char*)command_sent_expected, "AT+USECPRF=0,6,\"");
    strcat((char*)command_sent_expected, thing_name);
    strcat((char*)command_sent_expected, ".key\"");
    status =  send_simple_string(command_sent, thing_name, command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);
}

TEST_F(ATTest, DISABLED_at_power_cell_scan)
{
    int status;
    uint32_t timeout = 10;
    uint32_t value = 243;
    uint8_t OK_expected[MAX_AT_BUFFER_SIZE] = "OK";
    uint8_t OK_received[MAX_AT_BUFFER_SIZE] = "OK";
    uint8_t command_expected[MAX_AT_BUFFER_SIZE];
    uint8_t command_received[MAX_AT_BUFFER_SIZE];
    uint8_t command_sent[MAX_AT_BUFFER_SIZE];
    uint8_t command_sent_expected[MAX_AT_BUFFER_SIZE];
    char thing_name[MAX_AT_BUFFER_SIZE] = "purple_cat";

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "COPS=5");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_expected, "MCC:%u");//LEN = 7 + 1
    strcpy((char*)command_received, "MCC:234, MNC:30, LAC:07d0,");

    status = expect_simple_num(command_expected, &value, command_received, timeout, 8);
    EXPECT_EQ(AT_NO_ERROR, status);


    strcpy((char*)command_sent, "abort");
    status =  send_raw_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "abort");
    status =  send_raw_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);
}

TEST_F(ATTest, DISABLED_at_read_file)
{
    int status;
    uint32_t timeout = 10;
    uint32_t value = 243;
    uint8_t command_expected[MAX_AT_BUFFER_SIZE];
    uint8_t command_received[MAX_AT_BUFFER_SIZE];
    uint8_t command_sent[MAX_AT_BUFFER_SIZE];
    uint8_t command_sent_expected[MAX_AT_BUFFER_SIZE];
    uint8_t OK_expected[MAX_AT_BUFFER_SIZE] = "OK";
    uint8_t OK_received[MAX_AT_BUFFER_SIZE] = "OK";
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTP=0");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTP=0,1,%s");
    strcpy((char*)command_sent_expected, "AT+UHTTP=0,1,");
    strcat((char*)command_sent_expected, domain);
    status =  send_simple_string(command_sent, domain, command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTP=0,6,1,2");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTP=0,5,%u");
    strcpy((char*)command_sent_expected, "AT+UHTTP=0,5,443");
    status =  send_simple_num(command_sent, port, command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTPC=0,1,%s,TEMP.DAT");
    strcpy((char*)command_sent_expected, "AT+UHTTPC=0,1,");
    strcat((char*)command_sent_expected, path);
    strcat((char*)command_sent_expected, ",TEMP.DAT");
    status =  send_simple_string(command_sent, path, command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "URDFILE=\"TEMP.DAT\"");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);
    //DELETE
    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_expected, "+URDFILE: \"TEMP.DAT\",%u,\"");//LEN = 7 + 1
    strcpy((char*)command_received, "+URDFILE: \"TEMP.DAT\",140,\"");
    int length = 140;
    uint8_t buffer[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";

    status = expect_simple_num(command_expected, &value, command_received, timeout, strlen((char*)command_received));
    EXPECT_EQ(AT_NO_ERROR, status);
    EXPECT_EQ(140, value);
    status =  read_buffer_simple(buffer, length, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);
}

TEST_F(ATTest, DISABLED_at_read_file_block)
{
    int status;
    uint32_t timeout = 10;
    uint32_t value = 243;
    uint8_t command_expected[MAX_AT_BUFFER_SIZE];
    uint8_t command_received[MAX_AT_BUFFER_SIZE];
    uint8_t command_sent[MAX_AT_BUFFER_SIZE];
    uint8_t command_sent_expected[MAX_AT_BUFFER_SIZE];
    uint8_t OK_expected[MAX_AT_BUFFER_SIZE] = "OK";
    uint8_t OK_received[MAX_AT_BUFFER_SIZE] = "OK";
    char domain[MAX_AT_BUFFER_SIZE] = "icoteq-test.s3.amazonaws.com";
    uint32_t port = 443;
    char path[MAX_AT_BUFFER_SIZE] = "/hwtest.bin?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1551719853&Signature=6rwzsXEoDkLBEETUr%2BSXLv1p6Bs%3D";

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTP=0");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTP=0,1,%s");
    strcpy((char*)command_sent_expected, "AT+UHTTP=0,1,");
    strcat((char*)command_sent_expected, domain);
    status =  send_simple_string(command_sent, domain, command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTP=0,6,1,2");
    status =  send_simple(command_sent);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTP=0,5,%u");
    strcpy((char*)command_sent_expected, "AT+UHTTP=0,5,443");
    status =  send_simple_num(command_sent, port, command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "UHTTPC=0,1,%s,TEMP.DAT");
    strcpy((char*)command_sent_expected, "AT+UHTTPC=0,1,");
    strcat((char*)command_sent_expected, path);
    strcat((char*)command_sent_expected, ",TEMP.DAT");
    status =  send_simple_string(command_sent, path, command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = expect_simple(OK_expected, OK_received, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    int length = 140;
    uint8_t complete[MAX_AT_BUFFER_SIZE] = "HTTP/1.1 504 Gateway Time-out \rServer: WebProxy/1.0 Pre-Alpha \rDate: Mon, 04 Mar 2019 14:20:36 GMT \rContent-Length: 0 \rConnection: close \r\r\"";
    int read = 0;
    int block_len = 50;

    strcpy((char*)command_sent, "URDBLOCK=\"post.ffs\",0,%u");
    strcpy((char*)command_sent_expected, "AT+URDBLOCK=\"post.ffs\",0,XX");
    status =  send_simple_num(command_sent, block_len, command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);


    strcpy((char*)command_expected, "+URDBLOCK: \"post.ffs\",%u,\"");//LEN = 7 + 1
    strcpy((char*)command_received, "+URDBLOCK: \"post.ffs\",50,\"");


    status = expect_simple_num(command_expected, &value, command_received, timeout, strlen((char*)command_received));
    EXPECT_EQ(AT_NO_ERROR, status);
    EXPECT_EQ(50, value);

    status =  read_buffer_simple((&complete[(read * block_len)]) , block_len, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    read++;
    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "URDBLOCK=\"post.ffs\",50,%u");
    strcpy((char*)command_sent_expected, "AT+URDBLOCK=\"post.ffs\",50,XX");
    status =  send_simple_num(command_sent, block_len , command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);


    strcpy((char*)command_expected, "+URDBLOCK: \"post.ffs\",%u,\"");//LEN = 7 + 1
    strcpy((char*)command_received, "+URDBLOCK: \"post.ffs\",50,\"");


    status = expect_simple_num(command_expected, &value, command_received, timeout, strlen((char*)command_received));
    EXPECT_EQ(AT_NO_ERROR, status);
    EXPECT_EQ(50, value);

    status =  read_buffer_simple((&complete[(read * block_len)]) , block_len, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);

    read++;
    status = flush_simple();
    EXPECT_EQ(AT_NO_ERROR, status);

    strcpy((char*)command_sent, "URDBLOCK=\"post.ffs\",100,%u");
    strcpy((char*)command_sent_expected, "AT+URDBLOCK=\"post.ffs\",100,XX");
    status =  send_simple_num(command_sent, block_len , command_sent_expected);
    EXPECT_EQ(AT_NO_ERROR, status);


    strcpy((char*)command_expected, "+URDBLOCK: \"post.ffs\",%u,\"");//LEN = 7 + 1
    strcpy((char*)command_received, "+URDBLOCK: \"post.ffs\",40,\"");


    status = expect_simple_num(command_expected, &value, command_received, timeout, strlen((char*)command_received));
    EXPECT_EQ(AT_NO_ERROR, status);
    EXPECT_EQ(40, value);

    status =  read_buffer_simple((&complete[(read * block_len)]) , block_len, timeout);
    EXPECT_EQ(AT_NO_ERROR, status);
}

