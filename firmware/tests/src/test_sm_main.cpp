// test_sm_main.cpp - Main statemachine unit tests
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
#include "unity.h"
#include <assert.h>
#include <stdint.h>
#include "Mocksyshal_cellular.h"
#include "Mocksyshal_axl.h"
#include "Mocksyshal_batt.h"
#include "Mocksyshal_ble.h"
#include "Mocksyshal_gpio.h"
#include "Mocksyshal_gps.h"
#include "Mocksyshal_flash.h"
#include "Mocksyshal_uart.h"
#include "Mocksyshal_sat.h"
#include "Mocksyshal_spi.h"
#include "Mocksyshal_switch.h"
#include "Mocksyshal_i2c.h"
#include "Mocksyshal_rtc.h"
#include "Mocksyshal_time.h"
#include "Mocksyshal_pressure.h"
#include "Mocksyshal_pmu.h"
#include "Mocksyshal_usb.h"
#include "Mocksyshal_led.h"
#include "Mocksyshal_device.h"
#include "Mockgps_scheduler.h"
#include "Mocksm_iot.h"
#include "Mockiot.h"
#include "syshal_timer.h"
#include "logging.h"
#include "config_if.h"
#include "fs_priv.h"
#include "fs.h"
#include "crc_32.h"
#include "cmd.h"

#include "sys_config.h"
#include "sm.h"
#include "sm_main.h"
#include "bsp.h"
#include <stdlib.h>
}

#include <gtest/gtest.h>

#include <ctime>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <utility>
#include <queue>
#include <vector>

#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)


#define LED_BLINK_CORRECT_DURATION_MS 200
#define LED_BLINK_FAIL_DURATION_MS 100
#define LED_DURATION_MS 5000

// syshal_usb
static bool syshal_usb_initialised;
static std::deque<std::vector<uint8_t>> syshal_usb_sent_data;
static std::deque<std::vector<uint8_t>> syshal_usb_receive_data;
static bool syshal_usb_plugged_in_state;
static bool syshal_usb_receive_queued;
static uint8_t * syshal_usb_receive_buffer;
static uint32_t syshal_usb_receive_buffer_size;

void syshal_usb_test_setup()
{
    syshal_usb_initialised = false;
    syshal_usb_sent_data.clear();
    syshal_usb_receive_data.clear();
    syshal_usb_plugged_in_state = false;
    syshal_usb_receive_queued = false;
    syshal_usb_receive_buffer = nullptr;
    syshal_usb_receive_buffer_size = 0;
}

int syshal_usb_init_GTest(int cmock_num_calls)
{
    if (syshal_usb_initialised)
        printf("%s() called despite already being init\r\n", __FUNCTION__);
    syshal_usb_initialised = true;
    return SYSHAL_USB_NO_ERROR;
}

int syshal_usb_term_GTest(int cmock_num_calls)
{
    if (!syshal_usb_initialised)
        printf("%s() called despite not being init\r\n", __FUNCTION__);
    syshal_usb_initialised = false;
    return SYSHAL_USB_NO_ERROR;
}

bool syshal_usb_plugged_in_GTest(int cmock_num_calls)
{
    return syshal_usb_plugged_in_state;
}

int syshal_usb_send_GTest(uint8_t * data, uint32_t size, int cmock_num_calls)
{
    EXPECT_TRUE(syshal_usb_initialised);

    syshal_usb_sent_data.push_back(std::vector<uint8_t>(data, data + size));

    // Generate a send complete event
    syshal_usb_event_t event;
    event.id = SYSHAL_USB_EVENT_SEND_COMPLETE;
    event.send.buffer = data;
    event.send.size = size;
    syshal_usb_event_handler(&event);

    return SYSHAL_USB_NO_ERROR;
}

int syshal_usb_receive_GTest(uint8_t * buffer, uint32_t size, int cmock_num_calls)
{
    EXPECT_TRUE(syshal_usb_initialised);

    if (syshal_usb_receive_queued)
        return SYSHAL_USB_ERROR_BUSY;

    if (syshal_usb_receive_data.size())
    {
        EXPECT_GE(size, syshal_usb_receive_data.front().size());
        std::copy(syshal_usb_receive_data.front().begin(), syshal_usb_receive_data.front().end(), buffer);

        // Generate a receive complete event
        syshal_usb_event_t event;
        event.id = SYSHAL_USB_EVENT_RECEIVE_COMPLETE;
        event.receive.buffer = buffer;
        event.receive.size = syshal_usb_receive_data.front().size();

        syshal_usb_receive_data.pop_front();

        syshal_usb_receive_queued = false;
        syshal_usb_event_handler(&event);

        return SYSHAL_USB_NO_ERROR;
    }

    syshal_usb_receive_queued = true;
    syshal_usb_receive_buffer = buffer;
    syshal_usb_receive_buffer_size = size;

    return SYSHAL_USB_NO_ERROR;
}

int syshal_usb_tick_GTest(int cmock_num_calls)
{
    EXPECT_TRUE(syshal_usb_initialised);

    if (syshal_usb_receive_queued)
    {
        if (syshal_usb_receive_data.size())
        {
            EXPECT_GE(syshal_usb_receive_buffer_size, syshal_usb_receive_data.front().size());
            std::copy(syshal_usb_receive_data.front().begin(), syshal_usb_receive_data.front().end(), syshal_usb_receive_buffer);

            // Generate a receive complete event
            syshal_usb_event_t event;
            event.id = SYSHAL_USB_EVENT_RECEIVE_COMPLETE;
            event.receive.buffer = syshal_usb_receive_buffer;
            event.receive.size = syshal_usb_receive_data.front().size();

            syshal_usb_receive_data.pop_front();

            syshal_usb_receive_queued = false;
            syshal_usb_event_handler(&event);
        }
    }

    return SYSHAL_USB_NO_ERROR;
}

void syshal_usb_queue_message(uint8_t * buffer, size_t length)
{
//    printf("%s: ", __FUNCTION__);
//    for (auto i = 0; i < length; ++i)
//        printf("%02X ", buffer[i]);
//    printf("\r\n");
    syshal_usb_receive_data.push_back(std::vector<uint8_t>(buffer, buffer + length));
}

void syshal_usb_queue_message(cmd_t * buffer, size_t length)
{
    syshal_usb_queue_message((uint8_t *) buffer, length);
}

void syshal_usb_fetch_response(uint8_t * message)
{
    ASSERT_GT(syshal_usb_sent_data.size(), 0); // We must at least have one message to receive

    std::copy(syshal_usb_sent_data.front().begin(), syshal_usb_sent_data.front().end(), message);

//    printf("%s: ", __FUNCTION__);
//    for (auto i = 0; i < syshal_usb_sent_data.front().size(); ++i)
//        printf("%02X ", message[i]);
//    printf("\r\n");

    // Remove this buffer from the deque
    syshal_usb_sent_data.pop_front();
}

void syshal_usb_fetch_response(cmd_t * message)
{
    syshal_usb_fetch_response((uint8_t *)message);
}

// syshal_ble
static bool syshal_ble_initialised;
static std::deque<std::vector<uint8_t>> syshal_ble_sent_data;
static std::deque<std::vector<uint8_t>> syshal_ble_receive_data;
static bool syshal_ble_plugged_in_state;
static bool syshal_ble_receive_queued;
static uint8_t * syshal_ble_receive_buffer;
static uint32_t syshal_ble_receive_buffer_size;
static const uint32_t syshal_ble_version = 0xABCD0123;
static syshal_ble_init_t config_ble;

void syshal_ble_test_setup()
{
    syshal_ble_initialised = false;
    syshal_ble_sent_data.clear();
    syshal_ble_receive_data.clear();
    syshal_ble_plugged_in_state = false;
    syshal_ble_receive_queued = false;
    syshal_ble_receive_buffer = nullptr;
    syshal_ble_receive_buffer_size = 0;
}

int syshal_ble_init_GTest(syshal_ble_init_t ble_config, int cmock_num_calls)
{
    if (syshal_ble_initialised)
        printf("%s() called despite already being init\r\n", __FUNCTION__);
    config_ble = ble_config;
    syshal_ble_initialised = true;
    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_term_GTest(int cmock_num_calls)
{
    if (!syshal_ble_initialised)
        printf("%s() called despite not being init\r\n", __FUNCTION__);
    syshal_ble_initialised = false;
    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_send_GTest(uint8_t *buffer, uint32_t size, int cmock_num_calls)
{
    EXPECT_TRUE(syshal_ble_initialised);

    syshal_ble_sent_data.push_back(std::vector<uint8_t>(buffer, buffer + size));

    // Generate a send complete event
    syshal_ble_event_t event;
    event.id = SYSHAL_BLE_EVENT_SEND_COMPLETE;
    event.send_complete.length = size;
    syshal_ble_event_handler(&event);

    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_receive_GTest(uint8_t *buffer, uint32_t size, int cmock_num_calls)
{
    EXPECT_TRUE(syshal_ble_initialised);

    if (syshal_ble_receive_queued)
        return SYSHAL_BLE_ERROR_BUSY;

    if (syshal_ble_receive_data.size())
    {
        EXPECT_GE(size, syshal_ble_receive_data.front().size());
        std::copy(syshal_ble_receive_data.front().begin(), syshal_ble_receive_data.front().end(), buffer);

        // Generate a receive complete event
        syshal_ble_event_t event;
        event.id = SYSHAL_BLE_EVENT_RECEIVE_COMPLETE;
        event.receive_complete.length = syshal_ble_receive_data.front().size();

        syshal_ble_receive_data.pop_front();

        syshal_ble_receive_queued = false;
        syshal_ble_event_handler(&event);

        return SYSHAL_BLE_NO_ERROR;
    }

    syshal_ble_receive_queued = true;
    syshal_ble_receive_buffer = buffer;
    syshal_ble_receive_buffer_size = size;

    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_tick_GTest(int cmock_num_calls)
{
    EXPECT_TRUE(syshal_ble_initialised);

    if (syshal_ble_receive_queued)
    {
        if (syshal_ble_receive_data.size())
        {
            EXPECT_GE(syshal_ble_receive_buffer_size, syshal_ble_receive_data.front().size());
            std::copy(syshal_ble_receive_data.front().begin(), syshal_ble_receive_data.front().end(), syshal_ble_receive_buffer);

            // Generate a receive complete event
            syshal_ble_event_t event;
            event.id = SYSHAL_BLE_EVENT_RECEIVE_COMPLETE;
            event.receive_complete.length = syshal_ble_receive_data.front().size();

            syshal_ble_receive_data.pop_front();

            syshal_ble_receive_queued = false;
            syshal_ble_event_handler(&event);
        }
    }

    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_get_version_GTest(uint32_t * version, int cmock_num_calls)
{
    *version = syshal_ble_version;
    return SYSHAL_BLE_NO_ERROR;
};

// syshal_time

int syshal_time_init_GTest(int cmock_num_calls) {return SYSHAL_TIME_NO_ERROR;}

uint64_t syshal_time_get_ticks_ms_value;
uint32_t syshal_time_get_ticks_ms_GTest(int cmock_num_calls)
{
    return static_cast<uint32_t>(syshal_time_get_ticks_ms_value);
}

void syshal_time_delay_us_GTest(uint32_t us, int cmock_num_calls) {}
void syshal_time_delay_ms_GTest(uint32_t ms, int cmock_num_calls) {}

// syshal_gpio
bool GPIO_pin_state[GPIO_TOTAL_NUMBER];
void (*GPIO_callback_function[GPIO_TOTAL_NUMBER])(const syshal_gpio_event_t*);

int syshal_gpio_init_GTest(uint32_t pin, int cmock_num_calls) {return SYSHAL_GPIO_NO_ERROR;}
bool syshal_gpio_get_input_GTest(uint32_t pin, int cmock_num_calls) {return GPIO_pin_state[pin];}
int syshal_gpio_set_output_low_GTest(uint32_t pin, int cmock_num_calls) {GPIO_pin_state[pin] = 0; return SYSHAL_GPIO_NO_ERROR;}
int syshal_gpio_set_output_high_GTest(uint32_t pin, int cmock_num_calls) {GPIO_pin_state[pin] = 1; return SYSHAL_GPIO_NO_ERROR;}
int syshal_gpio_set_output_toggle_GTest(uint32_t pin, int cmock_num_calls) {GPIO_pin_state[pin] = !GPIO_pin_state[pin]; return SYSHAL_GPIO_NO_ERROR;}
int syshal_gpio_enable_interrupt_GTest(uint32_t pin, void (*callback_function)(const syshal_gpio_event_t*), int cmock_num_calls) {GPIO_callback_function[pin] = callback_function; return SYSHAL_GPIO_NO_ERROR;}

// syshal_rtc
time_t current_date_time = time(0);
uint32_t current_milliseconds = 0;

int syshal_rtc_init_GTest(int cmock_num_calls) {return SYSHAL_RTC_NO_ERROR;}

int syshal_rtc_set_date_and_time_GTest(syshal_rtc_data_and_time_t date_time, int cmock_num_calls)
{
    struct tm * timeinfo = localtime(&current_date_time);

    timeinfo->tm_sec = date_time.seconds; // seconds of minutes from 0 to 61
    timeinfo->tm_min = date_time.minutes; // minutes of hour from 0 to 59
    timeinfo->tm_hour = date_time.hours;  // hours of day from 0 to 24
    timeinfo->tm_mday = date_time.day;    // day of month from 1 to 31
    timeinfo->tm_mon = date_time.month;   // month of year from 0 to 11
    timeinfo->tm_year = date_time.year;   // year since 1900

    current_date_time = mktime(timeinfo);

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_date_and_time_GTest(syshal_rtc_data_and_time_t * date_time, int cmock_num_calls)
{
    struct tm * timeinfo = localtime(&current_date_time);

    date_time->milliseconds = current_milliseconds;
    date_time->seconds = timeinfo->tm_sec; // seconds of minutes from 0 to 61
    date_time->minutes = timeinfo->tm_min; // minutes of hour from 0 to 59
    date_time->hours = timeinfo->tm_hour;  // hours of day from 0 to 24
    date_time->day = timeinfo->tm_mday;    // day of month from 1 to 31
    date_time->month = timeinfo->tm_mon;   // month of year from 0 to 11
    date_time->year = timeinfo->tm_year;   // year since 1900

    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_timestamp_GTest(uint32_t * timestamp, int cmock_num_calls)
{
    *timestamp = (uint32_t) current_date_time;
    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_uptime_GTest(uint32_t * timestamp, int cmock_num_calls)
{
    *timestamp = (uint32_t) syshal_time_get_ticks_ms_value / 1000;
    return SYSHAL_RTC_NO_ERROR;
}

// syshal_batt
uint8_t battery_level;
uint16_t battery_voltage;

int syshal_batt_level_GTest(uint8_t * level, int cmock_num_calls)
{
    *level = battery_level;
    return SYSHAL_BATT_NO_ERROR;
}

int syshal_batt_voltage_GTest(uint16_t * voltage, int cmock_num_calls)
{
    *voltage = battery_voltage;
    return SYSHAL_BATT_NO_ERROR;
}

// syshal_uart
int syshal_uart_init_GTest(uint32_t instance, int cmock_num_calls) {return SYSHAL_UART_NO_ERROR;}

// syshal_spi
int syshal_spi_init_GTest(uint32_t instance, int cmock_num_calls) {return SYSHAL_SPI_NO_ERROR;}

// syshal_i2c
int syshal_i2c_init_GTest(uint32_t instance, int cmock_num_calls) {return SYSHAL_I2C_NO_ERROR;}

// syshal_flash
#define FLASH_SIZE          (FS_PRIV_SECTOR_SIZE * FS_PRIV_MAX_SECTORS)
#define ASCII(x)            ((x) >= 32 && (x) <= 127) ? (x) : '.'

static bool fs_trace;
char flash_ram[FLASH_SIZE];

int syshal_flash_init_GTest(uint32_t drive, uint32_t device, int cmock_num_calls) {return SYSHAL_FLASH_NO_ERROR;}

int syshal_flash_read_GTest(uint32_t device, void * dest, uint32_t address, uint32_t size, int cmock_num_calls)
{
    for (unsigned int i = 0; i < size; i++)
        ((char *)dest)[i] = flash_ram[address + i];

    return 0;
}

int syshal_flash_write_GTest(uint32_t device, const void * src, uint32_t address, uint32_t size, int cmock_num_calls)
{
    if (fs_trace)
        printf("syshal_flash_write(%08x, %u)\n", address, size);
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

int syshal_flash_erase_GTest(uint32_t device, uint32_t address, uint32_t size, int cmock_num_calls)
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

// syshal_gps

// syshal_gps_send_raw callback function
uint8_t gps_write_buffer[2048];
int syshal_gps_send_raw_GTest(uint8_t * data, uint32_t size, int cmock_num_calls)
{
    if (size > sizeof(gps_write_buffer))
        assert(0);

    memcpy(&gps_write_buffer[0], data, size);

    return SYSHAL_GPS_NO_ERROR;
}

// syshal_gps_receive_raw callback function
std::queue<uint8_t> gps_receive_buffer;
int syshal_gps_receive_raw_GTest(uint8_t * data, uint32_t size, int cmock_num_calls)
{
    if (size > gps_receive_buffer.size())
        size = gps_receive_buffer.size();

    for (unsigned int i = 0; i < size; ++i)
    {
        data[i] = gps_receive_buffer.front();
        gps_receive_buffer.pop();
    }

    return size;
}

// syshal_gps

// syshal_celular_send_raw callback function
uint8_t cellular_write_buffer[2048];
int syshal_cellular_send_raw_GTest(uint8_t * data, uint32_t size, int cmock_num_calls)
{
    if (size > sizeof(cellular_write_buffer))
        assert(0);

    memcpy(&cellular_write_buffer[0], data, size);

    return SYSHAL_CELLULAR_NO_ERROR;
}

// syshal_gps_receive_raw callback function
std::queue<uint8_t> cellular_receive_buffer;
int syshal_cellular_receive_raw_GTest(uint8_t * data, uint32_t size, int cmock_num_calls)
{
    if (size > cellular_receive_buffer.size())
        size = cellular_receive_buffer.size();

    for (unsigned int i = 0; i < size; ++i)
    {
        data[i] = cellular_receive_buffer.front();
        cellular_receive_buffer.pop();
    }

    return size;
}

// syshal_switch
syshal_switch_state_t syshal_switch_state[SWITCH_TOTAL_NUMBER];

int syshal_switch_init_GTest(uint32_t instance, void (*callback_function)(const syshal_switch_state_t*), int cmock_num_calls)
{
    if (instance >= SWITCH_TOTAL_NUMBER)
        return SYSHAL_SWITCH_ERROR_INVALID_INSTANCE;
    
    return SYSHAL_SWITCH_NO_ERROR;
}

int syshal_switch_get_GTest(uint32_t instance, syshal_switch_state_t *state, int cmock_num_calls)
{
    if (instance >= SWITCH_TOTAL_NUMBER)
        return SYSHAL_SWITCH_ERROR_INVALID_INSTANCE;
    
    *state = syshal_switch_state[instance];

    return SYSHAL_SWITCH_NO_ERROR;
}

// syshal_pressure
bool syshal_pressure_awake_GTest(int cmock_num_calls) {return false;}

// syshal_axl
bool syshal_axl_awake_GTest(int cmock_num_calls) {return false;}

// syshal_gps
bool syshal_gps_on;
int syshal_gps_wake_up_GTest(int cmock_num_calls) {syshal_gps_on = true; return SYSHAL_GPS_NO_ERROR;}
int syshal_gps_shutdown_GTest(int cmock_num_calls) {syshal_gps_on = false; return SYSHAL_GPS_NO_ERROR;}

// syshal_led
bool led_on;
bool led_blinking;
uint32_t led_colour;
int syshal_led_set_solid_GTest(uint32_t colour, int cmock_num_calls)
{
    led_blinking = false;

    if (SYSHAL_LED_COLOUR_OFF == colour)
        led_on = false;
    else
        led_on = true;

    return SYSHAL_LED_NO_ERROR;
}

int syshal_led_set_blinking_GTest(uint32_t colour, uint32_t time_ms, int cmock_num_calls)
{
    led_blinking = true;
    led_colour = colour;
    led_on = true;

    return SYSHAL_LED_NO_ERROR;
}

bool syshal_led_is_active_GTest(int cmock_num_calls) {return led_on;}

int syshal_led_get_GTest(uint32_t * colour, bool * is_blinking, int cmock_num_calls)
{
    if (!led_on)
        return SYSHAL_LED_ERROR_LED_OFF;

    if (colour)      *colour = led_colour;
    if (is_blinking) *is_blinking = led_blinking;

    return SYSHAL_LED_NO_ERROR;
}

int syshal_led_off_GTest(int cmock_num_calls)
{
    led_on = false;
    led_blinking = false;
    led_colour = SYSHAL_LED_COLOUR_OFF;
    return SYSHAL_LED_NO_ERROR;
}

// syshal_device
static uint32_t app_firmware_version = 1;

int syshal_device_firmware_GTest(uint32_t *version, int cmock_num_calls)
{
    *version = app_firmware_version;
    return SYSHAL_DEVICE_NO_ERROR;
}

class Sm_MainTest : public ::testing::Test
{

    virtual void SetUp()
    {
        Mocksyshal_gpio_Init();
        Mocksyshal_axl_Init();
        Mocksyshal_led_Init();
        Mocksyshal_time_Init();
        Mocksyshal_rtc_Init();
        Mocksyshal_batt_Init();
        Mocksm_iot_Init();
        Mocksyshal_uart_Init();
        Mocksyshal_spi_Init();
        Mocksyshal_i2c_Init();
        Mocksyshal_flash_Init();
        Mocksyshal_gps_Init();
        Mocksyshal_cellular_Init();
        Mocksyshal_switch_Init();
        Mocksyshal_pressure_Init();
        Mocksyshal_pmu_Init();
        Mocksyshal_usb_Init();
        Mocksyshal_ble_Init();
        Mocksyshal_device_Init();
        Mocksyshal_sat_Init();
        Mockiot_Init();
        Mockgps_scheduler_Init();

        srand(time(NULL));

        // syshal_axl
        syshal_axl_awake_StubWithCallback(syshal_axl_awake_GTest);

        // syshal_gpio
        syshal_gpio_init_StubWithCallback(syshal_gpio_init_GTest);
        syshal_gpio_get_input_StubWithCallback(syshal_gpio_get_input_GTest);
        syshal_gpio_set_output_low_StubWithCallback(syshal_gpio_set_output_low_GTest);
        syshal_gpio_set_output_high_StubWithCallback(syshal_gpio_set_output_high_GTest);
        syshal_gpio_set_output_toggle_StubWithCallback(syshal_gpio_set_output_toggle_GTest);
        syshal_gpio_enable_interrupt_StubWithCallback(syshal_gpio_enable_interrupt_GTest);

        // Set all gpio pin states to low
        for (unsigned int i = 0; i < GPIO_TOTAL_NUMBER; ++i)
            GPIO_pin_state[i] = 0;

        // Clear all gpio interrupts
        for (unsigned int i = 0; i < GPIO_TOTAL_NUMBER; ++i)
            GPIO_callback_function[i] = nullptr;

        // syshal_time
        syshal_time_init_StubWithCallback(syshal_time_init_GTest);
        syshal_time_delay_us_StubWithCallback(syshal_time_delay_us_GTest);
        syshal_time_delay_ms_StubWithCallback(syshal_time_delay_ms_GTest);
        syshal_time_get_ticks_ms_StubWithCallback(syshal_time_get_ticks_ms_GTest);
        syshal_time_get_ticks_ms_value = rand() / 2; // Start with a random up time (not a typical use case but will help stress testing)

        // syshal_rtc
        syshal_rtc_init_StubWithCallback(syshal_rtc_init_GTest);
        syshal_rtc_set_date_and_time_StubWithCallback(syshal_rtc_set_date_and_time_GTest);
        syshal_rtc_get_date_and_time_StubWithCallback(syshal_rtc_get_date_and_time_GTest);
        syshal_rtc_get_timestamp_StubWithCallback(syshal_rtc_get_timestamp_GTest);
        syshal_rtc_get_uptime_StubWithCallback(syshal_rtc_get_uptime_GTest);
        syshal_rtc_set_alarm_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);
        syshal_rtc_soft_watchdog_enable_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);
        syshal_rtc_soft_watchdog_refresh_IgnoreAndReturn(SYSHAL_RTC_NO_ERROR);

        // syshal_batt
        syshal_batt_init_IgnoreAndReturn(SYSHAL_BATT_NO_ERROR);
        syshal_batt_level_StubWithCallback(syshal_batt_level_GTest);
        syshal_batt_voltage_StubWithCallback(syshal_batt_voltage_GTest);

        battery_level = 100;
        battery_voltage = 0;

        // sys_config
        unset_all_configuration_tags_RAM();

        // sm_iot
        sm_iot_init_IgnoreAndReturn(SM_IOT_NO_ERROR);
        sm_iot_term_IgnoreAndReturn(SM_IOT_NO_ERROR);
        sm_iot_new_position_IgnoreAndReturn(SM_IOT_NO_ERROR);
        sm_iot_tick_IgnoreAndReturn(SM_IOT_NO_ERROR);

        // syshal_uart
        syshal_uart_init_StubWithCallback(syshal_uart_init_GTest);

        // syshal_led
        led_on = false;
        led_blinking = false;
        led_colour = SYSHAL_LED_COLOUR_OFF;
        syshal_led_init_IgnoreAndReturn(0);
        syshal_led_tick_Ignore();
        syshal_led_set_solid_StubWithCallback(syshal_led_set_solid_GTest);
        syshal_led_set_blinking_StubWithCallback(syshal_led_set_blinking_GTest);
        syshal_led_is_active_StubWithCallback(syshal_led_is_active_GTest);
        syshal_led_is_active_StubWithCallback(syshal_led_is_active_GTest);
        syshal_led_get_StubWithCallback(syshal_led_get_GTest);
        syshal_led_off_StubWithCallback(syshal_led_off_GTest);

        // syshal_spi
        syshal_spi_init_StubWithCallback(syshal_spi_init_GTest);

        // syshal_i2c
        syshal_i2c_init_StubWithCallback(syshal_i2c_init_GTest);

        // syshal_flash
        syshal_flash_init_StubWithCallback(syshal_flash_init_GTest);
        syshal_flash_read_StubWithCallback(syshal_flash_read_GTest);
        syshal_flash_write_StubWithCallback(syshal_flash_write_GTest);
        syshal_flash_erase_StubWithCallback(syshal_flash_erase_GTest);

        // Clear FLASH contents
        for (auto i = 0; i < FLASH_SIZE; ++i)
            flash_ram[i] = 0xFF;

        fs_trace = false; // turn FS trace off

        // syshal_gps
        syshal_gps_on = false;
        syshal_gps_init_IgnoreAndReturn(SYSHAL_GPS_NO_ERROR);
        syshal_gps_tick_IgnoreAndReturn(SYSHAL_GPS_NO_ERROR);
        syshal_gps_wake_up_StubWithCallback(syshal_gps_wake_up_GTest);
        syshal_gps_shutdown_StubWithCallback(syshal_gps_shutdown_GTest);
        syshal_gps_send_raw_StubWithCallback(syshal_gps_send_raw_GTest);
        syshal_gps_receive_raw_StubWithCallback(syshal_gps_receive_raw_GTest);

        // syshal_cellular
        syshal_cellular_init_IgnoreAndReturn(SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_power_off_IgnoreAndReturn(SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_power_on_IgnoreAndReturn(SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_sync_comms_IgnoreAndReturn(SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_send_raw_StubWithCallback(syshal_cellular_send_raw_GTest);
        syshal_cellular_receive_raw_StubWithCallback(syshal_cellular_receive_raw_GTest);

        // syshal_sat
        syshal_sat_init_IgnoreAndReturn(SYSHAL_SAT_NO_ERROR);

        // syshal_switch
        syshal_switch_init_StubWithCallback(syshal_switch_init_GTest);
        syshal_switch_get_StubWithCallback(syshal_switch_get_GTest);

        // syshal_pressure
        syshal_pressure_awake_StubWithCallback(syshal_pressure_awake_GTest);

        // syshal_pmu
        syshal_pmu_init_Ignore();
        syshal_pmu_sleep_Ignore();
        syshal_pmu_get_startup_status_IgnoreAndReturn(0x00000000);

        // syshal_usb
        syshal_usb_test_setup();
        syshal_usb_init_StubWithCallback(syshal_usb_init_GTest);
        syshal_usb_term_StubWithCallback(syshal_usb_term_GTest);
        syshal_usb_plugged_in_StubWithCallback(syshal_usb_plugged_in_GTest);
        syshal_usb_send_StubWithCallback(syshal_usb_send_GTest);
        syshal_usb_receive_StubWithCallback(syshal_usb_receive_GTest);
        syshal_usb_tick_StubWithCallback(syshal_usb_tick_GTest);

        // syshal_ble
        syshal_ble_test_setup();
        syshal_ble_get_version_StubWithCallback(syshal_ble_get_version_GTest);
        syshal_ble_init_StubWithCallback(syshal_ble_init_GTest);
        syshal_ble_term_StubWithCallback(syshal_ble_term_GTest);
        syshal_ble_send_StubWithCallback(syshal_ble_send_GTest);
        syshal_ble_receive_StubWithCallback(syshal_ble_receive_GTest);
        syshal_ble_tick_StubWithCallback(syshal_ble_tick_GTest);

        // syshal_device
        syshal_device_init_IgnoreAndReturn(SYSHAL_DEVICE_NO_ERROR);
        syshal_device_firmware_version_StubWithCallback(syshal_device_firmware_GTest);

        // gps_scheduler
        gps_scheduler_init_IgnoreAndReturn(GPS_SCHEDULER_NO_ERROR);
        gps_scheduler_start_IgnoreAndReturn(GPS_SCHEDULER_NO_ERROR);
        gps_scheduler_stop_IgnoreAndReturn(GPS_SCHEDULER_NO_ERROR);

        // Setup main state machine
        sm_init(&state_handle, sm_main_states);

        sys_config.format_version = SYS_CONFIG_FORMAT_VERSION;
    }

    virtual void TearDown()
    {
        // Reset all syshal timers
        for (timer_handle_t i = 0; i < SYSHAL_TIMER_NUMBER_OF_TIMERS; ++i)
            syshal_timer_term(i);

        config_if_term();

        Mocksyshal_axl_Verify();
        Mocksyshal_axl_Destroy();
        Mocksyshal_gpio_Verify();
        Mocksyshal_gpio_Destroy();
        Mocksyshal_time_Verify();
        Mocksyshal_time_Destroy();
        Mocksyshal_ble_Verify();
        Mocksyshal_ble_Destroy();
        Mocksyshal_rtc_Verify();
        Mocksyshal_rtc_Destroy();
        Mocksyshal_batt_Verify();
        Mocksyshal_batt_Destroy();
        Mocksyshal_uart_Verify();
        Mocksyshal_uart_Destroy();
        Mocksyshal_spi_Verify();
        Mocksyshal_spi_Destroy();
        Mocksm_iot_Verify();
        Mocksm_iot_Destroy();
        Mocksyshal_i2c_Verify();
        Mocksyshal_i2c_Destroy();
        Mocksyshal_flash_Verify();
        Mocksyshal_flash_Destroy();
        Mocksyshal_gps_Verify();
        Mocksyshal_gps_Destroy();
        Mocksyshal_cellular_Verify();
        Mocksyshal_cellular_Destroy();
        Mocksyshal_pressure_Verify();
        Mocksyshal_pressure_Destroy();
        Mocksyshal_pmu_Verify();
        Mocksyshal_pmu_Destroy();
        Mocksyshal_usb_Verify();
        Mocksyshal_usb_Destroy();
        Mocksyshal_led_Verify();
        Mocksyshal_led_Destroy();
        Mocksyshal_device_Verify();
        Mocksyshal_device_Destroy();
        Mocksyshal_sat_Verify();
        Mocksyshal_sat_Destroy();
        Mockiot_Verify();
        Mockiot_Destroy();
        Mockgps_scheduler_Verify();
        Mockgps_scheduler_Destroy();
    }

public:

    sm_handle_t state_handle;

    void BootTagsNotSet()
    {
        sm_set_current_state(&state_handle, SM_MAIN_BOOT);

        sm_tick(&state_handle);
    }

    void BootTagsSetAndLogFileCreated()
    {
        set_all_configuration_tags_RAM();
        SetBatteryLowThreshold(10);

        // Create the log file
        fs_t file_system;
        fs_handle_t file_system_handle;

        EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
        EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));
        EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

        sm_set_current_state(&state_handle, SM_MAIN_BOOT);

        sm_tick(&state_handle);
    }

    void BootTagsSetAndLogFileCreatedUSBConnected()
    {


        set_all_configuration_tags_RAM();
        SetBatteryLowThreshold(10);

        // Create the log file
        fs_t file_system;
        fs_handle_t file_system_handle;

        EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
        EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));
        EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));
        sm_set_current_state(&state_handle, SM_MAIN_BOOT);
        sm_tick(&state_handle);



        sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING);

        config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_USB};
        config_if_init(backend);
        USBConnectionEvent();
        SetVUSB(true);
        EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));
        EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());

    }
    void BootTagsNotSetUSBConnected()
    {
        BootTagsNotSet();

        sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING);

        config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_USB};
        config_if_init(backend);
        USBConnectionEvent();

        SetVUSB(true);
        sm_tick(&state_handle);

        EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));
        EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());
    }

    void CreateEmptyLogfile()
    {
        fs_t file_system;
        fs_handle_t file_system_handle;

        EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
        EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));
        EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));
    }

    void boot_operational_gps_test(void)
    {
        uint32_t gps_write_length = 256;
        uint32_t acquisition_interval = 165;
        uint32_t maximum_acquisition = 15;
        uint32_t no_fix_timeout = 0;
        uint32_t test_fix_hold_time = 5;
        auto gps_schedule_interval_on_max_backoff = 1024;

        BootTagsSetAndLogFileCreatedUSBConnected();
        sm_tick(&state_handle);
        // Generate gps test message
        cmd_t req;
        CMD_SET_HDR((&req), CMD_TEST_REQ);
        req.cmd_test_req.test_device_flag = CMD_TEST_REQ_GPS_FLAG;
        syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_test_req_t));

        sm_tick(&state_handle); // Process the message
        // Check the response
        cmd_t resp;
        syshal_usb_fetch_response(&resp);
        EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
        EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
        EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

        sm_tick(&state_handle); // Process the message
        USBDisconnectEvent();
        SetVUSB(false);
        IncrementMilliseconds(LED_DURATION_MS + 10);
        CreateEmptyLogfile();

        // Set GPS trigger mode
        sys_config.gps_trigger_mode.hdr.set = true;
        sys_config.gps_trigger_mode.contents.mode = SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED_BITMASK;

        // Set GPS acquisition interval
        sys_config.gps_scheduled_acquisition_interval.hdr.set = true;
        sys_config.gps_scheduled_acquisition_interval.contents.seconds = acquisition_interval;

        // Set GPS acquisition time
        sys_config.gps_maximum_acquisition_time.hdr.set = true;
        sys_config.gps_maximum_acquisition_time.contents.seconds = maximum_acquisition;

        // Set GPS no fix timeout
        sys_config.gps_scheduled_acquisition_no_fix_timeout.hdr.set = true;
        sys_config.gps_scheduled_acquisition_no_fix_timeout.contents.seconds = no_fix_timeout;

        // Set GPS first fix hold
        sys_config.gps_test_fix_hold_time.hdr.set = true;
        sys_config.gps_test_fix_hold_time.contents.seconds = test_fix_hold_time;

        // Enable GPS logging
        sys_config.gps_log_position_enable.hdr.set = true;
        sys_config.gps_log_position_enable.contents.enable = true;

        // Enable Global logging
        sys_config.logging_enable.hdr.set = true;
        sys_config.logging_enable.contents.enable = true;
        sm_tick(&state_handle);
        EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
        IncrementMilliseconds(LED_DURATION_MS + 10);
        sm_tick(&state_handle);
    }

    void boot_operational_cellular_test(void)
    {
        BootTagsSetAndLogFileCreatedUSBConnected();
        sm_tick(&state_handle);
        // Generate gps test message
        cmd_t req;
        CMD_SET_HDR((&req), CMD_TEST_REQ);
        req.cmd_test_req.test_device_flag = CMD_TEST_REQ_CELLULAR_FLAG;
        syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_test_req_t));

        sm_tick(&state_handle); // Process the message
        // Check the response
        cmd_t resp;
        syshal_usb_fetch_response(&resp);
        EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
        EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
        EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

        sm_tick(&state_handle); // Process the message
        USBDisconnectEvent();
        SetVUSB(false);
        IncrementMilliseconds(LED_DURATION_MS + 10);
        CreateEmptyLogfile();

        //cellular test set
        sys_config.iot_cellular_settings.contents.enable = true;

        sm_tick(&state_handle);
        EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
        IncrementMilliseconds(LED_DURATION_MS + 10);
        sm_tick(&state_handle);
    }

    void boot_operational_gps_cellular_test(void)
    {
        uint32_t gps_write_length = 256;
        uint32_t acquisition_interval = 165;
        uint32_t maximum_acquisition = 15;
        uint32_t no_fix_timeout = 0;
        uint32_t test_fix_hold_time = 5;
        auto gps_schedule_interval_on_max_backoff = 1024;
        BootTagsSetAndLogFileCreatedUSBConnected();
        sm_tick(&state_handle);

        // Generate gps + cellular test message
        cmd_t req;
        CMD_SET_HDR((&req), CMD_TEST_REQ);
        req.cmd_test_req.test_device_flag = (CMD_TEST_REQ_GPS_FLAG | CMD_TEST_REQ_CELLULAR_FLAG);
        syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_test_req_t));

        sm_tick(&state_handle); // Process the message

        // Check the response
        cmd_t resp;
        syshal_usb_fetch_response(&resp);
        EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
        EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
        EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

        sm_tick(&state_handle); // Process the message

        USBDisconnectEvent();
        SetVUSB(false);
        IncrementMilliseconds(LED_DURATION_MS + 10);
        CreateEmptyLogfile();

        // Set GPS trigger mode
        sys_config.gps_trigger_mode.hdr.set = true;
        sys_config.gps_trigger_mode.contents.mode = SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED_BITMASK;

        // Set GPS acquisition interval
        sys_config.gps_scheduled_acquisition_interval.hdr.set = true;
        sys_config.gps_scheduled_acquisition_interval.contents.seconds = acquisition_interval;

        // Set GPS acquisition time
        sys_config.gps_maximum_acquisition_time.hdr.set = true;
        sys_config.gps_maximum_acquisition_time.contents.seconds = maximum_acquisition;

        // Set GPS no fix timeout
        sys_config.gps_scheduled_acquisition_no_fix_timeout.hdr.set = true;
        sys_config.gps_scheduled_acquisition_no_fix_timeout.contents.seconds = no_fix_timeout;

        // Set GPS first fix hold
        sys_config.gps_test_fix_hold_time.hdr.set = true;
        sys_config.gps_test_fix_hold_time.contents.seconds = test_fix_hold_time;

        // Enable GPS logging
        sys_config.gps_log_position_enable.hdr.set = true;
        sys_config.gps_log_position_enable.contents.enable = true;

        // Enable Global logging
        sys_config.logging_enable.hdr.set = true;
        sys_config.logging_enable.contents.enable = true;

        //cellular test set
        sys_config.iot_cellular_settings.contents.enable = true;

        sm_tick(&state_handle);

        EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
        IncrementMilliseconds(LED_DURATION_MS + 10);
        sm_tick(&state_handle);
    }

    void boot_operational_satellite_test(void)
    {
        BootTagsSetAndLogFileCreatedUSBConnected();
        sm_tick(&state_handle);

        // Generate cellular test message
        cmd_t req;
        CMD_SET_HDR((&req), CMD_TEST_REQ);
        req.cmd_test_req.test_device_flag = CMD_TEST_REQ_SATELLITE_FLAG;
        syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_test_req_t));

        sm_tick(&state_handle); // Process the message
        // Check the response
        cmd_t resp;
        syshal_usb_fetch_response(&resp);
        EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
        EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
        EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

        sm_tick(&state_handle); // Process the message
        USBDisconnectEvent();
        SetVUSB(false);
        IncrementMilliseconds(LED_DURATION_MS + 10);
        CreateEmptyLogfile();

        // cellular test set
        sys_config.iot_sat_settings.hdr.set = true;
        sys_config.iot_sat_settings.contents.enable = true;

        sys_config.iot_sat_artic_settings.hdr.set = true;

        sm_tick(&state_handle);
        EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
        IncrementMilliseconds(LED_DURATION_MS + 10);
        sm_tick(&state_handle);
    }

    void SetVUSB(bool state)
    {
        syshal_usb_plugged_in_state = state;
    }

    void SetBatteryPercentage(uint8_t level)
    {
        battery_level = level;
    }

    void SetBatteryVoltage(uint16_t voltage)
    {
        battery_voltage = voltage;
    }

    void SetBatteryLowThreshold(uint8_t level)
    {
        sys_config.battery_low_threshold.hdr.set = true;
        sys_config.battery_low_threshold.contents.threshold = level;
    }

    void SetGPIOPin(uint32_t pin, bool state)
    {
        GPIO_pin_state[pin] = state;

        syshal_gpio_event_t event =
        {
            .id = SYSHAL_GPIO_EVENT_TOGGLE,
            .pin_number = pin
        };

        // If there's any interrupt on this pin, call it
        if (GPIO_callback_function[pin])
            GPIO_callback_function[pin](&event);
    }

    static void IncrementMilliseconds(uint32_t milliseconds)
    {
        struct tm * timeinfo = localtime(&current_date_time);

        timeinfo->tm_sec += (current_milliseconds + milliseconds) / 1000;

        current_date_time = mktime(timeinfo);

        current_milliseconds = (current_milliseconds + milliseconds) % 1000;

        syshal_time_get_ticks_ms_value += milliseconds;
    }

    static void IncrementSeconds(uint32_t seconds)
    {
        IncrementMilliseconds(seconds * 1000);
    }

    // Message handling //

    static void BLEConnectionEvent(void)
    {
        syshal_ble_event_t event;
        event.id = SYSHAL_BLE_EVENT_CONNECTED;
        syshal_ble_event_handler(&event);
    }

    static void BLEDisconnectEvent(void)
    {
        syshal_ble_event_t event;
        event.id = SYSHAL_BLE_EVENT_DISCONNECTED;
        syshal_ble_event_handler(&event);
    }

    static void BLETriggeredOnReedSwitchEnable(void)
    {
        sys_config.tag_bluetooth_trigger_control.hdr.set = true;
        sys_config.tag_bluetooth_trigger_control.contents.flags |= SYS_CONFIG_TAG_BLUETOOTH_TRIGGER_CONTROL_REED_SWITCH;
    }

    static void BLETriggeredOnSchedule(uint32_t interval, uint32_t duration, uint32_t timeout)
    {
        sys_config.tag_bluetooth_trigger_control.hdr.set = true;
        sys_config.tag_bluetooth_trigger_control.contents.flags |= SYS_CONFIG_TAG_BLUETOOTH_TRIGGER_CONTROL_SCHEDULED;

        sys_config.tag_bluetooth_scheduled_interval.hdr.set = true;
        sys_config.tag_bluetooth_scheduled_interval.contents.seconds = interval;

        sys_config.tag_bluetooth_scheduled_duration.hdr.set = true;
        sys_config.tag_bluetooth_scheduled_duration.contents.seconds = duration;

        sys_config.tag_bluetooth_connection_inactivity_timeout.hdr.set = true;
        sys_config.tag_bluetooth_connection_inactivity_timeout.contents.seconds = timeout;
    }

    static void USBConnectionEvent(void)
    {
        syshal_usb_event_t event;
        event.id = SYSHAL_USB_EVENT_CONNECTED;
        syshal_usb_event_handler(&event);
    }

    static void USBDisconnectEvent(void)
    {
        syshal_usb_event_t event;
        event.id = SYSHAL_USB_EVENT_DISCONNECTED;
        syshal_usb_event_handler(&event);
    }

    static void set_all_configuration_tags_RAM()
    {
        uint8_t config_if_dummy_data[4096] = {0};

        uint16_t tag, last_index = 0;
        uint32_t length = 0;

        while (!sys_config_iterate(&tag, &last_index))
        {
            int ret;
            do
            {
                ret = sys_config_set(tag, &config_if_dummy_data, length++);
            }
            while (SYS_CONFIG_ERROR_WRONG_SIZE == ret);

            length = 0;

            if (SYS_CONFIG_ERROR_NO_MORE_TAGS == ret)
                break;
        }
    }

    static void unset_all_configuration_tags_RAM()
    {
        // Set all configuration tag data to random values
        // This is to ensure the tests properly ignore them
        for (auto i = 0; i < sizeof(sys_config); ++i)
            ((uint8_t *)&sys_config)[i] = rand();

        sys_config.tag_bluetooth_trigger_control.contents.flags = 0;

        // Unset all the tags
        uint16_t last_index = 0;
        uint16_t tag;

        while (!sys_config_iterate(&tag, &last_index))
        {
            sys_config_unset(tag);
        }
    }

};

//////////////////////////////////////////////////////////////////
/////////////////////////// Boot State ///////////////////////////
//////////////////////////////////////////////////////////////////

TEST_F(Sm_MainTest, BootNoTags)
{
    BootTagsNotSet();

    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, BootNoTagsVUSB)
{
    SetVUSB(true);

    BootTagsNotSet();

    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, BootOperationalState)
{
    BootTagsSetAndLogFileCreated();

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
}

//////////////////////////////////////////////////////////////////
/////////////////////// Battery Low State ////////////////////////
//////////////////////////////////////////////////////////////////

TEST_F(Sm_MainTest, BatteryLowToBatteryChargingNoVUSB)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_LEVEL_LOW);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_LEVEL_LOW, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, BatteryLowToBatteryChargingVUSB)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_LEVEL_LOW);

    SetVUSB(true);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

//////////////////////////////////////////////////////////////////
///////////////////// Battery Charging State /////////////////////
//////////////////////////////////////////////////////////////////

TEST_F(Sm_MainTest, BatteryChargingNoVUSBBatteryLowNoThreshold)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_CHARGING);

    SetBatteryPercentage(0);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, BatteryChargingNoVUSBOperationalState)
{
    BootTagsSetAndLogFileCreated();

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_CHARGING);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, BatteryChargingNoVUSBBatteryLow)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_CHARGING);

    SetBatteryPercentage(0);

    SetBatteryLowThreshold(10);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_LEVEL_LOW, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, BatteryChargingUSBTimeout)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_CHARGING);

    SetVUSB(true);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));

    // Jump forward 5 seconds
    IncrementSeconds(5);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));

    // Jump forward 100 seconds
    IncrementSeconds(100);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, BatteryChargingBLERunningButNotConnected)
{
    // This should realise the BLE stack is running but is not connected
    // So it should terminate it and start the USB stack
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_CHARGING);

    config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_BLE};
    config_if_init(backend);
    SetVUSB(true);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, BatteryChargingUSBConnected)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_CHARGING);

    SetVUSB(true);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));

    USBConnectionEvent();

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, DISABLED_BatteryChargingBLEReedSwitchWithUSBTimeout)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_CHARGING);

    SetVUSB(true);
    BLETriggeredOnReedSwitchEnable(); // Ensure the BLE should be on by setting the Reed switch activation
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));

    // Jump forward 5 seconds
    IncrementSeconds(5);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));

    // Jump forward 100 seconds
    IncrementSeconds(100);

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    // USB timed out but BLE should be running
    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, BatteryChargingBLEScheduledWithUSBTimeout)
{
    const uint32_t interval = 15;
    const uint32_t duration = 10;

    SetVUSB(true);
    BootTagsNotSet();
    BLETriggeredOnSchedule(15, 5, 0);

    sm_set_current_state(&state_handle, SM_MAIN_BATTERY_CHARGING);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));

    // Jump forward 5 seconds
    IncrementSeconds(5);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));

    // Jump forward 10 seconds
    IncrementSeconds(10);

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    // USB timed out but BLE should be running
    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());
    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

//////////////////////////////////////////////////////////////////
////////////////////// Log File Full State ///////////////////////
//////////////////////////////////////////////////////////////////

TEST_F(Sm_MainTest, LogFileFullToBatteryCharging)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_LOG_FILE_FULL);

    EXPECT_EQ(SM_MAIN_LOG_FILE_FULL, sm_get_current_state(&state_handle));

    SetVUSB(true);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, LogFileFullToLowBattery)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_LOG_FILE_FULL);

    EXPECT_EQ(SM_MAIN_LOG_FILE_FULL, sm_get_current_state(&state_handle));

    SetBatteryPercentage(0);
    SetBatteryLowThreshold(10);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_LEVEL_LOW, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, LogFileFullBLEConnection)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_LOG_FILE_FULL);

    EXPECT_EQ(SM_MAIN_LOG_FILE_FULL, sm_get_current_state(&state_handle));

    config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_BLE};
    config_if_init(backend);
    BLETriggeredOnReedSwitchEnable(); // Ensure the BLE should be on by setting the Reed switch activation
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch
    BLEConnectionEvent();

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, DISABLED_LogFileFullBLEReedSwitchToggle)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_LOG_FILE_FULL);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());

    // Enable reed switch activation of BLE
    BLETriggeredOnReedSwitchEnable();

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch

    IncrementSeconds(5); // Progress time to allow for switch debouncing

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());
    SetGPIOPin(GPIO_REED_SW, 1); // Release the reed switch

    IncrementSeconds(5); // Progress time to allow for switch debouncing

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_LOG_FILE_FULL, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, LogFileFullBLEScheduled)
{
    const uint32_t interval = 15;
    const uint32_t duration = 5;

    BootTagsNotSet();
    BLETriggeredOnSchedule(15, 5, 0);

    sm_set_current_state(&state_handle, SM_MAIN_LOG_FILE_FULL);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());

    IncrementSeconds(interval);

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());

    IncrementSeconds(duration);

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_LOG_FILE_FULL, sm_get_current_state(&state_handle));
}

//////////////////////////////////////////////////////////////////
//////////////////// Provisioning Needed State ///////////////////
//////////////////////////////////////////////////////////////////

TEST_F(Sm_MainTest, ProvisioningNeededToBatteryCharging)
{

    BootTagsNotSet();
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING_NEEDED);
    SetVUSB(true);
    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, ProvisioningNeededToBatteryLow)
{
    BootTagsNotSet();
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING_NEEDED);

    SetBatteryPercentage(0);
    SetBatteryLowThreshold(10);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_LEVEL_LOW, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, ProvisioningNeededToProvisioning)
{
    BootTagsNotSet();
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING_NEEDED);
    config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_BLE};
    config_if_init(backend);
    BLETriggeredOnReedSwitchEnable(); // Ensure the BLE should be on by setting the Reed switch activation
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch
    BLEConnectionEvent();

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, DISABLED_ProvisioningNeededBLEReedSwitchToggle)
{
    BootTagsNotSet();
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING_NEEDED);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());

    // Enable reed switch activation of BLE
    BLETriggeredOnReedSwitchEnable();

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch

    IncrementSeconds(5); // Progress time to allow for switch debouncing

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());
    SetGPIOPin(GPIO_REED_SW, 1); // Release the reed switch

    IncrementSeconds(5); // Progress time to allow for switch debouncing

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, ProvisioningNeededBLEReedSwitchToggleButDisabled)
{
    BootTagsNotSet();
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING_NEEDED);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    SetGPIOPin(GPIO_REED_SW, 1); // Release the reed switch

    sm_tick(&state_handle);
    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, ProvisioningNeededBLEScheduled)
{
    const uint32_t interval = 15;
    const uint32_t duration = 5;

    BootTagsNotSet();
    BLETriggeredOnSchedule(interval, duration, 0);
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING_NEEDED);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());

    IncrementSeconds(interval);

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());

    IncrementSeconds(duration);

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, ProvisioningNeededBLEScheduledConnectionInactivityTimeout)
{
    const uint32_t interval = 30;
    const uint32_t duration = 15;
    const uint32_t timeout = 5;
    BootTagsNotSet();
    BLETriggeredOnSchedule(interval, duration, timeout);
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING_NEEDED);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    IncrementSeconds(interval);

    BLEConnectionEvent();

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());

    IncrementSeconds(1);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());
    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));

    IncrementSeconds(timeout);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
}

//////////////////////////////////////////////////////////////////
/////////////////////// Provisioning State ///////////////////////
//////////////////////////////////////////////////////////////////

TEST_F(Sm_MainTest, ProvisioningToProvisioningNeeded)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING);

    config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_BLE};
    config_if_init(backend);
    BLETriggeredOnReedSwitchEnable(); // Ensure the BLE should be on by setting the Reed switch activation
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch
    BLEConnectionEvent();

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));

    BLEDisconnectEvent();
    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, ProvisioningToCharging)
{
    BootTagsNotSet();

    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING);

    config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_USB};
    config_if_init(backend);
    SetVUSB(true);
    USBConnectionEvent();

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));

    USBDisconnectEvent();

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, ProvisioningToOperationalState)
{
    BootTagsSetAndLogFileCreated();

    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING);

    sm_tick(&state_handle);
    IncrementSeconds(6);
    sm_tick(&state_handle);
    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, ProvisioningToOperationalState_Fail_APN_Cellular)
{
    auto min_gps_updates = 100;
    BootTagsSetAndLogFileCreated();

    // Enable the IoT layer
    sys_config.iot_general_settings.hdr.set = true;
    sys_config.iot_general_settings.contents.enable = true;
    sys_config.iot_cellular_settings.hdr.set = true;
    sys_config.iot_cellular_settings.contents.enable = true;
    sys_config.iot_cellular_settings.contents.min_updates = min_gps_updates;

    sys_config_unset(SYS_CONFIG_TAG_IOT_CELLULAR_APN);
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING);

    sm_tick(&state_handle);
    IncrementSeconds(6);
    sm_tick(&state_handle);
    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, DISABLED_ProvisioningBLEReedSwitchDisable)
{
    BootTagsNotSet();
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING);

    // Enable reed switch activation of BLE
    BLETriggeredOnReedSwitchEnable();
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch

    config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_BLE};
    config_if_init(backend);
    BLEConnectionEvent();

    IncrementSeconds(5); // Progress time to allow for switch debouncing

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());

    SetGPIOPin(GPIO_REED_SW, 1); // Release the reed switch

    IncrementSeconds(5); // Progress time to allow for switch debouncing

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
}

//////////////////////////////////////////////////////////////////
/////////////////////// Operational State ////////////////////////
//////////////////////////////////////////////////////////////////

TEST_F(Sm_MainTest, OperationalToBatteryLow)
{
    BootTagsSetAndLogFileCreated();

    sm_set_current_state(&state_handle, SM_MAIN_OPERATIONAL);

    SetBatteryPercentage(0);
    SetBatteryLowThreshold(10);

    // TODO: handle these ignored functions properly
    syshal_axl_term_IgnoreAndReturn(0);
    syshal_pressure_term_IgnoreAndReturn(0);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_LEVEL_LOW, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, OperationalToBatteryCharging)
{
    BootTagsSetAndLogFileCreated();

    sm_set_current_state(&state_handle, SM_MAIN_OPERATIONAL);

    SetVUSB(true);

    // TODO: handle these ignored functions properly
    syshal_axl_term_IgnoreAndReturn(0);
    syshal_pressure_term_IgnoreAndReturn(0);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_BATTERY_CHARGING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, OperationalToProvisioning)
{
    BootTagsSetAndLogFileCreated();

    sm_set_current_state(&state_handle, SM_MAIN_OPERATIONAL);

    config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_BLE};
    config_if_init(backend);
    BLETriggeredOnReedSwitchEnable(); // Ensure the BLE should be on by setting the Reed switch activation
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch
    BLEConnectionEvent();

    // TODO: handle these ignored functions properly
    syshal_axl_term_IgnoreAndReturn(0);
    syshal_pressure_term_IgnoreAndReturn(0);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, OperationalToLogFileFull)
{
    int32_t pressureReading = rand();

    BootTagsSetAndLogFileCreated();

    // Fill the log file completely
    fs_t file_system;
    fs_handle_t file_system_handle;
    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_WRITEONLY, NULL));

    int writeResult;
    uint8_t dummyData[2048];
    uint32_t bytes_written;

    do
    {
        writeResult = fs_write(file_system_handle, dummyData, sizeof(dummyData), &bytes_written);
    }
    while (writeResult == FS_NO_ERROR);

    EXPECT_EQ(FS_ERROR_FILESYSTEM_FULL, writeResult);
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    sm_set_current_state(&state_handle, SM_MAIN_OPERATIONAL);

    // Enable general logging
    sys_config.logging_enable.hdr.set = true;
    sys_config.logging_enable.contents.enable = true;

    // Enable the pressure sensor
    sys_config.pressure_sensor_log_enable.hdr.set = true;
    sys_config.pressure_sensor_log_enable.contents.enable = true;

    // TODO: handle these ignored functions properly
    syshal_pressure_init_ExpectAndReturn(SYSHAL_PRESSURE_NO_ERROR);
    syshal_pressure_wake_ExpectAndReturn(SYSHAL_PRESSURE_NO_ERROR);
    syshal_pressure_tick_ExpectAndReturn(SYSHAL_PRESSURE_NO_ERROR);

    sm_tick(&state_handle);

    syshal_pressure_callback(pressureReading);

    syshal_pressure_tick_ExpectAndReturn(SYSHAL_PRESSURE_NO_ERROR);

    syshal_axl_term_IgnoreAndReturn(SYSHAL_AXL_NO_ERROR);
    syshal_pressure_term_IgnoreAndReturn(SYSHAL_PRESSURE_NO_ERROR);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_LOG_FILE_FULL, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, DISABLED_OperationalBLEReedSwitchToggle)
{
    BootTagsSetAndLogFileCreated();

    sm_set_current_state(&state_handle, SM_MAIN_OPERATIONAL);

    // TODO: handle these ignored functions properly
    syshal_axl_term_IgnoreAndReturn(0);
    syshal_pressure_term_IgnoreAndReturn(0);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());

    // Enable reed switch activation of BLE
    BLETriggeredOnReedSwitchEnable();

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    SetGPIOPin(GPIO_REED_SW, 0); // Trigger the reed switch

    IncrementSeconds(5); // Progress time to allow for switch debouncing

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());
    SetGPIOPin(GPIO_REED_SW, 1); // Release the reed switch

    IncrementSeconds(5); // Progress time to allow for switch debouncing

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, OperationalPressureLogging)
{
    int32_t pressureReading = rand();

    BootTagsSetAndLogFileCreated();

    sm_set_current_state(&state_handle, SM_MAIN_OPERATIONAL);

    // Enable general logging
    sys_config.logging_enable.hdr.set = true;
    sys_config.logging_enable.contents.enable = true;

    // Enable the pressure sensor
    sys_config.pressure_sensor_log_enable.hdr.set = true;
    sys_config.pressure_sensor_log_enable.contents.enable = true;

    // TODO: handle these ignored functions properly
    syshal_pressure_init_ExpectAndReturn(SYSHAL_PRESSURE_NO_ERROR);
    syshal_pressure_wake_ExpectAndReturn(SYSHAL_PRESSURE_NO_ERROR);
    syshal_pressure_tick_ExpectAndReturn(SYSHAL_PRESSURE_NO_ERROR);

    sm_tick(&state_handle);

    syshal_pressure_callback(pressureReading);

    syshal_pressure_tick_ExpectAndReturn(SYSHAL_PRESSURE_NO_ERROR);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));

    EXPECT_EQ(FS_NO_ERROR, fs_flush(sm_main_file_handle)); // Flush any content that hasn't been written yet

    fs_t file_system;
    fs_handle_t file_system_handle;
    uint32_t bytes_read;
    logging_pressure_t pressureLog;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_READONLY, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_read(file_system_handle, (uint8_t *) &pressureLog, sizeof(logging_pressure_t), &bytes_read));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(sizeof(logging_pressure_t), bytes_read);
    EXPECT_EQ(LOGGING_PRESSURE, pressureLog.h.id);
    EXPECT_EQ(pressureReading, pressureLog.pressure);
}

TEST_F(Sm_MainTest, OperationalAXLLogging)
{
    syshal_axl_data_t axlReading;
    axlReading.x = rand();
    axlReading.y = rand();
    axlReading.z = rand();

    BootTagsSetAndLogFileCreated();

    sm_set_current_state(&state_handle, SM_MAIN_OPERATIONAL);

    // Enable general logging
    sys_config.logging_enable.hdr.set = true;
    sys_config.logging_enable.contents.enable = true;

    // Enable the axl sensor
    sys_config.axl_log_enable.hdr.set = true;
    sys_config.axl_log_enable.contents.enable = true;

    // TODO: handle these ignored functions properly
    syshal_axl_init_ExpectAndReturn(SYSHAL_AXL_NO_ERROR);
    syshal_axl_wake_ExpectAndReturn(SYSHAL_AXL_NO_ERROR);
    syshal_axl_tick_ExpectAndReturn(SYSHAL_AXL_NO_ERROR);

    sm_tick(&state_handle);

    syshal_axl_callback(axlReading);

    syshal_axl_tick_ExpectAndReturn(SYSHAL_AXL_NO_ERROR);

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));

    EXPECT_EQ(FS_NO_ERROR, fs_flush(sm_main_file_handle)); // Flush any content that hasn't been written yet

    fs_t file_system;
    fs_handle_t file_system_handle;
    uint32_t bytes_read;
    logging_axl_xyz_t axlLog;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_READONLY, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_read(file_system_handle, (uint8_t *) &axlLog, sizeof(logging_axl_xyz_t), &bytes_read));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(sizeof(logging_axl_xyz_t), bytes_read);
    EXPECT_EQ(LOGGING_AXL_XYZ, axlLog.h.id);
    EXPECT_EQ(axlReading.x, axlLog.x);
    EXPECT_EQ(axlReading.y, axlLog.y);
    EXPECT_EQ(axlReading.z, axlLog.z);
}

TEST_F(Sm_MainTest, OperationalBatteryLogging)
{
    int32_t batteryLevel = rand() % 100;

    BootTagsSetAndLogFileCreated();

    sm_set_current_state(&state_handle, SM_MAIN_OPERATIONAL);

    // Enable general logging
    sys_config.logging_enable.hdr.set = true;
    sys_config.logging_enable.contents.enable = true;

    // Enable battery level logging
    sys_config.battery_log_enable.hdr.set = true;
    sys_config.battery_log_enable.contents.enable = true;

    // Disable battery low threshold
    sys_config.battery_low_threshold.hdr.set = false;

    SetBatteryPercentage(batteryLevel);

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));

    EXPECT_EQ(FS_NO_ERROR, fs_flush(sm_main_file_handle)); // Flush any content that hasn't been written yet

    fs_t file_system;
    fs_handle_t file_system_handle;
    uint32_t bytes_read;
    logging_battery_t batteryCharge;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_READONLY, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_read(file_system_handle, (uint8_t *) &batteryCharge, sizeof(batteryCharge), &bytes_read));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(sizeof(logging_battery_t), bytes_read);
    EXPECT_EQ(LOGGING_BATTERY, batteryCharge.h.id);
    EXPECT_EQ(batteryLevel, batteryCharge.charge);
}

TEST_F(Sm_MainTest, DISABLED_OperationalBLEScheduled)
{
    const uint32_t interval = 15;
    const uint32_t duration = 5;

    BootTagsSetAndLogFileCreated();
    BLETriggeredOnSchedule(15, 5, 0);

    sm_set_current_state(&state_handle, SM_MAIN_OPERATIONAL);

    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());

    IncrementSeconds(interval);

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_BLE, config_if_current());

    IncrementSeconds(duration);

    sm_tick(&state_handle);
    sm_tick(&state_handle);

    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, DISABLED_OperationalStateGPSScheduled)
{
    uint32_t acquisition_interval = 165;
    uint32_t maximum_acquisition = 15;
    uint32_t no_fix_timeout = 0;


    sm_set_current_state(&state_handle, SM_MAIN_BOOT);

    set_all_configuration_tags_RAM();
    CreateEmptyLogfile();

    // Set GPS trigger mode
    sys_config.gps_trigger_mode.hdr.set = true;
    sys_config.gps_trigger_mode.contents.mode = SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED_BITMASK;

    // Set GPS acquisition interval
    sys_config.gps_scheduled_acquisition_interval.hdr.set = true;
    sys_config.gps_scheduled_acquisition_interval.contents.seconds = acquisition_interval;

    // Set GPS acquisition time
    sys_config.gps_maximum_acquisition_time.hdr.set = true;
    sys_config.gps_maximum_acquisition_time.contents.seconds = maximum_acquisition;

    // Set GPS no fix timeout
    sys_config.gps_scheduled_acquisition_no_fix_timeout.hdr.set = true;
    sys_config.gps_scheduled_acquisition_no_fix_timeout.contents.seconds = no_fix_timeout;

    // Disable GPS first fix hold
    sys_config.gps_test_fix_hold_time.hdr.set = false;

    // Enable GPS logging
    sys_config.gps_log_position_enable.hdr.set = true;
    sys_config.gps_log_position_enable.contents.enable = true;

    // Enable Global logging
    sys_config.logging_enable.hdr.set = true;
    sys_config.logging_enable.contents.enable = true;

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
    EXPECT_FALSE(syshal_gps_on);

    for (unsigned int i = 0; i < acquisition_interval + 1; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_TRUE(syshal_gps_on);

    for (unsigned int i = 0; i < maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_gps_on);

    for (unsigned int i = 0; i < acquisition_interval - maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_TRUE(syshal_gps_on);

    for (unsigned int i = 0; i < maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_gps_on);
}

TEST_F(Sm_MainTest, DISABLED_OperationalStateGPSScheduledIoTMaxBackoff)
{
    uint32_t acquisition_interval = 165;
    uint32_t maximum_acquisition = 15;
    uint32_t no_fix_timeout = 0;
    auto gps_schedule_interval_on_max_backoff = 1024;
    sm_iot_event_t sm_iot_event;


    sm_set_current_state(&state_handle, SM_MAIN_BOOT);

    set_all_configuration_tags_RAM();
    CreateEmptyLogfile();

    // Set GPS trigger mode
    sys_config.gps_trigger_mode.hdr.set = true;
    sys_config.gps_trigger_mode.contents.mode = SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED_BITMASK;

    // Set GPS acquisition interval
    sys_config.gps_scheduled_acquisition_interval.hdr.set = true;
    sys_config.gps_scheduled_acquisition_interval.contents.seconds = acquisition_interval;

    // Set GPS acquisition time
    sys_config.gps_maximum_acquisition_time.hdr.set = true;
    sys_config.gps_maximum_acquisition_time.contents.seconds = maximum_acquisition;

    // Set GPS no fix timeout
    sys_config.gps_scheduled_acquisition_no_fix_timeout.hdr.set = true;
    sys_config.gps_scheduled_acquisition_no_fix_timeout.contents.seconds = no_fix_timeout;

    // Disable GPS first fix hold
    sys_config.gps_test_fix_hold_time.hdr.set = false;

    // Enable GPS logging
    sys_config.gps_log_position_enable.hdr.set = true;
    sys_config.gps_log_position_enable.contents.enable = true;

    // Enable Global logging
    sys_config.logging_enable.hdr.set = true;
    sys_config.logging_enable.contents.enable = true;

    // Enable the IoT layer
    sys_config.iot_general_settings.hdr.set = true;
    sys_config.iot_general_settings.contents.enable = true;
    sys_config.iot_cellular_settings.hdr.set = true;
    sys_config.iot_cellular_settings.contents.enable = true;
    sys_config.iot_cellular_settings.contents.min_updates = 10;
    sys_config.iot_cellular_settings.contents.gps_schedule_interval_on_max_backoff = 1024;

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
    EXPECT_FALSE(syshal_gps_on);

    for (unsigned int i = 0; i < acquisition_interval + 1; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_TRUE(syshal_gps_on);

    for (unsigned int i = 0; i < maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_gps_on);

    for (unsigned int i = 0; i < acquisition_interval - maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_TRUE(syshal_gps_on);

    for (unsigned int i = 0; i < maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_gps_on);

    // Trigger a max backoff event
    // We now expect the GPS to be using the max backoff interval
    sm_iot_event.id = SM_IOT_CELLULAR_MAX_BACKOFF_REACHED;
    sm_iot_callback(&sm_iot_event);

    for (unsigned int i = 0; i < acquisition_interval + 1; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_gps_on);

    for (unsigned int i = 0; i < gps_schedule_interval_on_max_backoff - acquisition_interval; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_TRUE(syshal_gps_on);

    for (unsigned int i = 0; i < maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_gps_on);

    // Trigger an IoT successful event
    // We now expect the GPS to be using the original interval
    sm_iot_event.id = SM_IOT_CELLULAR_SEND_DEVICE_STATUS;
    sm_iot_event.iot_report_error.iot_error_code = SM_IOT_NO_ERROR;
    sm_iot_callback(&sm_iot_event);

    for (unsigned int i = 0; i < acquisition_interval + 1; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_TRUE(syshal_gps_on);

    for (unsigned int i = 0; i < maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_gps_on);

    for (unsigned int i = 0; i < acquisition_interval - maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_TRUE(syshal_gps_on);

    for (unsigned int i = 0; i < maximum_acquisition; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_gps_on);
}

TEST_F(Sm_MainTest, DISABLED_OperationalStateGPSScheduledAlwaysOn)
{
    uint32_t acquisition_interval = 0;
    uint32_t maximum_acquisition = 0;
    uint32_t no_fix_timeout = 0;

    sm_set_current_state(&state_handle, SM_MAIN_BOOT);

    set_all_configuration_tags_RAM();
    CreateEmptyLogfile();

    // Set GPS trigger mode
    sys_config.gps_trigger_mode.hdr.set = true;
    sys_config.gps_trigger_mode.contents.mode = SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED_BITMASK;

    // Set GPS acquisition interval
    sys_config.gps_scheduled_acquisition_interval.hdr.set = true;
    sys_config.gps_scheduled_acquisition_interval.contents.seconds = acquisition_interval;

    // Set GPS acquisition time
    sys_config.gps_maximum_acquisition_time.hdr.set = true;
    sys_config.gps_maximum_acquisition_time.contents.seconds = maximum_acquisition;

    // Set GPS no fix timeout
    sys_config.gps_scheduled_acquisition_no_fix_timeout.hdr.set = true;
    sys_config.gps_scheduled_acquisition_no_fix_timeout.contents.seconds = no_fix_timeout;

    // Disable GPS first fix hold
    sys_config.gps_test_fix_hold_time.hdr.set = false;

    // Enable GPS logging
    sys_config.gps_log_position_enable.hdr.set = true;
    sys_config.gps_log_position_enable.contents.enable = true;

    // Enable Global logging
    sys_config.logging_enable.hdr.set = true;
    sys_config.logging_enable.contents.enable = true;

    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));

    for (unsigned int i = 0; i < 100; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
        ASSERT_TRUE(syshal_gps_on);
    }
}

//////////////////////////////////////////////////////////////////
//////////////////////// Message Handling ////////////////////////
//////////////////////////////////////////////////////////////////

TEST_F(Sm_MainTest, StatusRequestUSB)
{
    BootTagsNotSet();
    static const int imsi_length_sara = 15;
    static const int imsi_length = 16;
    sm_set_current_state(&state_handle, SM_MAIN_PROVISIONING);

    uint8_t imsi[imsi_length];
    memset(imsi, 0, imsi_length);
    for (int i = 0; i < imsi_length_sara; ++i)
    {
        imsi[i] = i;
    }

    uint8_t gps_is_present = 1;
    uint8_t celluar_is_present = 1;
    uint8_t satellite_is_present = 0;
    uint16_t error_code;

    syshal_gps_is_present_ExpectAndReturn(gps_is_present);
    syshal_cellular_is_present_ExpectAndReturn(&error_code,celluar_is_present);
    syshal_cellular_is_present_IgnoreArg_error_code();
    syshal_cellular_check_sim_ExpectAndReturn(imsi,&error_code, SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_check_sim_ReturnArrayThruPtr_imsi(imsi, imsi_length_sara);
    syshal_cellular_check_sim_IgnoreArg_imsi();
    syshal_cellular_check_sim_IgnoreArg_error_code();
    config_if_backend_t backend = {.id = CONFIG_IF_BACKEND_USB};
    config_if_init(backend);
    USBConnectionEvent();

    SetVUSB(true);
    sm_tick(&state_handle);

    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));
    EXPECT_EQ(CONFIG_IF_BACKEND_USB, config_if_current());

    // Generate status request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_STATUS_REQ);
    syshal_usb_queue_message(&req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_STATUS_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_status_resp.error_code);

    EXPECT_EQ(gps_is_present, resp.cmd_status_resp.gps_module_detected );
    EXPECT_EQ(celluar_is_present, resp.cmd_status_resp.cellular_module_detected);
    EXPECT_EQ(1, resp.cmd_status_resp.sim_card_present);
    EXPECT_TRUE( 0 == std::memcmp( imsi, resp.cmd_status_resp.sim_card_imsi, sizeof( resp.cmd_status_resp.sim_card_imsi ) ) );
    EXPECT_EQ(satellite_is_present, resp.cmd_status_resp.satellite_module_detected);
}

TEST_F(Sm_MainTest, USBResetRequestApp)
{
    BootTagsNotSetUSBConnected();

    // Generate reset request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_RESET_REQ);
    req.cmd_reset_req.reset_type = RESET_REQ_APP;
    syshal_usb_queue_message(&req, CMD_SIZE(cmd_reset_req_t));

    syshal_pmu_reset_Expect();

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBResetRequestFlashErase)
{
    BootTagsNotSetUSBConnected();

    // Generate reset request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_RESET_REQ);
    req.cmd_reset_req.reset_type = RESET_REQ_FLASH_ERASE_ALL;
    syshal_usb_queue_message(&req, CMD_SIZE(cmd_reset_req_t));

    CreateEmptyLogfile(); // Create file in the file system

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    fs_t file_system;
    fs_handle_t file_system_handle;
    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));

    // Check the FLASH has been erased
    for (unsigned int id = 0; id <= 255; ++id)
        EXPECT_EQ(FS_ERROR_FILE_NOT_FOUND, fs_open(file_system, &file_system_handle, id, FS_MODE_READONLY, NULL));
}

TEST_F(Sm_MainTest, USBResetRequestInvalid)
{
    BootTagsNotSetUSBConnected();

    // Generate reset request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_RESET_REQ);
    req.cmd_reset_req.reset_type = 0xAB; // Invalid/unknown type
    syshal_usb_queue_message(&req, CMD_SIZE(cmd_reset_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_INVALID_PARAMETER, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBCfgWriteOne)
{
    BootTagsNotSetUSBConnected();

    // Generate cfg write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_WRITE_REQ);
    req.cmd_cfg_write_req.length = SYS_CONFIG_TAG_ID_SIZE + SYS_CONFIG_TAG_DATA_SIZE(sys_config_logging_group_sensor_readings_enable_t);
    syshal_usb_queue_message(&req, CMD_SIZE(cmd_cfg_write_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Generate cfg tag data packet
    uint8_t tag_data_packet[3];
    tag_data_packet[0] = uint16_t(SYS_CONFIG_TAG_LOGGING_GROUP_SENSOR_READINGS_ENABLE) & 0x00FF;
    tag_data_packet[1] = (uint16_t(SYS_CONFIG_TAG_LOGGING_GROUP_SENSOR_READINGS_ENABLE) & 0xFF00) >> 8;
    tag_data_packet[2] = true; // Enable

    syshal_usb_queue_message(tag_data_packet, sizeof(tag_data_packet));

    sm_tick(&state_handle); // Process the message

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_CFG_WRITE_CNF, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_cfg_write_cnf.error_code);

    // Check the tag was correctly set
    EXPECT_TRUE(sys_config.logging_group_sensor_readings_enable.contents.enable);
}

TEST_F(Sm_MainTest, USBCfgWriteOneInvalidTag)
{
    BootTagsNotSetUSBConnected();

    // Generate cfg write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_WRITE_REQ);
    req.cmd_cfg_write_req.length = SYS_CONFIG_TAG_ID_SIZE + SYS_CONFIG_TAG_DATA_SIZE(sys_config_logging_group_sensor_readings_enable_t);
    syshal_usb_queue_message(&req, CMD_SIZE(cmd_cfg_write_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Generate cfg tag data packet
    uint8_t tag_data_packet[3];
    uint16_t invalid_tag_ID = 0xABAB;
    tag_data_packet[0] = invalid_tag_ID & 0x00FF;
    tag_data_packet[1] = (invalid_tag_ID & 0xFF00) >> 8;
    tag_data_packet[2] = true; // Enable

    syshal_usb_queue_message(tag_data_packet, sizeof(tag_data_packet));

    sm_tick(&state_handle); // Process the message
    sm_tick(&state_handle); // Return the error

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_CFG_WRITE_CNF, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_INVALID_CONFIG_TAG, resp.cmd_cfg_write_cnf.error_code);
}

TEST_F(Sm_MainTest, USBCfgReadOne)
{
    // Set a tag up for reading later
    sys_config.logging_group_sensor_readings_enable.contents.enable = true;
    sys_config.logging_group_sensor_readings_enable.hdr.set = true;

    BootTagsNotSetUSBConnected();

    // Generate cfg read request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_READ_REQ);
    req.cmd_cfg_read_req.configuration_tag = SYS_CONFIG_TAG_LOGGING_GROUP_SENSOR_READINGS_ENABLE;
    syshal_usb_queue_message(&req, CMD_SIZE(cmd_cfg_read_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_CFG_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_cfg_read_resp.error_code);

    sm_tick(&state_handle); // Send the data packet

    syshal_usb_fetch_response(&resp);
    uint8_t * message = (uint8_t *) &resp;

    uint16_t tag = 0;
    tag |= (uint16_t) message[0] & 0x00FF;
    tag |= (uint16_t) (message[1] << 8) & 0xFF00;
    EXPECT_EQ(SYS_CONFIG_TAG_LOGGING_GROUP_SENSOR_READINGS_ENABLE, tag);

    // Check the tag was correctly read
    EXPECT_TRUE(message[2]);
}

TEST_F(Sm_MainTest, USBCfgReadInvalidTag)
{
    BootTagsNotSetUSBConnected();

    // Generate cfg read request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_READ_REQ);
    req.cmd_cfg_read_req.configuration_tag = 0xABAB;
    syshal_usb_queue_message(&req, CMD_SIZE(cmd_cfg_read_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_CFG_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_INVALID_CONFIG_TAG, resp.cmd_cfg_read_resp.error_code);
}

TEST_F(Sm_MainTest, USBCfgSaveSuccess)
{
    BootTagsNotSetUSBConnected();

    // Generate cfg save request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_SAVE_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Check the configuration file in FLASH matches the configuration in RAM
    fs_t file_system;
    fs_handle_t file_system_handle;
    uint32_t bytes_read;
    uint8_t flash_config_data[sizeof(sys_config)];

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_READONLY, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_read(file_system_handle, &flash_config_data[0], sizeof(sys_config), &bytes_read));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));
    EXPECT_EQ(sizeof(sys_config), bytes_read);

    // Check RAM and FLASH contents match
    bool RAM_FLASH_mismatch = false;
    uint8_t * sys_config_itr = (uint8_t *) &sys_config;
    for (unsigned int i = 0; i < bytes_read; ++i)
    {
        if (flash_config_data[i] != sys_config_itr[i])
        {
            RAM_FLASH_mismatch = true;
            break;
        }
    }

    EXPECT_FALSE(RAM_FLASH_mismatch);
}

TEST_F(Sm_MainTest, USBCfgRestoreNoFile)
{
    BootTagsNotSetUSBConnected();

    // Generate cfg save request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_RESTORE_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_FILE_NOT_FOUND, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBCfgRestoreSuccess)
{
    BootTagsNotSetUSBConnected();

    // Generate the configuration file in FLASH
    fs_t file_system;
    fs_handle_t file_system_handle;
    uint32_t bytes_written;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &sys_config, sizeof(sys_config), &bytes_written));
    uint32_t calculated_crc32 = crc32(0, &sys_config, sizeof(sys_config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Generate cfg save request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_RESTORE_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBCfgProtectSuccess)
{
    BootTagsNotSetUSBConnected();

    // Generate the configuration file in FLASH
    fs_t file_system;
    fs_handle_t file_system_handle;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Generate cfg save request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_PROTECT_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBCfgProtectNoFile)
{
    BootTagsNotSetUSBConnected();

    // Generate cfg save request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_PROTECT_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_FILE_NOT_FOUND, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBCfgUnprotectSuccess)
{
    BootTagsNotSetUSBConnected();

    // Generate the configuration file in FLASH
    fs_t file_system;
    fs_handle_t file_system_handle;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Generate cfg save request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_UNPROTECT_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBCfgUnprotectNoFile)
{
    BootTagsNotSetUSBConnected();

    // Generate cfg save request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_UNPROTECT_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_FILE_NOT_FOUND, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBCfgEraseAll)
{
    BootTagsNotSetUSBConnected();

    set_all_configuration_tags_RAM(); // Set all the configuration tags

    // Generate cfg erase request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_ERASE_REQ);
    req.cmd_cfg_erase_req.configuration_tag = CFG_ERASE_REQ_ERASE_ALL;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_cfg_erase_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Check all the configuration tags have been unset
    bool all_tags_unset = true;
    uint16_t tag, last_index = 0;
    while (!sys_config_iterate(&tag, &last_index))
    {
        void * src;
        int ret = sys_config_get(tag, &src);

        if (SYS_CONFIG_ERROR_TAG_NOT_SET != ret &&
            SYS_CONFIG_TAG_RTC_CURRENT_DATE_AND_TIME != tag &&
            SYS_CONFIG_TAG_LOGGING_FILE_SIZE != tag &&
            SYS_CONFIG_TAG_LOGGING_FILE_TYPE != tag &&
            SYS_CONFIG_TAG_LOGGING_START_END_SYNC_ENABLE != tag)
        {
            all_tags_unset = false;
            break;
        }
    }

    EXPECT_TRUE(all_tags_unset);
}

TEST_F(Sm_MainTest, USBCfgEraseOne)
{
    BootTagsNotSetUSBConnected();

    set_all_configuration_tags_RAM(); // Set all the configuration tags

    // Generate log erase request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CFG_ERASE_REQ);
    req.cmd_cfg_erase_req.configuration_tag = SYS_CONFIG_TAG_LOGGING_GROUP_SENSOR_READINGS_ENABLE;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_cfg_erase_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Check the configuration tag has been unset
    void * src;
    int ret = sys_config_get(SYS_CONFIG_TAG_LOGGING_GROUP_SENSOR_READINGS_ENABLE, &src);

    bool tag_unset = false;
    if (SYS_CONFIG_ERROR_TAG_NOT_SET == ret)
        tag_unset = true;

    EXPECT_TRUE(tag_unset);
}

TEST_F(Sm_MainTest, USBLogEraseSuccess)
{
    BootTagsNotSetUSBConnected();

    // Generate the log file in FLASH
    fs_t file_system;
    fs_handle_t file_system_handle;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Generate log erase request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_LOG_ERASE_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBLogEraseNoFile)
{
    BootTagsNotSetUSBConnected();

    // Generate log erase request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_LOG_ERASE_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_FILE_NOT_FOUND, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBLogCreateFill)
{
    BootTagsNotSetUSBConnected();

    // Generate log create request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_LOG_CREATE_REQ);
    req.cmd_log_create_req.mode = CMD_LOG_CREATE_REQ_MODE_FILL;
    req.cmd_log_create_req.sync_enable = false;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_log_create_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Check the log file has been created and is of the right mode
    fs_t file_system;
    fs_stat_t file_stats;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_stat(file_system, FILE_ID_LOG, &file_stats));
    EXPECT_FALSE(file_stats.is_circular);
}

TEST_F(Sm_MainTest, USBLogCreateCircular)
{
    BootTagsNotSetUSBConnected();

    // Generate log create request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_LOG_CREATE_REQ);
    req.cmd_log_create_req.mode = CMD_LOG_CREATE_REQ_MODE_CIRCULAR;
    req.cmd_log_create_req.sync_enable = false;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_log_create_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Check the log file has been created and is of the right mode
    fs_t file_system;
    fs_stat_t file_stats;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_stat(file_system, FILE_ID_LOG, &file_stats));
    EXPECT_TRUE(file_stats.is_circular);
}

TEST_F(Sm_MainTest, USBLogCreateAlreadyExists)
{
    BootTagsNotSetUSBConnected();

    // Create the log file
    fs_t file_system;
    fs_handle_t file_system_handle;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Generate log create request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_LOG_CREATE_REQ);
    req.cmd_log_create_req.mode = CMD_LOG_CREATE_REQ_MODE_CIRCULAR;
    req.cmd_log_create_req.sync_enable = false;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_log_create_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_FILE_ALREADY_EXISTS, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBLogReadNoFile)
{
    BootTagsNotSetUSBConnected();

    // Generate log read request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_LOG_READ_REQ);
    req.cmd_log_read_req.start_offset = 0;
    req.cmd_log_read_req.length = 256;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_log_read_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_LOG_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_FILE_NOT_FOUND, resp.cmd_log_read_resp.error_code);
}

TEST_F(Sm_MainTest, USBLogReadSuccess)
{
    const uint32_t log_size = 256;
    uint8_t testData[log_size];
    uint32_t bytes_written = 0;

    BootTagsNotSetUSBConnected();

    // Generate test data
    for (auto i = 0; i < log_size; ++i)
        testData[i] = rand();

    // Create the log file
    fs_t file_system;
    fs_handle_t file_system_handle;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, testData, log_size, &bytes_written)); // Load test data into the log file
    EXPECT_EQ(log_size, bytes_written);
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Generate log read request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_LOG_READ_REQ);
    req.cmd_log_read_req.start_offset = 0;
    req.cmd_log_read_req.length = log_size;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_log_read_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_LOG_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_log_read_resp.error_code);
    EXPECT_EQ(log_size, resp.cmd_log_read_resp.length);

    sm_tick(&state_handle); // Process the message

    uint8_t log_data[log_size];
    syshal_usb_fetch_response(log_data);

    bool log_read_mismatch = false;
    for (auto i = 0; i < log_size; ++i)
    {
        if (log_data[i] != testData[i])
        {
            log_read_mismatch = true;
            break;
        }
    }

    EXPECT_FALSE(log_read_mismatch);
}

TEST_F(Sm_MainTest, USBLogReadAll)
{
    const uint32_t log_size = 256;
    uint8_t testData[log_size];
    uint32_t bytes_written = 0;

    BootTagsNotSetUSBConnected();

    // Generate test data
    for (auto i = 0; i < log_size; ++i)
        testData[i] = rand();

    // Create the log file
    fs_t file_system;
    fs_handle_t file_system_handle;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, testData, log_size, &bytes_written)); // Load test data into the log file
    EXPECT_EQ(log_size, bytes_written);
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Generate log read request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_LOG_READ_REQ);
    req.cmd_log_read_req.start_offset = 0; // Both being zero means read all
    req.cmd_log_read_req.length = 0;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_log_read_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_LOG_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_log_read_resp.error_code);
    EXPECT_EQ(log_size, resp.cmd_log_read_resp.length);

    sm_tick(&state_handle); // Process the message

    uint8_t log_data[log_size];
    syshal_usb_fetch_response(log_data);

    bool log_read_mismatch = false;
    for (auto i = 0; i < log_size; ++i)
    {
        if (log_data[i] != testData[i])
        {
            log_read_mismatch = true;
            break;
        }
    }

    EXPECT_FALSE(log_read_mismatch);
}

TEST_F(Sm_MainTest, USBLogReadOffset)
{
    const uint32_t log_size = 256;
    const uint32_t log_read_offset = 128;
    const uint32_t log_read_size = log_size - log_read_offset;
    uint8_t testData[log_size];
    uint32_t bytes_written = 0;

    BootTagsNotSetUSBConnected();

    // Generate test data
    for (auto i = 0; i < log_size; ++i)
        testData[i] = rand();

    // Create the log file
    fs_t file_system;
    fs_handle_t file_system_handle;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, testData, log_size, &bytes_written)); // Load test data into the log file
    EXPECT_EQ(log_size, bytes_written);
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Generate log read request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_LOG_READ_REQ);
    req.cmd_log_read_req.start_offset = log_read_offset; // Both being zero means read all
    req.cmd_log_read_req.length = log_read_size;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_log_read_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_LOG_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_log_read_resp.error_code);
    EXPECT_EQ(log_read_size, resp.cmd_log_read_resp.length);

    sm_tick(&state_handle); // Process the message

    uint8_t log_data[log_size];
    syshal_usb_fetch_response(log_data);

    bool log_read_mismatch = false;
    for (auto i = 0; i < log_read_size; ++i)
    {
        if (log_data[i] != testData[i + log_read_offset])
        {
            log_read_mismatch = true;
            break;
        }
    }

    EXPECT_FALSE(log_read_mismatch);
}

TEST_F(Sm_MainTest, USBBatteryStatus)
{
    BootTagsNotSetUSBConnected();

    // Generate battery status request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_BATTERY_STATUS_REQ);
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE_HDR);

    uint8_t chargePercentage = rand() % 100;
    uint16_t batteryVoltage = rand();

    SetBatteryPercentage(chargePercentage);
    SetBatteryVoltage(batteryVoltage);

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_BATTERY_STATUS_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_battery_status_resp.error_code);
    EXPECT_EQ(true, resp.cmd_battery_status_resp.charging_indicator);
    EXPECT_EQ(chargePercentage, resp.cmd_battery_status_resp.charge_level);
    EXPECT_EQ(batteryVoltage, resp.cmd_battery_status_resp.millivolts);
}

TEST_F(Sm_MainTest, USBGpsConfig)
{
    BootTagsNotSetUSBConnected();

    // Generate gps config request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_GPS_CONFIG_REQ);
    req.cmd_gps_config_req.enable = true;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_gps_config_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_battery_status_resp.error_code);
}

TEST_F(Sm_MainTest, USBGpsWriteBridgingOff)
{
    BootTagsNotSetUSBConnected();

    // Generate GPS write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_GPS_WRITE_REQ);
    req.cmd_gps_write_req.length = 100;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_gps_write_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_BRIDGING_DISABLED, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBGpsWriteSuccess)
{
    uint32_t gps_write_length = 256;
    BootTagsNotSetUSBConnected();

    // Generate gps config message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_GPS_CONFIG_REQ);
    req.cmd_gps_config_req.enable = true;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_gps_config_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    sm_tick(&state_handle); // Process the message

    // Generate GPS write request message
    CMD_SET_HDR((&req), CMD_GPS_WRITE_REQ);
    req.cmd_gps_write_req.length = gps_write_length;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_gps_write_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Generate GPS write payload
    uint8_t gps_data_packet[gps_write_length];
    for (unsigned int i = 0; i < sizeof(gps_data_packet); ++i)
        gps_data_packet[i] = i;

    syshal_usb_queue_message(gps_data_packet, sizeof(gps_data_packet));

    sm_tick(&state_handle); // Process the message

    // Check message wrote is as expected
    bool gps_write_mismatch = false;
    for (unsigned int i = 0; i < gps_write_length; ++i)
    {
        if (gps_write_buffer[i] != gps_data_packet[i])
        {
            gps_write_mismatch = true;
            break;
        }
    }

    EXPECT_FALSE(gps_write_mismatch);
}

TEST_F(Sm_MainTest, USBGpsReadBridgingOff)
{
    BootTagsNotSetUSBConnected();

    // Generate GPS read request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_GPS_READ_REQ);
    req.cmd_gps_read_req.length = 100;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_gps_read_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GPS_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_BRIDGING_DISABLED, resp.cmd_gps_read_resp.error_code);
}

TEST_F(Sm_MainTest, USBGpsReadSuccess)
{
    uint32_t gps_read_length = 256;

    BootTagsNotSetUSBConnected();

    // Generate gps config message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_GPS_CONFIG_REQ);
    req.cmd_gps_config_req.enable = true;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_gps_config_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    sm_tick(&state_handle); // Process the message

    // Generate GPS read request message
    CMD_SET_HDR((&req), CMD_GPS_READ_REQ);
    req.cmd_gps_read_req.length = gps_read_length;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_gps_read_req_t));

    syshal_gps_available_raw_ExpectAndReturn(gps_read_length);

    sm_tick(&state_handle); // Process the message

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GPS_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Load the GPS SPI buffer with test data
    for (unsigned int i = 0; i < gps_read_length; ++i)
        gps_receive_buffer.push(i);

    sm_tick(&state_handle);

    uint8_t received_data[gps_read_length];
    syshal_usb_fetch_response(received_data);

    // Look for mismatch between data received on SPI and data transmitted on config_if
    bool SPI_and_config_if_mismatch = false;
    for (unsigned int i = 0; i < gps_read_length; ++i)
    {
        if (received_data[i] != i)
        {
            SPI_and_config_if_mismatch = true;
            break;
        }
    }

    EXPECT_FALSE(SPI_and_config_if_mismatch);
}

TEST_F(Sm_MainTest, USBFwWriteWrongImageType)
{
    BootTagsNotSetUSBConnected();

    // FILE_ID_APP_FIRM_IMAGE    (1)  // STM32 application image
    // FS_FILE_ID_BLE_APP_IMAGE  (2)  // BLE application image
    // FS_FILE_ID_BLE_SOFT_IMAGE (3)  // BLE soft-device image

    // Generate FW write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_FW_SEND_IMAGE_REQ);
    req.cmd_fw_send_image_req.image_type = 0xAA; // Invalid image type
    req.cmd_fw_send_image_req.length = 100;
    req.cmd_fw_send_image_req.CRC32 = 0;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_fw_send_image_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_INVALID_FW_IMAGE_TYPE, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBFwWriteInvalidCRC32)
{
    const uint32_t fw_size = 100;
    uint8_t fw_image[fw_size];

    BootTagsNotSetUSBConnected();

    // Generate the FW image to be sent
    for (uint32_t i = 0; i < fw_size; ++i)
        fw_image[i] = rand();

    // Generate FW write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_FW_SEND_IMAGE_REQ);
    req.cmd_fw_send_image_req.image_type = FW_SEND_IMAGE_REQ_ARTIC;
    req.cmd_fw_send_image_req.length = fw_size;
    req.cmd_fw_send_image_req.CRC32 = 0;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_fw_send_image_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Send the firmware image
    syshal_usb_queue_message(fw_image, sizeof(fw_image));

    sm_tick(&state_handle); // Process the image

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_FW_SEND_IMAGE_COMPLETE_CNF, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_IMAGE_CRC_MISMATCH, resp.cmd_fw_send_image_complete_cnf.error_code);
}

TEST_F(Sm_MainTest, USBFwWriteSingleArtic)
{
    const uint32_t fw_size = 100;
    uint8_t fw_image[fw_size];
    uint32_t fw_crc32 = 0;

    uint8_t fw_type = FW_SEND_IMAGE_REQ_ARTIC;

    BootTagsNotSetUSBConnected();

    // Generate the FW image to be sent
    for (uint32_t i = 0; i < fw_size; ++i)
        fw_image[i] = rand();

    // Calculate the crc
    fw_crc32 = crc32(fw_crc32, fw_image, fw_size);

    // Generate FW write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_FW_SEND_IMAGE_REQ);
    req.cmd_fw_send_image_req.image_type = fw_type;
    req.cmd_fw_send_image_req.length = fw_size;
    req.cmd_fw_send_image_req.CRC32 = fw_crc32;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_fw_send_image_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Send the firmware image
    syshal_usb_queue_message(fw_image, fw_size);

    sm_tick(&state_handle); // Process the image

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_FW_SEND_IMAGE_COMPLETE_CNF, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_fw_send_image_complete_cnf.error_code);

    // Check the image that has been written to the FLASH
    fs_t file_system;
    fs_handle_t file_system_handle;
    uint8_t flash_fw_data[fw_size];
    uint32_t bytes_read;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_ARTIC_FIRM_IMAGE, FS_MODE_READONLY, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_read(file_system_handle, &flash_fw_data[0], fw_size, &bytes_read));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Look for differences between the two
    bool flash_and_image_match = true;
    for (uint32_t i = 0; i < fw_size; ++i)
    {
        if (flash_fw_data[i] != fw_image[i])
        {
            flash_and_image_match = false;
            break;
        }
    }

    EXPECT_TRUE(flash_and_image_match);
}

TEST_F(Sm_MainTest, USBFwWriteMultipleArtic)
{
    const uint32_t packet_number = 20;
    const uint32_t packet_size = 512;
    const uint32_t fw_size = packet_number * packet_size;
    uint8_t fw_image[fw_size];
    uint32_t fw_crc32 = 0;

    uint8_t fw_type = FW_SEND_IMAGE_REQ_ARTIC;

    BootTagsNotSetUSBConnected();

    // Generate the FW image to be sent
    for (uint32_t i = 0; i < fw_size; ++i)
        fw_image[i] = rand();

    // Calculate the crc
    fw_crc32 = crc32(fw_crc32, fw_image, fw_size);

    // Generate FW write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_FW_SEND_IMAGE_REQ);
    req.cmd_fw_send_image_req.image_type = fw_type;
    req.cmd_fw_send_image_req.length = fw_size;
    req.cmd_fw_send_image_req.CRC32 = fw_crc32;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_fw_send_image_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Send the full firmware image in discrete packets
    for (uint32_t i = 0; i < packet_number; ++i)
    {
        syshal_usb_queue_message(fw_image + (i * packet_size), packet_size);
        sm_tick(&state_handle); // Process the image
    }

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_FW_SEND_IMAGE_COMPLETE_CNF, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_fw_send_image_complete_cnf.error_code);

    // Check the image that has been written to the FLASH
    fs_t file_system;
    fs_handle_t file_system_handle;
    uint8_t flash_fw_data[fw_size];
    uint32_t bytes_read;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_ARTIC_FIRM_IMAGE, FS_MODE_READONLY, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_read(file_system_handle, &flash_fw_data[0], fw_size, &bytes_read));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Look for differences between the two
    bool flash_and_image_match = true;
    for (uint32_t i = 0; i < fw_size; ++i)
    {
        if (flash_fw_data[i] != fw_image[i])
        {
            flash_and_image_match = false;
            break;
        }
    }

    EXPECT_TRUE(flash_and_image_match);
}

TEST_F(Sm_MainTest, DISABLED_USBFwApplyImageCorrectArtic)
{
    const uint32_t packet_number = 20;
    const uint32_t packet_size = 512;
    const uint32_t fw_size = packet_number * packet_size;
    uint8_t fw_image[fw_size];
    uint32_t fw_crc32 = 0;

    uint8_t fw_type = FW_SEND_IMAGE_REQ_ARTIC;

    BootTagsNotSetUSBConnected();

    // Generate the FW image to be sent
    for (uint32_t i = 0; i < fw_size; ++i)
        fw_image[i] = rand();

    // Calculate the crc
    fw_crc32 = crc32(fw_crc32, fw_image, fw_size);

    // Generate FW write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_FW_SEND_IMAGE_REQ);
    req.cmd_fw_send_image_req.image_type = fw_type;
    req.cmd_fw_send_image_req.length = fw_size;
    req.cmd_fw_send_image_req.CRC32 = fw_crc32;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_fw_send_image_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Send the full firmware image in discrete packets
    for (uint32_t i = 0; i < packet_number; ++i)
    {
        syshal_usb_queue_message(fw_image + (i * packet_size), packet_size);
        sm_tick(&state_handle); // Process the image
    }

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_FW_SEND_IMAGE_COMPLETE_CNF, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_fw_send_image_complete_cnf.error_code);

    // Check the image that has been written to the FLASH
    fs_t file_system;
    fs_handle_t file_system_handle;
    uint8_t flash_fw_data[fw_size];
    uint32_t bytes_read;

    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_ARTIC_FIRM_IMAGE, FS_MODE_READONLY, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_read(file_system_handle, &flash_fw_data[0], fw_size, &bytes_read));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    // Look for differences between the two
    bool flash_and_image_match = true;
    for (uint32_t i = 0; i < fw_size; ++i)
    {
        if (flash_fw_data[i] != fw_image[i])
        {
            flash_and_image_match = false;
            break;
        }
    }

    EXPECT_TRUE(flash_and_image_match);

    // Generate an apple image request
    CMD_SET_HDR((&req), CMD_FW_APPLY_IMAGE_REQ);
    req.cmd_fw_apply_image_req.image_type = fw_type;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_fw_apply_image_req_t));

    sm_tick(&state_handle); // Process the request

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, DISABLED_FwApplyImageNotFound)
{
    uint8_t fw_type = FILE_ID_APP_FIRM_IMAGE;

    BootTagsNotSetUSBConnected();

    // Generate an apply image request
    cmd_t req;
    CMD_SET_HDR((&req), CMD_FW_APPLY_IMAGE_REQ);
    req.cmd_fw_apply_image_req.image_type = fw_type;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_fw_apply_image_req_t));

    sm_tick(&state_handle); // Process the request

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_FILE_NOT_FOUND, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBCellularWriteBridging)
{
    BootTagsNotSetUSBConnected();

    // Generate Cellular write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CELLULAR_WRITE_REQ);
    req.cmd_gps_write_req.length = 100;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_cellular_write_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_BRIDGING_DISABLED, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, USBCellularWriteSuccess)
{
    uint32_t cellular_write_length = 256;

    BootTagsNotSetUSBConnected();

    // Generate cellular config message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CELLULAR_CONFIG_REQ);
    req.cmd_cellular_config_req.enable = true;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_cellular_config_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    sm_tick(&state_handle); // Process the message

    // Generate cellular write request message
    CMD_SET_HDR((&req), CMD_CELLULAR_WRITE_REQ);
    req.cmd_cellular_write_req.length = cellular_write_length;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_cellular_write_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Generate Cellular write payload
    uint8_t cellular_data_packet[cellular_write_length];
    for (unsigned int i = 0; i < sizeof(cellular_data_packet); ++i)
        cellular_data_packet[i] = i;

    syshal_usb_queue_message(cellular_data_packet, sizeof(cellular_data_packet));

    sm_tick(&state_handle); // Process the message

    // Check message wrote is as expected
    bool cellular_write_mismatch = false;
    for (unsigned int i = 0; i < cellular_write_length; ++i)
    {
        if (cellular_write_buffer[i] != cellular_data_packet[i])
        {
            //printf("IT[%u]:\tbuffer %u\tpacket %u\n", i, cellular_write_buffer[i], cellular_data_packet[i] );
            cellular_write_mismatch = true;
            //break;
        }
    }

    EXPECT_FALSE(cellular_write_mismatch);
}

TEST_F(Sm_MainTest, USBCellularReadBridgingOff)
{
    BootTagsNotSetUSBConnected();

    // Generate CELLULAR read request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CELLULAR_READ_REQ);
    req.cmd_cellular_read_req.length = 100;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_cellular_read_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_CELLULAR_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_BRIDGING_DISABLED, resp.cmd_cellular_read_resp.error_code);
}

TEST_F(Sm_MainTest, USBCellularReadSuccess)
{
    uint32_t cellular_read_length = 256;
    uint16_t error_code;

    syshal_cellular_sync_comms_IgnoreAndReturn(SYSHAL_CELLULAR_NO_ERROR); // WARN: I am unsure why this call in SetUp() does not work. Perhaps a bug in CMock?

    BootTagsNotSetUSBConnected();

    // Generate cellular config message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CELLULAR_CONFIG_REQ);
    req.cmd_cellular_config_req.enable = true;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_cellular_config_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    sm_tick(&state_handle); // Process the message

    // Generate CELLULAR read request message
    CMD_SET_HDR((&req), CMD_CELLULAR_READ_REQ);
    req.cmd_cellular_read_req.length = cellular_read_length;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_cellular_read_req_t));

    syshal_cellular_available_raw_ExpectAndReturn(cellular_read_length);

    sm_tick(&state_handle); // Process the message

    // Check the response
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_CELLULAR_READ_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_NO_ERROR, resp.cmd_generic_resp.error_code);

    // Load the CELLULAR UART buffer with test data
    for (unsigned int i = 0; i < cellular_read_length; ++i)
        cellular_receive_buffer.push(i);

    sm_tick(&state_handle);

    uint8_t received_data[cellular_read_length];
    syshal_usb_fetch_response(received_data);

    // Look for mismatch between data received on UART and data transmitted on config_if
    bool UART_and_config_if_mismatch = false;
    for (unsigned int i = 0; i < cellular_read_length; ++i)
    {
        if (received_data[i] != i)
        {
            UART_and_config_if_mismatch = true;
            break;
        }
    }

    EXPECT_FALSE(UART_and_config_if_mismatch);
}

TEST_F(Sm_MainTest, USBCellularWriteBridgingOff)
{
    BootTagsNotSetUSBConnected();

    // Generate cellular write request message
    cmd_t req;
    CMD_SET_HDR((&req), CMD_CELLULAR_WRITE_REQ);
    req.cmd_gps_write_req.length = 100;
    syshal_usb_queue_message((uint8_t *) &req, CMD_SIZE(cmd_cellular_write_req_t));

    sm_tick(&state_handle); // Process the message

    // Check the response
    cmd_t resp;
    syshal_usb_fetch_response(&resp);
    EXPECT_EQ(CMD_SYNCWORD, resp.hdr.sync);
    EXPECT_EQ(CMD_GENERIC_RESP, resp.hdr.cmd);
    EXPECT_EQ(CMD_ERROR_BRIDGING_DISABLED, resp.cmd_generic_resp.error_code);
}

TEST_F(Sm_MainTest, FsScriptConfigurationUpdateSuccessful)
{
    fs_t file_system;
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint8_t buffer[SYS_CONFIG_MAX_DATA_SIZE];
    cmd_t req;

    // Generate the configuration update file in FLASH
    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_COMMANDS, FS_MODE_CREATE, NULL));

    // Generate cfg erase all request message
    CMD_SET_HDR((&req), CMD_CFG_ERASE_REQ);
    req.cmd_cfg_erase_req.configuration_tag = CFG_ERASE_REQ_ERASE_ALL;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, (uint8_t *) &req, CMD_SIZE(cmd_cfg_erase_req_t), &bytes_written));

    // Generate cfg write request message
    CMD_SET_HDR((&req), CMD_CFG_WRITE_REQ);
    req.cmd_cfg_write_req.length = MEMBER_SIZE(sys_config_gps_log_position_enable_t, contents) + MEMBER_SIZE(sys_config_gps_trigger_mode_t, contents) + MEMBER_SIZE(sys_config_temp_sensor_log_enable_t, contents) + 3 * SYS_CONFIG_TAG_ID_SIZE;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, (uint8_t *) &req, CMD_SIZE(cmd_cfg_write_req_t), &bytes_written));

    // Generate cfg write data payload
    // GPS log pos enable
    buffer[0] = ((uint16_t) SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE);
    buffer[1] = ((uint16_t) SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE) >> 8;
    sys_config_gps_log_position_enable_t gps_log_position_enable;
    gps_log_position_enable.contents.enable = 1;
    buffer[2] = gps_log_position_enable.contents.enable;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, buffer, 3, &bytes_written));

    // GPS trigger mode
    buffer[0] = ((uint16_t) SYS_CONFIG_TAG_GPS_TRIGGER_MODE);
    buffer[1] = ((uint16_t) SYS_CONFIG_TAG_GPS_TRIGGER_MODE) >> 8;
    sys_config_gps_trigger_mode_t gps_trigger_mode;
    gps_trigger_mode.contents.mode = 1;
    buffer[2] = gps_trigger_mode.contents.mode;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, buffer, 3, &bytes_written));

    // Temp sensor log enable
    buffer[0] = (uint8_t) (((uint16_t) SYS_CONFIG_TAG_TEMP_SENSOR_LOG_ENABLE));
    buffer[1] = (uint8_t) (((uint16_t) SYS_CONFIG_TAG_TEMP_SENSOR_LOG_ENABLE) >> 8);
    sys_config_temp_sensor_log_enable_t temp_sensor_log_enable;
    temp_sensor_log_enable.contents.enable = 1;
    buffer[2] = temp_sensor_log_enable.contents.enable;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, buffer, 3, &bytes_written));

    // Generate cfg save request message
    CMD_SET_HDR((&req), CMD_CFG_SAVE_REQ);
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, (uint8_t *) &req, CMD_SIZE(cmd_generic_req_t), &bytes_written));

    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    BootTagsNotSet();
    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));
    EXPECT_EQ(CONFIG_IF_BACKEND_FS_SCRIPT, config_if_current());

    for (auto i = 0; i < 20; ++i)
    {
        sm_tick(&state_handle);
        IncrementSeconds(1);
    }

    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(FS_ERROR_FILE_NOT_FOUND, fs_open(file_system, &file_system_handle, FILE_ID_CONF_COMMANDS, FS_MODE_READONLY, NULL));

    // Blank our sys_config
    memset(&sys_config, 0, sizeof(sys_config));
    // Then reload it from the FS
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));

    // And check its contents
    EXPECT_TRUE(sys_config.gps_log_position_enable.hdr.set);
    EXPECT_EQ(1, sys_config.gps_log_position_enable.contents.enable);
    EXPECT_TRUE(sys_config.gps_trigger_mode.hdr.set);
    EXPECT_EQ(1, sys_config.gps_trigger_mode.contents.mode);
    EXPECT_TRUE(sys_config.temp_sensor_log_enable.hdr.set);
    EXPECT_EQ(1, sys_config.temp_sensor_log_enable.contents.enable);
}

TEST_F(Sm_MainTest, FsScriptConfigurationUpdateMalformed)
{
    fs_t file_system;
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint8_t buffer[SYS_CONFIG_MAX_DATA_SIZE];
    cmd_t req;

    // Generate the configuration update file in FLASH
    EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
    EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_COMMANDS, FS_MODE_CREATE, NULL));

    // Generate cfg erase all request message
    CMD_SET_HDR((&req), CMD_CFG_ERASE_REQ);
    req.cmd_cfg_erase_req.configuration_tag = CFG_ERASE_REQ_ERASE_ALL;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, (uint8_t *) &req, CMD_SIZE(cmd_cfg_erase_req_t) + 1, &bytes_written));

    // Generate cfg write request message
    CMD_SET_HDR((&req), CMD_CFG_WRITE_REQ);
    req.cmd_cfg_write_req.length = MEMBER_SIZE(sys_config_gps_log_position_enable_t, contents) + MEMBER_SIZE(sys_config_gps_trigger_mode_t, contents) + MEMBER_SIZE(sys_config_temp_sensor_log_enable_t, contents) + 3 * SYS_CONFIG_TAG_ID_SIZE;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, (uint8_t *) &req, CMD_SIZE(cmd_cfg_write_req_t) - 1, &bytes_written));

    // Generate cfg write data payload
    // GPS log pos enable
    buffer[0] = ((uint16_t) SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE);
    buffer[1] = ((uint16_t) SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE) >> 8;
    sys_config_gps_log_position_enable_t gps_log_position_enable;
    gps_log_position_enable.contents.enable = 1;
    buffer[2] = gps_log_position_enable.contents.enable;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, buffer, 3, &bytes_written));

    // GPS trigger mode
    buffer[0] = ((uint16_t) SYS_CONFIG_TAG_GPS_TRIGGER_MODE);
    buffer[1] = ((uint16_t) SYS_CONFIG_TAG_GPS_TRIGGER_MODE) >> 8;
    sys_config_gps_trigger_mode_t gps_trigger_mode;
    gps_trigger_mode.contents.mode = 1;
    buffer[2] = gps_trigger_mode.contents.mode;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, buffer, 3, &bytes_written));

    // Temp sensor log enable
    buffer[0] = (uint8_t) (((uint16_t) SYS_CONFIG_TAG_TEMP_SENSOR_LOG_ENABLE));
    buffer[1] = (uint8_t) (((uint16_t) SYS_CONFIG_TAG_TEMP_SENSOR_LOG_ENABLE) >> 8);
    sys_config_temp_sensor_log_enable_t temp_sensor_log_enable;
    temp_sensor_log_enable.contents.enable = 1;
    buffer[2] = temp_sensor_log_enable.contents.enable;
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, buffer, 3, &bytes_written));

    // Generate cfg save request message
    CMD_SET_HDR((&req), CMD_CFG_SAVE_REQ);
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, (uint8_t *) &req, CMD_SIZE(cmd_generic_req_t), &bytes_written));

    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    BootTagsNotSet();
    EXPECT_EQ(SM_MAIN_PROVISIONING, sm_get_current_state(&state_handle));
    EXPECT_EQ(CONFIG_IF_BACKEND_FS_SCRIPT, config_if_current());

    for (auto i = 0; i < 20; ++i)
    {
        sm_tick(&state_handle);
        IncrementSeconds(1);
    }

    EXPECT_EQ(SM_MAIN_PROVISIONING_NEEDED, sm_get_current_state(&state_handle));
    EXPECT_EQ(CONFIG_IF_BACKEND_NOT_SET, config_if_current());
    EXPECT_EQ(SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND, sys_config_load_from_fs(file_system));
    EXPECT_EQ(FS_ERROR_FILE_NOT_FOUND, fs_open(file_system, &file_system_handle, FILE_ID_CONF_COMMANDS, FS_MODE_READONLY, NULL));
}

TEST_F(Sm_MainTest, DISABLED_TestCellular)
{
    bool is_blinking;
    uint32_t colour;

    boot_operational_cellular_test();

    // Increment to get past the initial 5 seconds of yellow LED flashing
    for (unsigned int i = 0; i < 8; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_EQ(SYSHAL_LED_NO_ERROR, syshal_led_get(&colour, &is_blinking));
    EXPECT_EQ(SYSHAL_LED_COLOUR_YELLOW, colour);
    EXPECT_FALSE(is_blinking);

    for (unsigned int i = 0; i < 10; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_led_is_active());

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
}

TEST_F(Sm_MainTest, DISABLED_TestSatellite)
{
    bool is_blinking;
    uint32_t colour;

    boot_operational_satellite_test();

    // Increment to get past the initial 5 seconds of blue LED flashing
    for (unsigned int i = 0; i < 8; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_EQ(SYSHAL_LED_NO_ERROR, syshal_led_get(&colour, &is_blinking));
    EXPECT_EQ(SYSHAL_LED_COLOUR_BLUE, colour);
    EXPECT_FALSE(is_blinking);

    for (unsigned int i = 0; i < 10; ++i)
    {
        IncrementSeconds(1);
        sm_tick(&state_handle);
    }

    EXPECT_FALSE(syshal_led_is_active());

    EXPECT_EQ(SM_MAIN_OPERATIONAL, sm_get_current_state(&state_handle));
}
