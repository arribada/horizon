// test_iot.cpp - IOT subsystem unit tests
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
#include "iot.h"
#include "fs_priv.h"
#include "fs.h"
#include "Mockaws.h"
#include "Mocksyshal_cellular.h"
#include "Mocksyshal_sat.h"
#include "Mocksyshal_flash.h"
#include "logging.h"
}

#include <gtest/gtest.h>
#include <chrono>
#include <random>

sys_config_iot_general_settings_t       iot_general_settings;
sys_config_iot_cellular_settings_t      iot_cellular_settings;
sys_config_iot_cellular_aws_settings_t  iot_cellular_aws_settings;
sys_config_iot_cellular_apn_t           iot_cellular_apn;
sys_config_iot_sat_settings_t           iot_sat_settings;
sys_config_iot_sat_artic_settings_t     iot_sat_artic_settings;

char logging_topic_path_full[sizeof(iot_cellular_aws_settings.contents.thing_name) + sizeof(iot_cellular_aws_settings.contents.logging_topic_path)];
char device_shadow_path_full[sizeof(iot_cellular_aws_settings.contents.thing_name) + sizeof(iot_cellular_aws_settings.contents.device_shadow_path)];

// syshal_flash
fs_t file_system;
#define FS_DEVICE 0
#define FILE_ID_LOG 1
#define FLASH_SIZE          (FS_PRIV_SECTOR_SIZE * FS_PRIV_MAX_SECTORS)
#define SYSHAL_CELLULAR_CODE_NO_ERROR 0
char flash_ram[FLASH_SIZE];

uint16_t error_code_no_error = SYSHAL_CELLULAR_CODE_NO_ERROR;

int syshal_flash_init_callback(uint32_t drive, uint32_t device, int cmock_num_calls) {return SYSHAL_FLASH_NO_ERROR;}

int syshal_flash_read_callback(uint32_t device, void * dest, uint32_t address, uint32_t size, int cmock_num_calls)
{
    //printf("syshal_flash_read(%08x,%u)\n", address, size);
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

int syshal_cellular_check_sim_return_val = SYSHAL_CELLULAR_NO_ERROR;
uint8_t syshal_cellular_check_sim_imsi[64] = {0};
int syshal_cellular_check_sim_callback(uint8_t * imsi, uint16_t *error_code, int cmock_num_calls)
{
    if (SYSHAL_CELLULAR_NO_ERROR != syshal_cellular_check_sim_return_val)
        return syshal_cellular_check_sim_return_val;

    memcpy(imsi, syshal_cellular_check_sim_imsi, sizeof(syshal_cellular_check_sim_imsi));

    *error_code = 0;
    return SYSHAL_CELLULAR_NO_ERROR;
}

int syshal_cellular_check_get_return_val = SYSHAL_CELLULAR_NO_ERROR;
uint32_t syshal_cellular_https_get_expected_timeout = 0;
char syshal_cellular_https_get_expected_domain[64] = {0};
uint32_t syshal_cellular_https_get_expected_port = 0;
char syshal_cellular_https_get_expected_path[256] = {0};

int syshal_cellular_https_get_callback(uint32_t timeout, const char *domain, uint32_t port, const char *path, uint16_t *error_code, int cmock_num_calls)
{
    if (SYSHAL_CELLULAR_NO_ERROR != syshal_cellular_check_get_return_val)
        return syshal_cellular_check_get_return_val;

    EXPECT_EQ(syshal_cellular_https_get_expected_timeout, timeout);
    EXPECT_TRUE(0 == memcmp(domain, syshal_cellular_https_get_expected_domain, sizeof(syshal_cellular_https_get_expected_domain)));
    EXPECT_EQ(syshal_cellular_https_get_expected_port, port);
    EXPECT_TRUE(0 == memcmp(path, syshal_cellular_https_get_expected_path, sizeof(syshal_cellular_https_get_expected_path)));

    *error_code = 0;
    return SYSHAL_CELLULAR_NO_ERROR;
}

int syshal_cellular_https_post_return_val = SYSHAL_CELLULAR_NO_ERROR;
uint32_t syshal_cellular_https_post_expected_timeout = 0;
char syshal_cellular_https_post_expected_domain[64] = {0};
uint32_t syshal_cellular_https_post_expected_port = 0;
char syshal_cellular_https_post_expected_path[512] = {0};
int syshal_cellular_https_post_callback(uint32_t timeout, const char *domain, uint32_t port, const char *path, uint16_t *error_code, int cmock_num_calls)
{
    if (SYSHAL_CELLULAR_NO_ERROR != syshal_cellular_https_post_return_val)
        return syshal_cellular_https_post_return_val;

    EXPECT_EQ(syshal_cellular_https_post_expected_timeout, timeout);
    EXPECT_TRUE(0 == memcmp(domain, syshal_cellular_https_post_expected_domain, sizeof(syshal_cellular_https_post_expected_domain)));
    EXPECT_EQ(syshal_cellular_https_post_expected_port, port);
    EXPECT_TRUE(0 == memcmp(path, syshal_cellular_https_post_expected_path, sizeof(syshal_cellular_https_post_expected_path)));

    *error_code = 0;
    return SYSHAL_CELLULAR_NO_ERROR;
}

std::vector<std::vector<uint8_t>> syshal_cellular_files;
int syshal_cellular_write_from_buffer_to_file_callback(const uint8_t * buffer, uint32_t buffer_size, int cmock_num_calls)
{
    EXPECT_LE(buffer_size, IOT_AWS_MAX_FILE_SIZE); // Check we're not writing more than the maximum supported file size
    std::vector<uint8_t> syshal_cellular_file;
    for (auto i = 0; i < buffer_size; ++i)
        syshal_cellular_file.push_back(buffer[i]);

    syshal_cellular_files.push_back(syshal_cellular_file);

    return SYSHAL_CELLULAR_NO_ERROR;
}

int syshal_cellular_read_from_file_to_fs_return_val = SYSHAL_CELLULAR_NO_ERROR;
std::vector<uint8_t> syshal_cellular_read_from_file_to_fs_file;
uint32_t syshal_cellular_read_from_file_to_fs_http_code = 200;
int syshal_cellular_read_from_file_to_fs_callback(fs_handle_t handle, uint16_t * http_code, uint32_t * file_size, int cmock_num_calls)
{
    *http_code = syshal_cellular_read_from_file_to_fs_http_code;

    if (SYSHAL_CELLULAR_NO_ERROR != syshal_cellular_check_get_return_val)
        return syshal_cellular_check_get_return_val;

    uint32_t written;
    int ret = fs_write(handle, &syshal_cellular_read_from_file_to_fs_file[0], syshal_cellular_read_from_file_to_fs_file.size(), &written);
    if (ret)
        return SYSHAL_CELLULAR_ERROR_DEVICE;

    if (file_size)
        *file_size = syshal_cellular_read_from_file_to_fs_file.size();

    return SYSHAL_CELLULAR_NO_ERROR;
}

class IOTTest : public ::testing::Test
{

    virtual void SetUp()
    {
        Mocksyshal_cellular_Init();
        Mocksyshal_sat_Init();
        Mocksyshal_flash_Init();
        Mockaws_Init();

        // syshal_cellular
        syshal_cellular_check_sim_StubWithCallback(syshal_cellular_check_sim_callback);
        syshal_cellular_check_sim_return_val = SYSHAL_CELLULAR_NO_ERROR;
        syshal_cellular_https_get_StubWithCallback(syshal_cellular_https_get_callback);
        syshal_cellular_https_post_StubWithCallback(syshal_cellular_https_post_callback);
        syshal_cellular_https_post_return_val = -1000;
        syshal_cellular_files.clear();
        syshal_cellular_write_from_buffer_to_file_StubWithCallback(syshal_cellular_write_from_buffer_to_file_callback);
        syshal_cellular_read_from_file_to_fs_file.clear();
        syshal_cellular_read_from_file_to_fs_StubWithCallback(syshal_cellular_read_from_file_to_fs_callback);
        syshal_cellular_get_line_error_IgnoreAndReturn(0);
        syshal_cellular_network_info_IgnoreAndReturn(0);

        // syshal_flash
        syshal_flash_init_StubWithCallback(syshal_flash_init_callback);
        syshal_flash_read_StubWithCallback(syshal_flash_read_callback);
        syshal_flash_write_StubWithCallback(syshal_flash_write_callback);
        syshal_flash_erase_StubWithCallback(syshal_flash_erase_callback);

        memset(flash_ram, 0xFF, sizeof(flash_ram));
        EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
        EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));

        // Null all our current settings
        memset(&iot_general_settings, 0, sizeof(iot_general_settings));
        memset(&iot_cellular_settings, 0, sizeof(iot_cellular_settings));
        memset(&iot_cellular_apn, 0, sizeof(iot_cellular_apn));
        memset(&iot_cellular_aws_settings, 0, sizeof(iot_cellular_aws_settings));
        memset(&iot_sat_settings, 0, sizeof(iot_sat_settings));
        memset(&iot_sat_artic_settings, 0, sizeof(iot_sat_artic_settings));

        default_iot_init_config();

        srand(time(NULL));
    }

    virtual void TearDown()
    {
        Mocksyshal_cellular_Verify();
        Mocksyshal_cellular_Destroy();
        Mocksyshal_sat_Verify();
        Mocksyshal_sat_Destroy();
        Mockaws_Verify();
        Mockaws_Destroy();
        Mocksyshal_flash_Verify();
        Mocksyshal_flash_Destroy();
    }

public:

    iot_init_t iot_init_config =
    {
        .iot_config = &iot_general_settings,
        .iot_cellular_config = &iot_cellular_settings,
        .iot_cellular_aws_config = &iot_cellular_aws_settings,
        .iot_cellular_apn = &iot_cellular_apn,
        .iot_sat_config = &iot_sat_settings,
        .iot_sat_artic_config = &iot_sat_artic_settings
    };

    void default_iot_init_config()
    {
        // General settings
        iot_general_settings.contents.enable = true;
        iot_general_settings.contents.log_enable = false;
        iot_general_settings.contents.min_battery_threshold = 0;
        iot_general_settings.hdr.set = true;

        // Cellular general settings
        iot_cellular_settings.contents.enable = true;
        iot_cellular_settings.contents.connection_priority = random_range(0U, 10);
        iot_cellular_settings.contents.connection_mode = random_range(0U, 2);
        iot_cellular_settings.contents.log_filter = 0;
        iot_cellular_settings.contents.status_filter = 0;
        iot_cellular_settings.contents.check_firmware_updates = 0;
        iot_cellular_settings.contents.check_configuration_updates = 0;
        iot_cellular_settings.contents.min_updates = 0;
        iot_cellular_settings.contents.max_interval = 0;
        iot_cellular_settings.contents.min_interval = 0;
        iot_cellular_settings.contents.max_backoff_interval = 0;
        iot_cellular_settings.contents.gps_schedule_interval_on_max_backoff = 0;
        iot_cellular_settings.hdr.set = true;

        // Cellular AWS settings
        strcpy(iot_cellular_aws_settings.contents.arn, "a8fb7n41z7p2n.iot.us-west-2.amazonaws.com");
        iot_cellular_aws_settings.contents.port = random_range(0U, UINT16_MAX);
        strcpy(iot_cellular_aws_settings.contents.thing_name, "NatGeo-PlasticTracker-0001");
        strcpy(iot_cellular_aws_settings.contents.logging_topic_path, "/topics/#/logging");
        strcpy(iot_cellular_aws_settings.contents.device_shadow_path, "/things/#/shadow");
        strcpy(logging_topic_path_full, "/topics/NatGeo-PlasticTracker-0001/logging");
        strcpy(device_shadow_path_full, "/things/NatGeo-PlasticTracker-0001/shadow");
        iot_cellular_aws_settings.hdr.set = true;

        // Cellular APN setting
        strcpy((char*)iot_cellular_apn.contents.apn.name, "Everywhere");

        iot_cellular_apn.contents.apn.username[0] = '\0';
        iot_cellular_apn.contents.apn.password[0] = '\0';
        iot_cellular_apn.hdr.set = true;

        // General satellite settings
        iot_sat_settings.contents.enable = true;
        iot_sat_settings.contents.connection_priority = random_range(0U, 10);
        iot_sat_settings.contents.status_filter = 0;
        iot_sat_settings.contents.min_updates = 0;
        iot_sat_settings.contents.max_interval = 0;
        iot_sat_settings.contents.min_interval = 0;
        iot_sat_settings.contents.randomised_tx_window = 0;
        iot_sat_settings.hdr.set = true;

        // Artic settings
        iot_sat_artic_settings.contents.device_identifier = rand();
        for (auto i = 0; i < 8; ++i)
        {
            iot_sat_artic_settings.contents.bulletin_data[i].sat[0] = (char) i + 1;
            iot_sat_artic_settings.contents.bulletin_data[i].time_bulletin = rand();
            for (auto j = 0; j < 6; ++j)
                iot_sat_artic_settings.contents.bulletin_data[i].params[j] = random_range(-100.0f, 100.0f);
        }
        iot_sat_artic_settings.hdr.set = true;

        // FS settings
        iot_init_config.satellite_firmware_file_id = rand();
    }

    uint32_t random_range(uint32_t startval, uint32_t endval)
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 gen(seed);
        std::uniform_int_distribution<> dis(startval, endval);

        return dis(gen);
    }

    float random_range(float startval, float endval)
    {
        float scale = rand() / (float) RAND_MAX;
        return startval + scale * (endval - startval);
    }

    std::string random_string(size_t length)
    {
        auto randchar = []() -> char
        {
            const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
            const size_t max_index = (sizeof(charset) - 1);
            return charset[ rand() % max_index ];
        };
        std::string str(length, 0);
        std::generate_n( str.begin(), length, randchar );
        return str;
    }

    void cellular_power_on()
    {
        syshal_cellular_power_on_ExpectAndReturn(SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_sync_comms_ExpectAndReturn(&error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_sync_comms_IgnoreArg_error_code();
        syshal_cellular_sync_comms_ReturnThruPtr_error_code(&error_code_no_error);

        syshal_cellular_create_secure_profile_ExpectAndReturn(&error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_create_secure_profile_ReturnThruPtr_error_code(&error_code_no_error);
        syshal_cellular_create_secure_profile_IgnoreArg_error_code();
        syshal_cellular_set_rat_IgnoreAndReturn(SYSHAL_CELLULAR_NO_ERROR);
        EXPECT_EQ(IOT_NO_ERROR, iot_power_on(IOT_RADIO_CELLULAR));
    }

    void cellular_connect()
    {
        uint32_t connect_timeout = rand();
        syshal_cellular_scan_ExpectAndReturn(connect_timeout, &error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_scan_IgnoreArg_error_code();
        syshal_cellular_scan_ReturnThruPtr_error_code(&error_code_no_error);
        syshal_cellular_attach_ExpectAndReturn(connect_timeout, &error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_attach_IgnoreArg_error_code();
        syshal_cellular_attach_ReturnThruPtr_error_code(&error_code_no_error);
        syshal_cellular_activate_pdp_ExpectAndReturn(((syshal_cellular_apn_t*)&iot_cellular_apn.contents), connect_timeout, &error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
        syshal_cellular_activate_pdp_IgnoreArg_error_code();
        syshal_cellular_activate_pdp_ReturnThruPtr_error_code(&error_code_no_error);
        EXPECT_EQ(IOT_NO_ERROR, iot_connect(connect_timeout));
    }

    void cellular_power_off()
    {
        syshal_cellular_power_off_ExpectAndReturn(SYSHAL_CELLULAR_NO_ERROR);
        EXPECT_EQ(IOT_NO_ERROR, iot_power_off());
    }

    void satallite_power_on()
    {
        syshal_sat_power_on_ExpectAndReturn(SYSHAL_SAT_NO_ERROR);
        syshal_sat_program_firmware_ExpectAndReturn(0, iot_init_config.satellite_firmware_file_id, SYSHAL_SAT_NO_ERROR);
        syshal_sat_program_firmware_IgnoreArg_fs();
        EXPECT_EQ(IOT_NO_ERROR, iot_power_on(IOT_RADIO_SATELLITE));
    }

    uint8_t generate_invalid_log_value()
    {
        bool tag_valid = true;
        uint8_t tag;
        while (tag_valid)
        {
            tag = rand();
            tag_valid = false;
            for (auto i = 0; i < LOGGING_COUNT; ++i)
            {
                if (logging_tag_lookup[i] == tag)
                {
                    tag_valid = true;
                    break;
                }
            }
        }
        return tag;
    }

    void create_log_file(std::vector<uint8_t> log)
    {
        fs_handle_t file_system_handle;
        uint32_t bytes_written_fs;

        EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));
        EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &log[0], log.size(), &bytes_written_fs));
        EXPECT_EQ(bytes_written_fs, log.size());
        EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));
    }

    std::vector<uint8_t> generate_log_data(size_t min_length)
    {
        std::vector<uint8_t> log_file;

        while (log_file.size() < min_length)
        {
            uint8_t tag_id = logging_tag_lookup[random_range(0U, LOGGING_COUNT - 1)];
            size_t tag_size;
            EXPECT_EQ(LOGGING_NO_ERROR, logging_tag_size(tag_id, &tag_size));
            std::vector<uint8_t> tag_contents;
            log_file.push_back(tag_id);
            for (auto i = 1; i < tag_size; ++i)
                log_file.push_back(rand());
        };

        return log_file;
    }

    bool log_file_valid(std::vector<uint8_t> log_file)
    {
        auto byte_idx = 0;
        while (byte_idx < log_file.size())
        {
            size_t tag_size;
            if (logging_tag_size(log_file[byte_idx], &tag_size))
                return false;
            byte_idx += tag_size;
        }

        if (byte_idx != log_file.size())
            return false;

        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// iot_init() ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST_F(IOTTest, successfulInit)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// iot_power_on() //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST_F(IOTTest, powerOnCellularSuccess)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
}

TEST_F(IOTTest, powerOnInvalidParam)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_INVALID_PARAM, iot_power_on(static_cast<iot_radio_type_t>(0xFF)));
}

TEST_F(IOTTest, powerOnCellularBackendPowerOnFailure)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    syshal_cellular_power_on_ExpectAndReturn(-1);
    EXPECT_EQ(IOT_ERROR_POWER_ON_FAIL, iot_power_on(IOT_RADIO_CELLULAR));
}

TEST_F(IOTTest, powerOnCellularBackendSyncCommsFailure)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    syshal_cellular_power_on_ExpectAndReturn(SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_sync_comms_ExpectAndReturn(&error_code_no_error, -1);
    syshal_cellular_sync_comms_ReturnThruPtr_error_code(&error_code_no_error);
    syshal_cellular_sync_comms_IgnoreArg_error_code();


    EXPECT_EQ(IOT_ERROR_SYNC_COMS_FAIL, iot_power_on(IOT_RADIO_CELLULAR));
}

TEST_F(IOTTest, powerOnCellularBackendCreateProfileFailure)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    syshal_cellular_power_on_ExpectAndReturn(SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_sync_comms_ExpectAndReturn(&error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_sync_comms_ReturnThruPtr_error_code(&error_code_no_error);
    syshal_cellular_sync_comms_IgnoreArg_error_code();

    syshal_cellular_create_secure_profile_ExpectAndReturn(&error_code_no_error, -1);
    syshal_cellular_create_secure_profile_ReturnThruPtr_error_code(&error_code_no_error);
    EXPECT_EQ(IOT_ERROR_CREATE_PDP_FAIL, iot_power_on(IOT_RADIO_CELLULAR));
}

TEST_F(IOTTest, powerOnIotNotEnabled)
{
    iot_general_settings.contents.enable = false;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_ERROR_NOT_ENABLED, iot_power_on(IOT_RADIO_CELLULAR));
}

TEST_F(IOTTest, powerOnCellularNotEnabled)
{
    iot_cellular_settings.contents.enable = false;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_ERROR_NOT_ENABLED, iot_power_on(IOT_RADIO_CELLULAR));
}

TEST_F(IOTTest, powerOnCellularNotSet)
{
    iot_cellular_settings.hdr.set = false;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_ERROR_NOT_ENABLED, iot_power_on(IOT_RADIO_CELLULAR));
}

TEST_F(IOTTest, powerOnCellularAlreadyOn)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    EXPECT_EQ(IOT_ERROR_RADIO_ALREADY_ON, iot_power_on(IOT_RADIO_CELLULAR));
}

TEST_F(IOTTest, powerOnCellularcheckSimNotPresent)
{
    iot_imsi_t imsi;
    syshal_cellular_check_sim_return_val = SYSHAL_CELLULAR_ERROR_SIM_CARD_NOT_FOUND;

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    syshal_cellular_power_on_ExpectAndReturn(SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_sync_comms_ExpectAndReturn(&error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_sync_comms_ReturnThruPtr_error_code(&error_code_no_error);
    syshal_cellular_sync_comms_IgnoreArg_error_code();

    EXPECT_EQ(IOT_ERROR_CHECK_SIM_FAIL, iot_power_on(IOT_RADIO_CELLULAR));
}

TEST_F(IOTTest, powerOnCellularcheckSimBackendError)
{
    iot_imsi_t imsi;
    syshal_cellular_check_sim_return_val = -1;

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    syshal_cellular_power_on_ExpectAndReturn(SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_sync_comms_ExpectAndReturn(&error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_sync_comms_ReturnThruPtr_error_code(&error_code_no_error);
    syshal_cellular_sync_comms_IgnoreArg_error_code();

    EXPECT_EQ(IOT_ERROR_CHECK_SIM_FAIL, iot_power_on(IOT_RADIO_CELLULAR));
}

TEST_F(IOTTest, DISABLED_powerOnCellularSatAlreadyOn)
{

}

TEST_F(IOTTest, DISABLED_powerOnSatSuccess)
{

}

TEST_F(IOTTest, DISABLED_powerOnSatNotEnabled)
{

}

TEST_F(IOTTest, DISABLED_powerOnSatAlreadyOn)
{

}

TEST_F(IOTTest, DISABLED_powerOnSatCellularAlreadyOn)
{

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// iot_power_off() /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST_F(IOTTest, powerOffCellular)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_power_off();
}

TEST_F(IOTTest, powerOffCellularBackendError)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    syshal_cellular_power_off_ExpectAndReturn(-1);
    EXPECT_EQ(IOT_NO_ERROR, iot_power_off());
}

TEST_F(IOTTest, DISABLED_powerOffSat)
{

}

TEST_F(IOTTest, powerOffNothingOn)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_off();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// iot_check_sim() /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST_F(IOTTest, checkSimNotPoweredOn)
{
    iot_imsi_t imsi;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_ERROR_RADIO_NOT_ON, iot_check_sim(&imsi));
}

TEST_F(IOTTest, checkSimNotSupportedByRadio)
{
    iot_imsi_t imsi;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    satallite_power_on();
    EXPECT_EQ(IOT_ERROR_NOT_SUPPORTED, iot_check_sim(&imsi));
}

TEST_F(IOTTest, checkSimNotPresent)
{
    iot_imsi_t imsi;

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();

    syshal_cellular_check_sim_return_val = SYSHAL_CELLULAR_ERROR_SIM_CARD_NOT_FOUND;
    EXPECT_EQ(IOT_ERROR_CHECK_SIM_FAIL, iot_check_sim(&imsi));
}

TEST_F(IOTTest, checkSimBackendError)
{
    iot_imsi_t imsi;

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();

    syshal_cellular_check_sim_return_val = -1;
    EXPECT_EQ(IOT_ERROR_CHECK_SIM_FAIL, iot_check_sim(&imsi));
}

TEST_F(IOTTest, checkSimSuccess)
{
    iot_imsi_t imsi;

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();

    syshal_cellular_check_sim_return_val = SYSHAL_CELLULAR_NO_ERROR;

    for (auto i = 0; i < sizeof(syshal_cellular_check_sim_imsi); ++i)
        syshal_cellular_check_sim_imsi[i] = random_range(0U, UINT8_MAX);
    EXPECT_EQ(IOT_NO_ERROR, iot_check_sim(&imsi));
    EXPECT_TRUE(0 == memcmp(&imsi, &syshal_cellular_check_sim_imsi, sizeof(imsi)));
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// iot_connect() //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define CONNECT_TIMEOUT (1000)

TEST_F(IOTTest, connectSuccessful)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();
}

TEST_F(IOTTest, connectNotPoweredOn)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_ERROR_RADIO_NOT_ON, iot_connect(CONNECT_TIMEOUT));
}

TEST_F(IOTTest, connectScanTimeout)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    syshal_cellular_scan_ExpectAndReturn(CONNECT_TIMEOUT, &error_code_no_error, SYSHAL_CELLULAR_ERROR_TIMEOUT);
    syshal_cellular_scan_IgnoreArg_error_code();
    syshal_cellular_scan_ReturnThruPtr_error_code(&error_code_no_error);
    EXPECT_EQ(IOT_ERROR_SCAN_FAIL, iot_connect(CONNECT_TIMEOUT));
}

TEST_F(IOTTest, connectAttachCopsProblem)
{
    iot_error_report_t report_error;

    int16_t iot_error_code;
    int16_t hal_error_code;
    uint16_t hal_line_number;
    uint16_t vendor_error_code;

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    syshal_cellular_scan_ExpectAndReturn(CONNECT_TIMEOUT, &error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_scan_IgnoreArg_error_code();
    syshal_cellular_scan_ReturnThruPtr_error_code(&error_code_no_error);
    syshal_cellular_attach_ExpectAndReturn(CONNECT_TIMEOUT, 0, SYSHAL_CELLULAR_ERROR_TIMEOUT);
    syshal_cellular_attach_IgnoreArg_error_code();
    uint16_t error_code_fail_cops = 111;
    syshal_cellular_attach_ReturnThruPtr_error_code(&error_code_fail_cops);
    EXPECT_EQ(IOT_ERROR_ATTACH_FAIL, iot_connect(CONNECT_TIMEOUT));
    iot_get_error_report(&report_error);
    EXPECT_EQ(SYSHAL_CELLULAR_ERROR_TIMEOUT, report_error.hal_error_code );
    EXPECT_EQ(111,report_error.vendor_error_code);
    EXPECT_EQ(0, report_error.hal_line_number);
    EXPECT_EQ(IOT_ERROR_ATTACH_FAIL, report_error.iot_error_code);
}

TEST_F(IOTTest, connectAttachTimeout)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    syshal_cellular_scan_ExpectAndReturn(CONNECT_TIMEOUT, &error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_scan_IgnoreArg_error_code();
    syshal_cellular_scan_ReturnThruPtr_error_code(&error_code_no_error);
    syshal_cellular_attach_ExpectAndReturn(CONNECT_TIMEOUT, 0, SYSHAL_CELLULAR_ERROR_TIMEOUT);
    syshal_cellular_attach_IgnoreArg_error_code();
    syshal_cellular_attach_ReturnThruPtr_error_code(&error_code_no_error);
    EXPECT_EQ(IOT_ERROR_ATTACH_FAIL, iot_connect(CONNECT_TIMEOUT));
}

TEST_F(IOTTest, connectActivatePdpTimeout)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    syshal_cellular_scan_ExpectAndReturn(CONNECT_TIMEOUT, &error_code_no_error, SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_scan_IgnoreArg_error_code();
    syshal_cellular_scan_ReturnThruPtr_error_code(&error_code_no_error);
    syshal_cellular_attach_ExpectAndReturn(CONNECT_TIMEOUT, 0, SYSHAL_CELLULAR_NO_ERROR);
    syshal_cellular_attach_IgnoreArg_error_code();
    syshal_cellular_attach_ReturnThruPtr_error_code(&error_code_no_error);
    syshal_cellular_activate_pdp_ExpectAndReturn(((syshal_cellular_apn_t*)&iot_cellular_apn.contents), CONNECT_TIMEOUT, &error_code_no_error, SYSHAL_CELLULAR_ERROR_TIMEOUT);
    syshal_cellular_activate_pdp_IgnoreArg_error_code();
    syshal_cellular_activate_pdp_ReturnThruPtr_error_code(&error_code_no_error);
    EXPECT_EQ(IOT_ERROR_ACTIVATE_PDP_FAIL, iot_connect(CONNECT_TIMEOUT));
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////// iot_fetch_device_shadow() //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define FETCH_DEVICE_TIMEOUT (996)

TEST_F(IOTTest, fetchDeviceShadowNotPoweredOn)
{
    iot_device_shadow_t device_shadow;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_ERROR_RADIO_NOT_ON, iot_fetch_device_shadow(FETCH_DEVICE_TIMEOUT, &device_shadow));
}

TEST_F(IOTTest, fetchDeviceSatNotSupported)
{
    iot_device_shadow_t device_shadow;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    satallite_power_on();
    EXPECT_EQ(IOT_ERROR_NOT_SUPPORTED, iot_fetch_device_shadow(FETCH_DEVICE_TIMEOUT, &device_shadow));
}

TEST_F(IOTTest, fetchDeviceShadowNotConnected)
{
    iot_device_shadow_t device_shadow;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    EXPECT_EQ(IOT_ERROR_NOT_CONNECTED, iot_fetch_device_shadow(FETCH_DEVICE_TIMEOUT, &device_shadow));
}

TEST_F(IOTTest, DISABLED_fetchDeviceShadowSuccess)
{
    iot_device_shadow_t device_shadow;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    syshal_cellular_https_get_expected_timeout = FETCH_DEVICE_TIMEOUT;

    memcpy(syshal_cellular_https_get_expected_domain, iot_cellular_aws_settings.contents.arn, sizeof(syshal_cellular_https_get_expected_domain));
    memcpy(syshal_cellular_https_get_expected_path, iot_cellular_aws_settings.contents.device_shadow_path, sizeof(syshal_cellular_https_get_expected_path));
    syshal_cellular_https_get_expected_port = iot_cellular_aws_settings.contents.port;

    EXPECT_EQ(IOT_ERROR_NOT_CONNECTED, iot_fetch_device_shadow(FETCH_DEVICE_TIMEOUT, &device_shadow));
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////// iot_send_device_status() //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define SEND_STATUS_TIMEOUT (997)

TEST_F(IOTTest, sendDeviceStatusNotPoweredOn)
{
    iot_device_status_t device_status;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_ERROR_RADIO_NOT_ON, iot_send_device_status(SEND_STATUS_TIMEOUT, &device_status));
}

TEST_F(IOTTest, sendDeviceStatusNotConnected)
{
    iot_device_status_t device_status;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    EXPECT_EQ(IOT_ERROR_NOT_CONNECTED, iot_send_device_status(SEND_STATUS_TIMEOUT, &device_status));
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////// iot_send_logging() ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define SEND_LOG_TIMEOUT (998)

TEST_F(IOTTest, sendLoggingNotPoweredOn)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_ERROR_RADIO_NOT_ON, iot_send_logging(SEND_LOG_TIMEOUT, file_system, 0, 0));
}

TEST_F(IOTTest, sendLoggingSatNotSupported)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    satallite_power_on();
    EXPECT_EQ(IOT_ERROR_NOT_SUPPORTED, iot_send_logging(SEND_LOG_TIMEOUT, file_system, 0, 0));
}

TEST_F(IOTTest, sendLoggingNotConnected)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    EXPECT_EQ(IOT_ERROR_NOT_CONNECTED, iot_send_logging(SEND_LOG_TIMEOUT, file_system, 0, 0));
}

TEST_F(IOTTest, sendLoggingFileNotFound)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    EXPECT_EQ(IOT_ERROR_NOT_CONNECTED, iot_send_logging(SEND_LOG_TIMEOUT, file_system, 0, 0));
}

TEST_F(IOTTest, sendLoggingNoData)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Generate an empty log file
    std::vector<uint8_t> log_file;
    create_log_file(log_file);

    EXPECT_EQ(IOT_NO_ERROR, iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, log_file.size()));

    // Check no file was sent
    EXPECT_EQ(0, syshal_cellular_files.size());
}

TEST_F(IOTTest, sendLoggingFileSmall)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Generate a small log file
    std::vector<uint8_t> log_file = generate_log_data(512);
    create_log_file(log_file);

    // HTTPS post expected values
    syshal_cellular_https_post_return_val = SYSHAL_CELLULAR_NO_ERROR;
    syshal_cellular_https_post_expected_timeout = SEND_LOG_TIMEOUT;
    memcpy(syshal_cellular_https_post_expected_domain, iot_cellular_aws_settings.contents.arn, sizeof(syshal_cellular_https_get_expected_domain));
    memcpy(syshal_cellular_https_post_expected_path, logging_topic_path_full, strlen(logging_topic_path_full));
    syshal_cellular_https_post_expected_port = iot_cellular_aws_settings.contents.port;

    EXPECT_EQ(log_file.size(), iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, 0));

    // Check only one file was sent
    EXPECT_EQ(1, syshal_cellular_files.size());

    // And that it matches what we tried to send
    EXPECT_TRUE(std::equal(log_file.begin(), log_file.end(), syshal_cellular_files[0].begin()));
}

TEST_F(IOTTest, sendLoggingExampleFile)
{
    fs_handle_t file_system_handle;
    uint32_t bytes_written_fs;
    uint8_t working_buffer[512];
    long int file_size;
    FILE * fp;

    // Load the bin file from disk into our file system
    fp = fopen("../FilesTest/logLarge.bin", "rb");
    ASSERT_NE(nullptr, fp);

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_ID_LOG, FS_MODE_CREATE, NULL));

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    auto bytes_read = fread(working_buffer, 1, sizeof(working_buffer), fp);
    while (bytes_read > 0)
    {
        EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, working_buffer, bytes_read, &bytes_written_fs));
        EXPECT_EQ(bytes_written_fs, bytes_read);
        bytes_read = fread(working_buffer, 1, sizeof(working_buffer), fp);
    }

    EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

    fclose(fp);

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // HTTPS post expected values
    syshal_cellular_https_post_return_val = SYSHAL_CELLULAR_NO_ERROR;
    syshal_cellular_https_post_expected_timeout = SEND_LOG_TIMEOUT;
    memcpy(syshal_cellular_https_post_expected_domain, iot_cellular_aws_settings.contents.arn, sizeof(syshal_cellular_https_get_expected_domain));
    memcpy(syshal_cellular_https_post_expected_path, logging_topic_path_full, strlen(logging_topic_path_full));
    syshal_cellular_https_post_expected_port = iot_cellular_aws_settings.contents.port;

    EXPECT_EQ(file_size, iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, 0));

    // Check only one file was sent
    EXPECT_EQ(1, syshal_cellular_files.size());

    // And that it matches what we tried to send
    //EXPECT_TRUE(std::equal(log_file.begin(), log_file.end(), syshal_cellular_files[0].begin()));
}

TEST_F(IOTTest, sendLoggingFileTwo)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Generate a log file that is slightly over the max file size
    std::vector<uint8_t> log_file = generate_log_data(IOT_AWS_MAX_FILE_SIZE + 1);
    create_log_file(log_file);

    // HTTPS post expected values
    syshal_cellular_https_post_return_val = SYSHAL_CELLULAR_NO_ERROR;
    syshal_cellular_https_post_expected_timeout = SEND_LOG_TIMEOUT;
    memcpy(syshal_cellular_https_post_expected_domain, iot_cellular_aws_settings.contents.arn, sizeof(syshal_cellular_https_get_expected_domain));
    memcpy(syshal_cellular_https_post_expected_path, logging_topic_path_full, strlen(logging_topic_path_full));
    syshal_cellular_https_post_expected_port = iot_cellular_aws_settings.contents.port;

    EXPECT_EQ(log_file.size(), iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, 0));

    // Check two files were sent
    EXPECT_EQ(2, syshal_cellular_files.size());

    // Check both files don't contain any partial log entries
    EXPECT_TRUE(log_file_valid(syshal_cellular_files[0]));
    EXPECT_TRUE(log_file_valid(syshal_cellular_files[1]));

    // Reconstruct the two log files and check that they match what we tried to send
    std::vector<uint8_t> concatenated_files;
    concatenated_files.insert(concatenated_files.end(), syshal_cellular_files[0].begin(), syshal_cellular_files[0].end());
    concatenated_files.insert(concatenated_files.end(), syshal_cellular_files[1].begin(), syshal_cellular_files[1].end());

    // And that it matches what we tried to send
    EXPECT_TRUE(std::equal(log_file.begin(), log_file.end(), concatenated_files.begin()));
}

TEST_F(IOTTest, sendLoggingFileMany)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Generate a large log file
    std::vector<uint8_t> log_file = generate_log_data(IOT_AWS_MAX_FILE_SIZE * 100);
    create_log_file(log_file);

    // HTTPS post expected values
    syshal_cellular_https_post_return_val = SYSHAL_CELLULAR_NO_ERROR;
    syshal_cellular_https_post_expected_timeout = SEND_LOG_TIMEOUT;
    memcpy(syshal_cellular_https_post_expected_domain, iot_cellular_aws_settings.contents.arn, sizeof(syshal_cellular_https_get_expected_domain));
    memcpy(syshal_cellular_https_post_expected_path, logging_topic_path_full, strlen(logging_topic_path_full));
    syshal_cellular_https_post_expected_port = iot_cellular_aws_settings.contents.port;

    EXPECT_EQ(log_file.size(), iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, 0));

    // Check files don't contain any partial log entries
    std::vector<uint8_t> concatenated_files;
    for (auto i = 0; i < syshal_cellular_files.size(); ++i)
    {
        EXPECT_NE(0, syshal_cellular_files[i].size()); // Check we haven't sent any empty files
        EXPECT_TRUE(log_file_valid(syshal_cellular_files[i])); // Check each file contains no partial log entries
        concatenated_files.insert(concatenated_files.end(), syshal_cellular_files[i].begin(), syshal_cellular_files[i].end());
    }

    // Check all files sent match what we expect
    EXPECT_TRUE(std::equal(log_file.begin(), log_file.end(), concatenated_files.begin()));
}

TEST_F(IOTTest, sendLoggingLargeStartMiddle)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Generate a large log file
    std::vector<uint8_t> log_file = generate_log_data(IOT_AWS_MAX_FILE_SIZE * 10);
    create_log_file(log_file);

    // HTTPS post expected values
    syshal_cellular_https_post_return_val = SYSHAL_CELLULAR_NO_ERROR;
    syshal_cellular_https_post_expected_timeout = SEND_LOG_TIMEOUT;
    memcpy(syshal_cellular_https_post_expected_domain, iot_cellular_aws_settings.contents.arn, sizeof(syshal_cellular_https_get_expected_domain));
    memcpy(syshal_cellular_https_post_expected_path, logging_topic_path_full, strlen(logging_topic_path_full));
    syshal_cellular_https_post_expected_port = iot_cellular_aws_settings.contents.port;

    // Start from a valid position in the middle
    auto last_idx = 0, idx = 0;
    while (idx < log_file.size() / 2)
    {
        size_t tag_size;
        EXPECT_EQ(LOGGING_NO_ERROR, logging_tag_size(log_file[idx], &tag_size));
        last_idx = idx;
        idx += tag_size;
    }

    EXPECT_EQ(log_file.size(), iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, last_idx));

    // Check files don't contain any partial log entries
    std::vector<uint8_t> concatenated_files;
    for (auto i = 0; i < syshal_cellular_files.size(); ++i)
    {
        EXPECT_NE(0, syshal_cellular_files[i].size()); // Check we haven't sent any empty files
        EXPECT_TRUE(log_file_valid(syshal_cellular_files[i])); // Check each file contains no partial log entries
        concatenated_files.insert(concatenated_files.end(), syshal_cellular_files[i].begin(), syshal_cellular_files[i].end());
    }

    // Check all files sent match what we expect
    std::vector<uint8_t>::iterator log_file_itr = log_file.begin();
    std::advance(log_file_itr, last_idx);
    EXPECT_TRUE(std::equal(log_file_itr, log_file.end(), concatenated_files.begin()));
}

TEST_F(IOTTest, sendLoggingFirstByteCorrupt)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Generate a log small
    std::vector<uint8_t> log_file = generate_log_data(512);

    // Set the first byte to be an invalid log entry
    log_file[0] = generate_invalid_log_value();

    create_log_file(log_file);

    EXPECT_EQ(IOT_ERROR_FILE_LOGGING_FAIL, iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, 0));

    // Check no file was sent
    EXPECT_EQ(0, syshal_cellular_files.size());
}

TEST_F(IOTTest, sendLoggingStartAtInvalidPos)
{
    size_t tag_size;

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Generate a large~ish log file
    std::vector<uint8_t> log_file = generate_log_data(IOT_AWS_MAX_FILE_SIZE * 6);
    create_log_file(log_file);

    // HTTPS post expected values
    syshal_cellular_https_post_return_val = SYSHAL_CELLULAR_NO_ERROR;
    syshal_cellular_https_post_expected_timeout = SEND_LOG_TIMEOUT;
    memcpy(syshal_cellular_https_post_expected_domain, iot_cellular_aws_settings.contents.arn, sizeof(syshal_cellular_https_get_expected_domain));
    memcpy(syshal_cellular_https_post_expected_path, logging_topic_path_full, strlen(logging_topic_path_full));
    syshal_cellular_https_post_expected_port = iot_cellular_aws_settings.contents.port;

    // Find a valid position mid-way through
    int valid_mid_point = 0;
    while (valid_mid_point < log_file.size() / 2)
    {
        EXPECT_EQ(LOGGING_NO_ERROR, logging_tag_size(log_file[valid_mid_point], &tag_size));
        valid_mid_point += tag_size;
    }

    EXPECT_EQ(log_file.size(), iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, valid_mid_point));

    // Find a position after this that is part way through a tag entry
    int invalid_mid_point = valid_mid_point;
    while(true)
    {
        EXPECT_EQ(LOGGING_NO_ERROR, logging_tag_size(log_file[invalid_mid_point], &tag_size));
        invalid_mid_point++;
        if (tag_size > 1)
            break;
    };

    EXPECT_EQ(log_file.size(), iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, invalid_mid_point));
}

TEST_F(IOTTest, sendLoggingLargeMiddleCorrupt)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Generate a large~ish log file
    std::vector<uint8_t> log_file = generate_log_data(IOT_AWS_MAX_FILE_SIZE * 6);

    // Corrupt the data from the middle to the end
    for (auto i = log_file.size() / 2; i < log_file.size(); ++i)
        log_file[i] = rand();

    create_log_file(log_file);

    EXPECT_EQ(IOT_ERROR_FILE_LOGGING_FAIL, iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, 0));

    // Check no file was sent
    EXPECT_EQ(0, syshal_cellular_files.size());
}

TEST_F(IOTTest, sendLoggingLargeFinalLogEntryCorrupt)
{
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Generate a large~ish log file
    std::vector<uint8_t> log_file = generate_log_data(IOT_AWS_MAX_FILE_SIZE * 6);

    // Find the very last log entry
    auto last_idx = 0, idx = 0;
    while (idx < log_file.size())
    {
        size_t tag_size;
        EXPECT_EQ(LOGGING_NO_ERROR, logging_tag_size(log_file[idx], &tag_size));
        last_idx = idx;
        idx += tag_size;
    }

    // And corrupt it
    log_file[last_idx] = generate_invalid_log_value();

    create_log_file(log_file);

    EXPECT_EQ(IOT_ERROR_FILE_LOGGING_FAIL, iot_send_logging(SEND_LOG_TIMEOUT, file_system, FILE_ID_LOG, 0));

    // Check no file was sent
    EXPECT_EQ(0, syshal_cellular_files.size());
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////// iot_download_file() ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define DOWNLOAD_TIMEOUT (999)

TEST_F(IOTTest, downloadFileNotPoweredOn)
{
    iot_url_t url;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    EXPECT_EQ(IOT_ERROR_RADIO_NOT_ON, iot_download_file(DOWNLOAD_TIMEOUT, &url, file_system, 0, NULL));
}

TEST_F(IOTTest, downloadFileSatNotSupported)
{
    iot_url_t url;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    satallite_power_on();
    EXPECT_EQ(IOT_ERROR_NOT_SUPPORTED, iot_download_file(DOWNLOAD_TIMEOUT, &url, file_system, 0, NULL));
}

TEST_F(IOTTest, downloadFileNotConnected)
{
    iot_url_t url;
    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    EXPECT_EQ(IOT_ERROR_NOT_CONNECTED, iot_download_file(DOWNLOAD_TIMEOUT, &url, file_system, 0, NULL));
}

TEST_F(IOTTest, downloadFileHttpsGetTimeout)
{
    iot_url_t url;

    strcpy(url.domain, random_string(sizeof(url.domain) - 1).c_str());
    strcpy(url.path, random_string(sizeof(url.path) - 1).c_str());
    url.port = random_range(0U, UINT16_MAX);

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Set up expected values
    syshal_cellular_check_get_return_val = SYSHAL_CELLULAR_ERROR_TIMEOUT;
    syshal_cellular_https_get_expected_timeout = DOWNLOAD_TIMEOUT;
    memcpy(syshal_cellular_https_get_expected_domain, url.domain, sizeof(syshal_cellular_https_get_expected_domain));
    memcpy(syshal_cellular_https_get_expected_path, url.path, sizeof(syshal_cellular_https_get_expected_path));
    syshal_cellular_https_get_expected_port = url.port;

    EXPECT_EQ(IOT_ERROR_GET_DOWNLOAD_FILE_FAIL, iot_download_file(DOWNLOAD_TIMEOUT, &url, file_system, 0, NULL));
}

TEST_F(IOTTest, downloadFileSuccessful)
{
    iot_url_t url;
    fs_handle_t handle;
    const uint32_t file_id = 10;

    strcpy(url.domain, random_string(sizeof(url.domain) - 1).c_str());
    strcpy(url.path, random_string(sizeof(url.path) - 1).c_str());
    url.port = random_range(0U, UINT16_MAX);

    EXPECT_EQ(IOT_NO_ERROR, iot_init(iot_init_config));
    cellular_power_on();
    cellular_connect();

    // Set up expected values
    syshal_cellular_check_get_return_val = SYSHAL_CELLULAR_NO_ERROR;
    syshal_cellular_https_get_expected_timeout = DOWNLOAD_TIMEOUT;
    memcpy(syshal_cellular_https_get_expected_domain, url.domain, sizeof(syshal_cellular_https_get_expected_domain));
    memcpy(syshal_cellular_https_get_expected_path, url.path, sizeof(syshal_cellular_https_get_expected_path));
    syshal_cellular_https_get_expected_port = url.port;

    // Generate a random file to be read
    syshal_cellular_read_from_file_to_fs_return_val = SYSHAL_CELLULAR_NO_ERROR;
    for (auto i = 0; i < 32 * 1024; ++i)
        syshal_cellular_read_from_file_to_fs_file.push_back(rand());

    EXPECT_EQ(IOT_NO_ERROR, iot_download_file(DOWNLOAD_TIMEOUT, &url, file_system, file_id, NULL));

    // Read the file that was written into a temporary buffer
    fs_stat_t stat;
    EXPECT_EQ(FS_NO_ERROR, fs_stat(file_system, file_id, &stat));
    std::vector<uint8_t> read_file;
    read_file.resize(stat.size);

    EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &handle, file_id, FS_MODE_READONLY, NULL));
    uint32_t bytes_read_fs;
    EXPECT_EQ(FS_NO_ERROR, fs_read(handle, &read_file[0], read_file.size(), &bytes_read_fs));
    EXPECT_EQ(bytes_read_fs, read_file.size());
    EXPECT_EQ(FS_NO_ERROR, fs_close(handle));

    // Check all files sent match what we expect
    EXPECT_TRUE(std::equal(read_file.begin(), read_file.end(), syshal_cellular_read_from_file_to_fs_file.begin()));
}