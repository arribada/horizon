// test_sys_config.cpp - System configuration unit tests
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
#include <stdbool.h>
#include "unity.h"
#include "sys_config.h"
#include "Mocksyshal_rtc.h"
#include "Mocksyshal_flash.h"
#include "fs_priv.h"
#include "fs.h"
#include "sm_main.h"
#include "crc_32.h"
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>

#define NUM_OF_TAGS  (60)

// syshal_flash
#define FLASH_SIZE          (FS_PRIV_SECTOR_SIZE * FS_PRIV_MAX_SECTORS)
#define ASCII(x)            ((x) >= 32 && (x) <= 127) ? (x) : '.'

fs_t file_system;
char flash_ram[FLASH_SIZE];

int syshal_flash_init_GTest(uint32_t drive, uint32_t device, int cmock_num_calls) {return SYSHAL_FLASH_NO_ERROR;}

int syshal_flash_read_GTest(uint32_t device, void * dest, uint32_t address, uint32_t size, int cmock_num_calls)
{
    //printf("syshal_flash_read(%08x,%u)\n", address, size);
    for (unsigned int i = 0; i < size; i++)
        ((char *)dest)[i] = flash_ram[address + i];

    return 0;
}

int syshal_flash_write_GTest(uint32_t device, const void * src, uint32_t address, uint32_t size, int cmock_num_calls)
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

syshal_rtc_data_and_time_t internal_date_time;

int syshal_rtc_set_callback(syshal_rtc_data_and_time_t date_time, int cmock_num_calls)
{
    internal_date_time = date_time;
    return SYSHAL_RTC_NO_ERROR;
}

int syshal_rtc_get_callback(syshal_rtc_data_and_time_t * date_time, int cmock_num_calls)
{
    memcpy(date_time, &internal_date_time, sizeof(internal_date_time));
    return SYSHAL_RTC_NO_ERROR;
}

class SysConfigTest : public ::testing::Test
{
    virtual void SetUp()
    {
        Mocksyshal_rtc_Init();
        syshal_rtc_set_date_and_time_StubWithCallback(syshal_rtc_set_callback);
        syshal_rtc_get_date_and_time_StubWithCallback(syshal_rtc_get_callback);

        // syshal_flash
        Mocksyshal_flash_Init();
        syshal_flash_init_StubWithCallback(syshal_flash_init_GTest);
        syshal_flash_read_StubWithCallback(syshal_flash_read_GTest);
        syshal_flash_write_StubWithCallback(syshal_flash_write_GTest);
        syshal_flash_erase_StubWithCallback(syshal_flash_erase_GTest);

        // Clear FLASH contents
        for (auto i = 0; i < FLASH_SIZE; ++i)
            flash_ram[i] = 0xFF;

        EXPECT_EQ(FS_NO_ERROR, fs_init(0));
        EXPECT_EQ(FS_NO_ERROR, fs_mount(0, &file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));

        // Blank all the configuration values
        memset(&sys_config, 0, sizeof(sys_config));

        // Blank the current RTC time
        memset(&internal_date_time, 0, sizeof(internal_date_time));
    }

    virtual void TearDown()
    {
        Mocksyshal_rtc_Verify();
        Mocksyshal_rtc_Destroy();
        Mocksyshal_flash_Verify();
        Mocksyshal_flash_Destroy();
    }

public:

    void minimum_configuration_tags_set()
    {
        sys_config.system_device_identifier.hdr.set = true;
    }

    // Function that returns true if the vectors contain the same unique values
    bool compare_unique_values_in_vectors(std::vector<uint16_t> vector_a, std::vector<uint16_t> vector_b)
    {
        std::set<uint16_t> set_a;
        set_a.insert(vector_a.begin(), vector_a.end());

        std::set<uint16_t> set_b;
        set_b.insert(vector_b.begin(), vector_b.end());

        return (set_a == set_b);
    }

    std::vector<uint16_t> get_all_required_tags()
    {
        std::vector<uint16_t> tags_required;
        uint16_t last_index = 0;
        uint16_t tag;

        while (!sys_config_iterate(&tag, &last_index))
        {
            bool required_tag = false;
            int ret = sys_config_is_required(tag, &required_tag);
            EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

            if (required_tag)
                tags_required.push_back(tag);
        }

        return tags_required;
    }

    uint32_t random_range(uint32_t startval, uint32_t endval)
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 gen(seed);
        std::uniform_int_distribution<> dis(startval, endval);

        return dis(gen);
    }

    sys_config_t random_config()
    {
        sys_config_t config;
        for (auto i = 0; i < sizeof(sys_config_t); ++i)
            *((uint8_t *)&config) = rand();
        config.format_version = SYS_CONFIG_FORMAT_VERSION;
        return config;
    }

    bool file_exists(uint8_t file_id)
    {
        fs_handle_t handle;
        int ret = fs_open(file_system, &handle, file_id, FS_MODE_READONLY, NULL);
        if (ret)
            return false;

        fs_close(handle);
        return true;
    }

    bool config_matches_file(sys_config_t config, uint8_t file_id)
    {
        fs_handle_t handle;
        sys_config_t loaded_conf;
        uint32_t bytes_read;
        EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &handle, file_id, FS_MODE_READONLY, NULL));
        EXPECT_EQ(FS_NO_ERROR, fs_read(handle, &loaded_conf, sizeof(loaded_conf), &bytes_read));
        EXPECT_EQ(FS_NO_ERROR, fs_close(handle));
        return !memcmp(&config, &loaded_conf, sizeof(loaded_conf));
    }
};

TEST_F(SysConfigTest, NumberOfTags)
{
    uint16_t last_index = 0;
    uint32_t num_tags = 0;
    uint16_t tag;

    while (!sys_config_iterate(&tag, &last_index))
        num_tags++;

    EXPECT_EQ(NUM_OF_TAGS, num_tags);
}

TEST_F(SysConfigTest, TagExists)
{
    EXPECT_TRUE(sys_config_exists(SYS_CONFIG_TAG_SYSTEM_DEVICE_IDENTIFIER));
    EXPECT_FALSE(sys_config_exists(0xFFFF));
}

TEST_F(SysConfigTest, TagsMaxSize)
{
    uint16_t last_index = 0;
    uint16_t tag;
    size_t max_size = 0;

    while (!sys_config_iterate(&tag, &last_index))
    {
        size_t size;
        int ret = sys_config_size(tag, &size);
        EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);
        if (size > max_size)
            max_size = size;
    }

    EXPECT_EQ(SYS_CONFIG_MAX_DATA_SIZE, max_size);
}

TEST_F(SysConfigTest, SetAndUnset)
{
    int ret;
    bool is_set;
    sys_config_gps_log_position_enable_t gps_log_position_enable;

    // Set the tag
    ret = sys_config_set(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, &gps_log_position_enable.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_gps_log_position_enable_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    // Check its set
    ret = sys_config_is_set(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, &is_set);
    ASSERT_EQ(SYS_CONFIG_NO_ERROR, ret);
    EXPECT_TRUE(is_set);

    // Unset the tag
    ret = sys_config_unset(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE);
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    // Check its unset
    ret = sys_config_is_set(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, &is_set);
    ASSERT_EQ(SYS_CONFIG_NO_ERROR, ret);
    EXPECT_FALSE(is_set);
}

TEST_F(SysConfigTest, SetWrongSize)
{
    sys_config_gps_log_position_enable_t gps_log_position_enable;
    int ret = sys_config_set(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, &gps_log_position_enable.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_gps_log_position_enable_t) + 1);
    EXPECT_EQ(SYS_CONFIG_ERROR_WRONG_SIZE, ret);
}

TEST_F(SysConfigTest, SetInvalidTag)
{
    sys_config_gps_log_position_enable_t gps_log_position_enable;
    int ret = sys_config_set(0xFFFF, &gps_log_position_enable.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_gps_log_position_enable_t));
    EXPECT_EQ(SYS_CONFIG_ERROR_INVALID_TAG, ret);
}

TEST_F(SysConfigTest, UnSetInvalidTag)
{
    int ret = sys_config_unset(0xFFFF);
    EXPECT_EQ(SYS_CONFIG_ERROR_INVALID_TAG, ret);
}

TEST_F(SysConfigTest, AllTagsNotSet)
{
    uint16_t last_index = 0;
    uint16_t tag;

    while (!sys_config_iterate(&tag, &last_index))
    {
        bool set = false;
        int ret = sys_config_is_set(tag, &set);
        EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);
        EXPECT_FALSE(set);
    }
}

TEST_F(SysConfigTest, TagGetNullPointer)
{
    int ret;
    sys_config_gps_log_position_enable_t gps_log_position_enable;
    ret = sys_config_set(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, &gps_log_position_enable.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_gps_log_position_enable_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);
    ret = sys_config_get(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, NULL);
    EXPECT_EQ(SYS_CONFIG_TAG_DATA_SIZE(sys_config_gps_log_position_enable_t), ret);
}

TEST_F(SysConfigTest, InvalidTagRequired)
{
    bool required;
    EXPECT_EQ(SYS_CONFIG_ERROR_INVALID_TAG, sys_config_is_required(0xFFFF, &required));
}

TEST_F(SysConfigTest, AllTagsNotSetRequired)
{
    std::vector<uint16_t> tags_required = get_all_required_tags();

    std::vector<uint16_t> tags_expected;
    tags_expected.push_back(SYS_CONFIG_TAG_SYSTEM_DEVICE_IDENTIFIER);

    EXPECT_TRUE(compare_unique_values_in_vectors(tags_required, tags_expected));
}

TEST_F(SysConfigTest, MinimumRequired)
{
    minimum_configuration_tags_set();

    std::vector<uint16_t> tags_required = get_all_required_tags();

    EXPECT_EQ(tags_required.size(), 0);
}

TEST_F(SysConfigTest, GPSDisabled)
{
    minimum_configuration_tags_set();

    sys_config_gps_log_position_enable_t gps_log_position_enable;
    gps_log_position_enable.contents.enable = false;
    int ret = sys_config_set(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, &gps_log_position_enable.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_gps_log_position_enable_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    std::vector<uint16_t> tags_required = get_all_required_tags();

    EXPECT_EQ(tags_required.size(), 0);
}

TEST_F(SysConfigTest, GPSEnabledDependancies)
{
    minimum_configuration_tags_set();

    sys_config_gps_log_position_enable_t gps_log_position_enable;
    gps_log_position_enable.contents.enable = true;
    int ret = sys_config_set(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, &gps_log_position_enable.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_gps_log_position_enable_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    std::vector<uint16_t> tags_required = get_all_required_tags();

    std::vector<uint16_t> tags_expected;
    tags_expected.push_back(SYS_CONFIG_TAG_GPS_TRIGGER_MODE);

    EXPECT_TRUE(compare_unique_values_in_vectors(tags_required, tags_expected));
}

TEST_F(SysConfigTest, GPSModeScheduledDependancies)
{
    minimum_configuration_tags_set();

    sys_config_gps_log_position_enable_t gps_log_position_enable;
    gps_log_position_enable.contents.enable = true;
    int ret = sys_config_set(SYS_CONFIG_TAG_GPS_LOG_POSITION_ENABLE, &gps_log_position_enable.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_gps_log_position_enable_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    sys_config_gps_trigger_mode_t gps_trigger_mode;
    gps_trigger_mode.contents.mode = SYS_CONFIG_GPS_TRIGGER_MODE_SCHEDULED_BITMASK;
    ret = sys_config_set(SYS_CONFIG_TAG_GPS_TRIGGER_MODE, &gps_trigger_mode.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_gps_trigger_mode_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    std::vector<uint16_t> tags_required = get_all_required_tags();

    std::vector<uint16_t> tags_expected;
    tags_expected.push_back(SYS_CONFIG_TAG_GPS_SCHEDULED_ACQUISITION_INTERVAL);
    tags_expected.push_back(SYS_CONFIG_TAG_GPS_MAXIMUM_ACQUISITION_TIME);

    EXPECT_TRUE(compare_unique_values_in_vectors(tags_required, tags_expected));
}

TEST_F(SysConfigTest, IOTCellAndSatDepedancies)
{
    int ret;

    minimum_configuration_tags_set();

    sys_config_iot_general_settings_t iot_general_settings;
    iot_general_settings.contents.enable = true;
    ret = sys_config_set(SYS_CONFIG_TAG_IOT_GENERAL_SETTINGS, &iot_general_settings.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_iot_general_settings_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    sys_config_iot_cellular_settings_t iot_cellular_settings;
    iot_cellular_settings.contents.enable = true;
    ret = sys_config_set(SYS_CONFIG_TAG_IOT_CELLULAR_SETTINGS, &iot_cellular_settings.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_iot_cellular_settings_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    sys_config_iot_sat_settings_t iot_sat_settings;
    iot_sat_settings.contents.enable = true;
    ret = sys_config_set(SYS_CONFIG_TAG_IOT_SATELLITE_SETTINGS, &iot_sat_settings.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_iot_sat_settings_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    std::vector<uint16_t> tags_required = get_all_required_tags();

    std::vector<uint16_t> tags_expected;
    tags_expected.push_back(SYS_CONFIG_TAG_IOT_CELLULAR_AWS_SETTINGS);
    tags_expected.push_back(SYS_CONFIG_TAG_IOT_CELLULAR_APN);
    tags_expected.push_back(SYS_CONFIG_TAG_IOT_SATELLITE_ARTIC_SETTINGS);

    EXPECT_TRUE(compare_unique_values_in_vectors(tags_required, tags_expected));
}

TEST_F(SysConfigTest, SetDateTime)
{
    // Generate a random date
    syshal_rtc_data_and_time_t rtc_data_time;
    rtc_data_time.year = random_range(1980, 2030);
    rtc_data_time.month = random_range(1, 12);
    rtc_data_time.day = random_range(1, 28);
    rtc_data_time.hours = random_range(0, 23);
    rtc_data_time.minutes = random_range(0, 59);
    rtc_data_time.seconds = random_range(0, 59);
    rtc_data_time.milliseconds = 0;

    // Generate and set the associated configuration tag
    sys_config_rtc_current_date_and_time_t sys_config_date_time;
    sys_config_date_time.contents.year = rtc_data_time.year;
    sys_config_date_time.contents.month = rtc_data_time.month;
    sys_config_date_time.contents.day = rtc_data_time.day;
    sys_config_date_time.contents.hours = rtc_data_time.hours;
    sys_config_date_time.contents.minutes = rtc_data_time.minutes;
    sys_config_date_time.contents.seconds = rtc_data_time.seconds;

    // Set the configuration tag
    int ret = sys_config_set(SYS_CONFIG_TAG_RTC_CURRENT_DATE_AND_TIME, &sys_config_date_time.contents, SYS_CONFIG_TAG_DATA_SIZE(sys_config_rtc_current_date_and_time_t));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, ret);

    // Check date was set correctly
    EXPECT_EQ(internal_date_time.year, rtc_data_time.year);
    EXPECT_EQ(internal_date_time.month, rtc_data_time.month);
    EXPECT_EQ(internal_date_time.day, rtc_data_time.day);
    EXPECT_EQ(internal_date_time.hours, rtc_data_time.hours);
    EXPECT_EQ(internal_date_time.minutes, rtc_data_time.minutes);
    EXPECT_EQ(internal_date_time.seconds, rtc_data_time.seconds);
}

TEST_F(SysConfigTest, GetDateTime)
{
    // Generate a random date
    internal_date_time.year = random_range(1980, 2030);
    internal_date_time.month = random_range(1, 12);
    internal_date_time.day = random_range(1, 28);
    internal_date_time.hours = random_range(0, 23);
    internal_date_time.minutes = random_range(0, 59);
    internal_date_time.seconds = random_range(0, 59);

    sys_config_rtc_current_date_and_time_t * sys_config_date_time;
    void * data_ptr;
    int ret = sys_config_get(SYS_CONFIG_TAG_RTC_CURRENT_DATE_AND_TIME, &data_ptr);

    sys_config_date_time = (sys_config_rtc_current_date_and_time_t*)(((uint8_t *)data_ptr) - sizeof(sys_config_hdr_t));
    EXPECT_EQ(SYS_CONFIG_TAG_DATA_SIZE(sys_config_rtc_current_date_and_time_t), ret);

    // Check the time/dates match
    EXPECT_EQ(sys_config_date_time->contents.year, internal_date_time.year);
    EXPECT_EQ(sys_config_date_time->contents.month, internal_date_time.month);
    EXPECT_EQ(sys_config_date_time->contents.day, internal_date_time.day);
    EXPECT_EQ(sys_config_date_time->contents.hours, internal_date_time.hours);
    EXPECT_EQ(sys_config_date_time->contents.minutes, internal_date_time.minutes);
    EXPECT_EQ(sys_config_date_time->contents.seconds, internal_date_time.seconds);
}

TEST_F(SysConfigTest, ConfigFileSaveLoadLoop)
{
    sys_config_t config = random_config();

    sys_config = config;
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_save_to_fs(file_system));
    memset(&sys_config, 0, sizeof(sys_config));
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));

    // Memcmp should be safe for comparing these structs as they are packed
    EXPECT_EQ(0, memcmp(&sys_config, &config, sizeof(sys_config))) << "Loaded config file does not match saved";
}

TEST_F(SysConfigTest, ConfigFileSaveLoadLoopMultiple)
{
    sys_config_t config;

    for (auto i = 0; i < 1000; ++i)
    {
        config = random_config();
        sys_config = config;
        EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_save_to_fs(file_system));
        memset(&sys_config, 0, sizeof(sys_config));
        EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));
        // Memcmp should be safe for comparing these structs as they are packed
        ASSERT_EQ(0, memcmp(&sys_config, &config, sizeof(sys_config))) << "Loaded config file does not match saved";
        EXPECT_TRUE(file_exists(FILE_ID_CONF_PRIMARY));
        EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));

        config = random_config();
        sys_config = config;
        EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_save_to_fs(file_system));
        memset(&sys_config, 0, sizeof(sys_config));
        EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));
        // Memcmp should be safe for comparing these structs as they are packed
        ASSERT_EQ(0, memcmp(&sys_config, &config, sizeof(sys_config))) << "Loaded config file does not match saved";
        EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
        EXPECT_TRUE(file_exists(FILE_ID_CONF_SECONDARY));
    }
}

TEST_F(SysConfigTest, ConfigFileSaveTwoValidVersionNotSet)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();
    config.version.hdr.set = false;

    config.version.contents.version = 9;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    config.version.contents.version = 10;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_SECONDARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    sys_config = random_config();
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_save_to_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_SECONDARY));

    EXPECT_TRUE(config_matches_file(sys_config, FILE_ID_CONF_SECONDARY));
}

TEST_F(SysConfigTest, ConfigFileSaveTwoValidSameVersion)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();
    config.version.hdr.set = true;

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_SECONDARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    sys_config = random_config();
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_save_to_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_SECONDARY));

    EXPECT_TRUE(config_matches_file(sys_config, FILE_ID_CONF_SECONDARY));
}

TEST_F(SysConfigTest, ConfigFileSaveTwoValidPrimaryMoreRecent)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();
    config.version.hdr.set = true;

    config.version.contents.version = 10;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    config.version.contents.version = 9;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_SECONDARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    sys_config = random_config();
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_save_to_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_SECONDARY));

    EXPECT_TRUE(config_matches_file(sys_config, FILE_ID_CONF_SECONDARY));
}

TEST_F(SysConfigTest, ConfigFileSaveTwoValidSecondaryMoreRecent)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();
    config.version.hdr.set = true;

    config.version.contents.version = 9;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    config.version.contents.version = 10;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_SECONDARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    sys_config = random_config();
    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_save_to_fs(file_system));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));

    EXPECT_TRUE(config_matches_file(sys_config, FILE_ID_CONF_PRIMARY));
}

TEST_F(SysConfigTest, ConfigFileLoadNoCrc)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_TRUE(file_exists(FILE_ID_CONF_PRIMARY));

    EXPECT_EQ(SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND, sys_config_load_from_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));
}

TEST_F(SysConfigTest, ConfigFileLoadPrimary)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));
    EXPECT_EQ(0, memcmp(&sys_config, &config, sizeof(sys_config))) << "Loaded config file does not match saved";
}

TEST_F(SysConfigTest, ConfigFileLoadSecondary)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_SECONDARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_SECONDARY));
    EXPECT_EQ(0, memcmp(&sys_config, &config, sizeof(sys_config))) << "Loaded config file does not match saved";
}

TEST_F(SysConfigTest, ConfigFileLoadInvalidFormatVersion)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();
    config.format_version = 0xAA;

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND, sys_config_load_from_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));
}

TEST_F(SysConfigTest, ConfigFileLoadUnderSize)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config) - 1, &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config) - 1);
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND, sys_config_load_from_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));
}

TEST_F(SysConfigTest, ConfigFileLoadOverSize)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    uint8_t extra_byte = rand();
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &extra_byte, sizeof(extra_byte), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    calculated_crc32 = crc32(calculated_crc32, &extra_byte, sizeof(extra_byte));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND, sys_config_load_from_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));
}

TEST_F(SysConfigTest, ConfigFileZeroSize)
{
    fs_handle_t file_system_handle;

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_PRIMARY));

    EXPECT_EQ(SYS_CONFIG_ERROR_NO_VALID_CONFIG_FILE_FOUND, sys_config_load_from_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));
}

TEST_F(SysConfigTest, ConfigFileLoadTwoValidSameVersion)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();
    config.version.hdr.set = true;

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_SECONDARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));
    EXPECT_EQ(0, memcmp(&sys_config, &config, sizeof(sys_config))) << "Loaded config file does not match saved";
}

TEST_F(SysConfigTest, ConfigFileLoadTwoValidPrimaryMoreRecent)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();
    config.version.hdr.set = true;

    config.version.contents.version = 10;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    config.version.contents.version = 9;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_SECONDARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));
    config.version.contents.version = 10;
    EXPECT_EQ(0, memcmp(&sys_config, &config, sizeof(sys_config))) << "Loaded config file does not match saved";
}

TEST_F(SysConfigTest, ConfigFileLoadTwoValidSecondaryMoreRecent)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();
    config.version.hdr.set = true;

    config.version.contents.version = 9;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    config.version.contents.version = 10;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_SECONDARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_SECONDARY));
    EXPECT_EQ(0, memcmp(&sys_config, &config, sizeof(sys_config))) << "Loaded config file does not match saved";
}

TEST_F(SysConfigTest, ConfigFileLoadTwoValidVersionNotSet)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written;
    uint32_t calculated_crc32;
    sys_config_t config;

    config = random_config();
    config.version.hdr.set = false;

    config.version.contents.version = 9;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_PRIMARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    config.version.contents.version = 10;
    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_CONF_SECONDARY, FS_MODE_CREATE, NULL));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &config, sizeof(config), &bytes_written));
    calculated_crc32 = crc32(0, &config, sizeof(config));
    EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &calculated_crc32, sizeof(calculated_crc32), &bytes_written));
    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    EXPECT_EQ(SYS_CONFIG_NO_ERROR, sys_config_load_from_fs(file_system));
    EXPECT_TRUE(file_exists(FILE_ID_CONF_PRIMARY));
    EXPECT_FALSE(file_exists(FILE_ID_CONF_SECONDARY));
    config.version.contents.version = 9;
    EXPECT_EQ(0, memcmp(&sys_config, &config, sizeof(sys_config))) << "Loaded config file does not match saved";
}