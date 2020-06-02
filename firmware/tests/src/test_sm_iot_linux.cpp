extern "C" {
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "unity.h"
#include "syshal_cellular.h"
#include "syshal_uart.h"
#include "syshal_timer.h"
#include "sm_main.h"
#include "sm_iot.h"
#include "Mocksyshal_batt.h"
#include "Mocksyshal_gpio.h"
#include "Mocksyshal_flash.h"
#include "Mocksyshal_firmware.h"
#include "Mocksyshal_pmu.h"
#include "Mocksyshal_time.h"
#include "Mocksyshal_device.h"
#include "Mocksyshal_sat.h"
#include "Mockprepas.h"
#include "fs_priv.h"
#include "fs.h"
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cmath>
#include <fstream>
#include <deque>

#define THINGNAME "CelullarTestPurple"
#define NAMESPACE "CelullarTestPurple"
#define CERT_PATH "../certificates/"
#define CERT_PATH_ERROR "../certificates_error/"
#define NO_INSTALL_CERTIFICATE (1)
#define TIME_ACC (30)

static uint32_t app_firmware_version = 1;


sm_iot_init_t init;
sys_config_iot_general_settings_t iot_config;
sys_config_iot_cellular_settings_t iot_cellular_config;
sys_config_iot_cellular_aws_settings_t iot_cellular_aws_config;
sys_config_iot_cellular_apn_t iot_cellular_apn;
sys_config_iot_sat_settings_t iot_sat_config;
sys_config_iot_sat_artic_settings_t iot_sat_artic_config;

fs_t file_system;

// syshal_flash
#define FLASH_SIZE          (FS_PRIV_SECTOR_SIZE * FS_PRIV_MAX_SECTORS)
#define ASCII(x)            ((x) >= 32 && (x) <= 127) ? (x) : '.'

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

std::deque<sm_iot_event_t> sm_iot_callback_events;
void sm_iot_callback(sm_iot_event_t * event)
{
    sm_iot_callback_events.push_back(*event);
}

class SmIotTestLinux : public ::testing::Test
{
    virtual void SetUp()
    {
        Mocksyshal_batt_Init();
        Mocksyshal_gpio_Init();
        Mocksyshal_flash_Init();
        Mocksyshal_firmware_Init();
        Mocksyshal_pmu_Init();
        Mocksyshal_time_Init();
        Mocksyshal_device_Init();
        Mocksyshal_sat_Init();
        Mockprepas_Init();

        syshal_time_delay_ms_Ignore();

        syshal_gpio_init_IgnoreAndReturn(SYSHAL_GPIO_NO_ERROR);
        syshal_gpio_set_output_low_IgnoreAndReturn(SYSHAL_GPIO_NO_ERROR);
        syshal_gpio_set_output_high_IgnoreAndReturn(SYSHAL_GPIO_NO_ERROR);

        // syshal_flash
        syshal_flash_init_StubWithCallback(syshal_flash_init_GTest);
        syshal_flash_read_StubWithCallback(syshal_flash_read_GTest);
        syshal_flash_write_StubWithCallback(syshal_flash_write_GTest);
        syshal_flash_erase_StubWithCallback(syshal_flash_erase_GTest);

        sm_iot_callback_events.clear();

        // Clear FLASH contents
        for (auto i = 0; i < FLASH_SIZE; ++i)
            flash_ram[i] = 0xFF;

        // Load the file system
        fs_init(0);
        fs_mount(0, &file_system);

        syshal_uart_init(0);

        // Set up our configuration tags
        init.iot_init.iot_config = &iot_config;
        init.iot_init.iot_cellular_config = &iot_cellular_config;
        init.iot_init.iot_cellular_aws_config = &iot_cellular_aws_config;
        init.iot_init.iot_cellular_apn = &iot_cellular_apn;
        init.iot_init.iot_sat_config = &iot_sat_config;
        init.iot_init.iot_sat_artic_config = &iot_sat_artic_config;

        // Set up the aws configuration options
        iot_cellular_aws_config.hdr.set = true;
        strcpy(iot_cellular_aws_config.contents.arn, "aoof21oggrmpd-ats.iot.us-west-2.amazonaws.com");
        iot_cellular_aws_config.contents.port = 8443;
        strcpy(iot_cellular_aws_config.contents.thing_name, THINGNAME);
        strcpy(iot_cellular_aws_config.contents.logging_topic_path, "/topics/#/logging");
        strcpy(iot_cellular_aws_config.contents.device_shadow_path, "/things/#/shadow");

        // Cellular APN setting
        strcpy((char*)iot_cellular_apn.contents.apn.name, "internetd.gdsp");

        iot_cellular_apn.contents.apn.username[0] = '\0';
        iot_cellular_apn.contents.apn.password[0] = '\0';
        iot_cellular_apn.hdr.set = true;

        syshal_device_firmware_version_IgnoreAndReturn(SYSHAL_DEVICE_NO_ERROR);
        syshal_device_firmware_version_ReturnThruPtr_version(&app_firmware_version);

        // Set up our AWS instance
        system("aws_config --unregister_thing " THINGNAME " --cert_path " CERT_PATH);
        ASSERT_EQ(0, system("aws_config --register_thing " THINGNAME " --cert_path " CERT_PATH));
    }

    virtual void TearDown()
    {
        Mocksyshal_batt_Verify();
        Mocksyshal_batt_Destroy();
        Mocksyshal_gpio_Verify();
        Mocksyshal_gpio_Destroy();
        Mocksyshal_flash_Verify();
        Mocksyshal_flash_Destroy();
        Mocksyshal_firmware_Verify();
        Mocksyshal_firmware_Destroy();
        Mocksyshal_pmu_Verify();
        Mocksyshal_pmu_Destroy();
        Mocksyshal_time_Verify();
        Mocksyshal_time_Destroy();
        Mocksyshal_device_Verify();
        Mocksyshal_device_Destroy();
        Mocksyshal_sat_Verify();
        Mocksyshal_sat_Destroy();
        Mockprepas_Verify();
        Mockprepas_Destroy();
    }

public:

    void setup_default_config()
    {
        iot_config.hdr.set = true;
        iot_config.contents.enable = true;
        //iot_config.contents.log_enable = ;
        //iot_config.contents.min_battery_threshold = ;

        iot_cellular_config.hdr.set = true;
        iot_cellular_config.contents.enable = true;
        //iot_cellular_config.contents.connection_priority = ;
        iot_cellular_config.contents.connection_mode = SYS_CONFIG_TAG_IOT_CELLULAR_CONNECTION_MODE_AUTO;
        iot_cellular_config.contents.log_filter = 0;
        iot_cellular_config.contents.status_filter = IOT_PRESENCE_FLAG_LAST_CELLULAR_CONNECTED_TIMESTAMP & IOT_PRESENCE_FLAG_CONFIGURATION_VERSION & IOT_PRESENCE_FLAG_FIRMWARE_VERSION;
        iot_cellular_config.contents.check_firmware_updates = false;
        iot_cellular_config.contents.check_configuration_updates = false;
        //iot_cellular_config.contents.min_updates = ;
        iot_cellular_config.contents.max_interval = 60; // 60 second max interval
        iot_cellular_config.contents.min_interval = 0;
        iot_cellular_config.contents.max_backoff_interval = 10 * 60; // 10 Minutes
        //iot_cellular_config.contents.gps_schedule_interval_on_max_backoff = ;

        iot_sat_config.hdr.set = false;
        iot_sat_artic_config.hdr.set = false;
    }
    void save_login_fs(int *size)
    {
        fs_handle_t file_handle;
        unsigned MAX_WRITE = 1024;
        uint8_t buffer[MAX_WRITE];
        FILE *checkFile;


        uint32_t remain_data;
        int ret;
        uint32_t bytes_written;
        uint32_t current_read;

        printf("BEFORE FILE OPEN\n");
        checkFile = fopen("../FilesTest/log.bin", "rb");
        ASSERT_EQ(1, (checkFile != NULL));

        fseek(checkFile, 0L, SEEK_END);

        ASSERT_EQ(FS_NO_ERROR, fs_open(file_system, &file_handle, FILE_ID_LOG, FS_MODE_CREATE , NULL));

        int size_ref_file = ftell(checkFile);
        rewind(checkFile);

        *size = size_ref_file;
        ret = FS_NO_ERROR;
        remain_data = size_ref_file;

        while (remain_data > 0 && ret == FS_NO_ERROR)
        {

            if (remain_data > MAX_WRITE)
            {
                current_read = MAX_WRITE;
            }
            else
            {
                current_read = remain_data;
            }
            fread(&buffer, current_read, 1, checkFile);
            ret =  fs_write(file_handle, buffer, current_read, &bytes_written);
            //DOnt do anything if bytes_written not equal to bytes_read
            remain_data -= current_read;
        }
        fs_close(file_handle);

    }

    std::vector<uint8_t> generate_random_file(char * filename, unsigned int size)
    {
        FILE * fp;
        std::vector<uint8_t> contents;
        fp = fopen(filename, "w");
        for (auto i = 0; i < size; ++i)
        {
            uint8_t data = rand();
            contents.push_back(data);
            fwrite(&data, 1, 1, fp);
        }
        fclose(fp);
        return contents;
    }

    void check_last_event(sm_iot_event_id_t id, int return_code)
    {
        ASSERT_LE(1, sm_iot_callback_events.size()) << "No event found";
        EXPECT_EQ(id, sm_iot_callback_events.front().id);
        //EXPECT_EQ(return_code, sm_iot_callback_events.front().code) << "Error ID " << sm_iot_callback_events.front().id;
        sm_iot_callback_events.pop_front();
    }
};


TEST_F(SmIotTestLinux, DISABLED_Power_Characterization)
{
    //
    setup_default_config();
    iot_cellular_config.contents.log_filter = true;
    iot_cellular_config.contents.check_firmware_updates = true;
    iot_cellular_config.contents.check_configuration_updates = true;
    std::string command;
    int ret;

    command = "";
    command += "aws_config ";
    command += "--firmware_update ";
    command += THINGNAME;
    command += " --firmware_version ";
    command += std::to_string(app_firmware_version + 1);
    command += " --file ";
    command += "TestFirmwareUpdate.zip";
    command += " --cert_path ";
    command += CERT_PATH;
    ASSERT_EQ(0, system(command.c_str()));


    command = "";

    command += "aws_config ";
    command += "--configuration_update ";
    command += THINGNAME;
    command += " --file ";
    command += THINGNAME;
    command += ".json";
    command += " --cert_path ";
    command += CERT_PATH;

    ASSERT_EQ(0, system(command.c_str()));

    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));


    syshal_pmu_reset_Expect();
    syshal_firmware_update_ExpectAndReturn(FILE_ID_APP_FIRM_IMAGE, app_firmware_version + 1, SYSHAL_FIRMWARE_NO_ERROR);
    int value;
    printf("PAUSE PRESS ANY BUTTON\n");
    scanf("%d", value);
    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));
    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_SEND_DEVICE_STATUS, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_DOWNLOAD_FIRMWARE_FILE, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_DOWNLOAD_CONFIG_FILE, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_APPLY_FIRMWARE_UPDATE, SM_IOT_NO_ERROR);

    check_last_event(SM_IOT_APPLY_CONFIG_UPDATE, SM_IOT_NO_ERROR);


    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());
}










TEST_F(SmIotTestLinux, CellularFetchShadow)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key --debug"));
    setup_default_config();

    int size;
    save_login_fs(&size);


    iot_cellular_config.contents.log_filter = true;

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_SEND_LOGGING, size);
    check_last_event(SM_IOT_CELLULAR_SEND_DEVICE_STATUS, SM_IOT_NO_ERROR);

    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());
}
TEST_F(SmIotTestLinux, DISABLED_Error_CellularFetchShadow)
{
    setup_default_config();
    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, IOT_ERROR_SCAN_FAIL);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());
}

TEST_F(SmIotTestLinux, DISABLED_CellularFirmwareUpdate)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));
    fs_handle_t file_handle;
    std::string command;
    int ret;

    setup_default_config();
    iot_cellular_config.contents.check_firmware_updates = true;

    //std::vector<uint8_t> contents = generate_random_file("TestFirmwareUpdate.zip", 1024);

    command = "";
    command += "aws_config ";
    command += "--firmware_update ";
    command += THINGNAME;
    command += " --firmware_version ";
    command += std::to_string(app_firmware_version + 1);
    command += " --file ";
    command += "TestFirmwareUpdate.zip";
    command += " --cert_path ";
    command += CERT_PATH;

    ASSERT_EQ(0, system(command.c_str()));


    syshal_firmware_update_ExpectAndReturn(FILE_ID_APP_FIRM_IMAGE, app_firmware_version + 1, SYSHAL_FIRMWARE_NO_ERROR);

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_SEND_DEVICE_STATUS, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_DOWNLOAD_FIRMWARE_FILE, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_APPLY_FIRMWARE_UPDATE, SM_IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());

    // Fetch the file from the fs and load it into a vector
    ASSERT_EQ(FS_NO_ERROR, fs_open(file_system, &file_handle, FILE_ID_APP_FIRM_IMAGE, FS_MODE_READONLY, NULL));
    FILE *checkFile;
    checkFile = fopen("firmwareRef.bin", "rb");
    fseek(checkFile, 0L, SEEK_END);
    int size_ref_file = ftell(checkFile);
    rewind(checkFile);
    int internal_counter = 0;
    std::vector<uint8_t> read_file;
    do
    {
        uint8_t read_buffer[1024];
        uint32_t bytes_actually_read;
        uint8_t ref_buffer;
        ret = fs_read(file_handle, &read_buffer, 1, &bytes_actually_read);
        fread(&ref_buffer, sizeof(ref_buffer), 1, checkFile);
        if (ret != FS_ERROR_END_OF_FILE)
        {
            EXPECT_EQ(1, bytes_actually_read);
        }
        if (bytes_actually_read == 1)
        {
            if (*read_buffer != ref_buffer)
            {
                printf("BYTE[i] DOESN'T MATCH --- read: %u\tref: %u\n", internal_counter, *read_buffer, ref_buffer);
                EXPECT_EQ(*read_buffer, ref_buffer);
            }

        }


    }
    while (FS_ERROR_END_OF_FILE != ret && internal_counter++ < size_ref_file);

    printf("size_ref_file %u\n", size_ref_file);

    EXPECT_EQ(FS_NO_ERROR, fs_close(file_handle));

    // Check the file in the fs matches that which we tried to send
    //EXPECT_TRUE(contents == read_file);
}

TEST_F(SmIotTestLinux, DISABLED_FAILCellularFirmwareUpdateCoverage)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));
    fs_handle_t file_handle;
    std::string command;
    int ret;

    setup_default_config();
    iot_cellular_config.contents.check_firmware_updates = true;

    //std::vector<uint8_t> contents = generate_random_file("TestFirmwareUpdate.zip", 1024);

    command += "aws_config ";
    command += "--firmware_update ";
    command += THINGNAME;
    command += " --firmware_version ";
    command += std::to_string(app_firmware_version + 1);
    command += " --file ";
    command += "TestFirmwareUpdateLARGE.zip";
    command += " --cert_path ";
    command += CERT_PATH;

    ASSERT_EQ(0, system(command.c_str()));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));
    syshal_firmware_update_ExpectAndReturn(FILE_ID_APP_FIRM_IMAGE, app_firmware_version + 1, SYSHAL_FIRMWARE_NO_ERROR);
    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_SEND_DEVICE_STATUS, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_DOWNLOAD_FIRMWARE_FILE, IOT_ERROR_GET_DOWNLOAD_FILE_FAIL); //Not sure
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);


    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());


}


TEST_F(SmIotTestLinux, DISABLED_CellularnoanthenafailCellularConfigurationUpdate)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));
    fs_handle_t file_handle;
    std::string command;
    int ret;

    setup_default_config();
    iot_cellular_config.contents.check_configuration_updates = true;

    //std::vector<uint8_t> contents = generate_random_file("TestFirmwareUpdate.zip", 1024);
    command += "aws_config ";
    command += "--configuration_update ";
    command += THINGNAME;
    command += " --file ";
    command += THINGNAME;
    command += ".json";
    command += " --cert_path ";
    command += CERT_PATH;

    ASSERT_EQ(0, system(command.c_str()));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    syshal_pmu_reset_Expect();
    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_SEND_DEVICE_STATUS, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_DOWNLOAD_CONFIG_FILE, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);


    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());



    // Check the file in the fs matches that which we tried to send
    //EXPECT_TRUE(contents == read_file);

}


TEST_F(SmIotTestLinux, DISABLED_CellularFirmwareUpdateInvalidPath)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));
    // Check for HTTP 404.?
    EXPECT_TRUE(false);
}

TEST_F(SmIotTestLinux, DISABLED_CellularConfigurationUpdateSuccess)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));
    EXPECT_TRUE(false);
}

TEST_F(SmIotTestLinux, DISABLED_CellularConfigurationUpdateInvalidPath)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));
    // Check for HTTP 404.?
    EXPECT_TRUE(false);
}

TEST_F(SmIotTestLinux, DISABLED_CellularCertificate_fail)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH_ERROR "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH_ERROR THINGNAME ".cert --key " CERT_PATH_ERROR THINGNAME ".key"));

    setup_default_config();

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    //check_last_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, IOT_ERROR_TIMEOUT);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);


    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());

}

TEST_F(SmIotTestLinux, DISABLED_CellularSIM_fail)
{

    setup_default_config();

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    //check_last_event(SM_IOT_CELLULAR_POWER_ON, IOT_ERROR_NO_SIM_FOUND );
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);


    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());

}

TEST_F(SmIotTestLinux, DISABLED_Cellularnoanthenafail)
{

    setup_default_config();

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    //check_last_event(SM_IOT_CELLULAR_POWER_ON, IOT_ERROR_NO_SIM_FOUND );
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);


    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());

}
TEST_F(SmIotTestLinux, DISABLED_Cellularpath_fail)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));

    strcpy(iot_cellular_aws_config.contents.device_shadow_path, "/things/#/shadowwww");
    setup_default_config();

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    //check_last_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, IOT_ERROR_HTTP);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);


    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());

}

TEST_F(SmIotTestLinux, DISABLED_Cellularport_fail)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));

    iot_cellular_aws_config.contents.port = 443;
    setup_default_config();

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    //check_last_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, IOT_ERROR_HTTP);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);


    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());

}

TEST_F(SmIotTestLinux, DISABLED_Cellulardomain_fail)
{
    ASSERT_EQ(0, system("cellular_config --serial /dev/ttyUSB0 --root_ca " CERT_PATH "VeriSign-Class-3-Public-Primary-Certification-Authority-G5.pem --cert " CERT_PATH THINGNAME ".cert --key " CERT_PATH THINGNAME ".key"));

    strcpy(iot_cellular_aws_config.contents.arn, "www.amazon.com");

    setup_default_config();

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    //check_last_event(SM_IOT_CELLULAR_FETCH_DEVICE_SHADOW, IOT_ERROR_TIMEOUT);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);


    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());

}

TEST_F(SmIotTestLinux, DISABLED_setrat_2G)
{


    setup_default_config();
    iot_cellular_config.contents.connection_mode = SYS_CONFIG_TAG_IOT_CELLULAR_CONNECTION_MODE_2_G;
    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    check_last_event(SM_IOT_CELLULAR_POWER_ON, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());
}
TEST_F(SmIotTestLinux, DISABLED_setrat_3G)
{


    setup_default_config();
    iot_cellular_config.contents.connection_mode = SYS_CONFIG_TAG_IOT_CELLULAR_CONNECTION_MODE_3_G;
    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    //check_last_event(SM_IOT_CELLULAR_POWER_ON, IOT_ERROR_NOT_RADIO_COVERAGE);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());
}
TEST_F(SmIotTestLinux, DISABLED_setrat_AUTO)
{
    setup_default_config();
    iot_cellular_config.contents.connection_mode = SYS_CONFIG_TAG_IOT_CELLULAR_CONNECTION_MODE_AUTO;

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_init(init));

    ASSERT_EQ(SM_IOT_NO_ERROR, sm_iot_trigger_force(IOT_RADIO_CELLULAR));

    // Check the correct series of events occured
    //check_last_event(SM_IOT_CELLULAR_POWER_ON, IOT_ERROR_NOT_RADIO_COVERAGE);
    check_last_event(SM_IOT_CELLULAR_CONNECT, SM_IOT_NO_ERROR);
    check_last_event(SM_IOT_CELLULAR_POWER_OFF, SM_IOT_NO_ERROR);

    EXPECT_EQ(SM_IOT_NO_ERROR, sm_iot_term());
}