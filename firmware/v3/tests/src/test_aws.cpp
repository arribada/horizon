// test_aws.cpp - Amazon Web Services unit tests
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
#include "aws.h"
}

#include <gtest/gtest.h>
#include <cstdarg>
#include <chrono>
#include <random>

class AWSTest : public ::testing::Test
{

    virtual void SetUp()
    {
        srand(time(NULL));
    }

    virtual void TearDown()
    {

    }

public:

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

    iot_device_status_t random_device_status()
    {
        iot_device_status_t status;

        status.presence_flags = rand() & 0x1FF;

        status.last_log_file_read_pos = rand();
        status.last_gps_location.longitude = random_range(-180.0f, 180.0f) * 10000000.0;
        status.last_gps_location.latitude = random_range(-90.0f, 90.0f) * 10000000.0;
        status.last_gps_location.timestamp = rand();
        status.battery_level = rand();
        status.battery_voltage = rand();
        status.last_cellular_connected_timestamp = rand();
        status.last_sat_tx_timestamp = rand();
        status.next_sat_tx_timestamp = rand();
        status.configuration_version = rand();
        status.firmware_version = rand();

        return status;
    }

    std::string format(const char* fmt, ...)
    {
        int size = 512;
        char* buffer = 0;
        buffer = new char[size];
        va_list vl;
        va_start(vl, fmt);
        int nsize = vsnprintf(buffer, size, fmt, vl);
        if (size <= nsize) // fail delete buffer and try again
        {
            delete[] buffer;
            buffer = 0;
            buffer = new char[nsize + 1]; // +1 for null char '/0'
            nsize = vsnprintf(buffer, size, fmt, vl);
        }
        std::string ret(buffer);
        va_end(vl);
        delete[] buffer;
        return ret;
    }

    bool check_json_matches_data(char * json, iot_device_status_t status)
    {
        std::string expected;

        expected.append("{\"state\":{\"desired\":{\"device_status\":{");

        if (status.presence_flags & IOT_PRESENCE_FLAG_LAST_LOG_FILE_READ_POS)
            expected.append(format("\"last_log_file_read_pos\":%lu,", status.last_log_file_read_pos));
        if (status.presence_flags & IOT_PRESENCE_FLAG_LAST_GPS_LOCATION)
            expected.append(format("\"last_gps_location\":{\"longitude\":%f,\"latitude\":%f,\"timestamp\":%lu},",
                                   status.last_gps_location.longitude / 10000000.0,
                                   status.last_gps_location.latitude / 10000000.0,
                                   status.last_gps_location.timestamp));
        if (status.presence_flags & IOT_PRESENCE_FLAG_BATTERY_LEVEL)
            expected.append(format("\"battery_level\":%u,", status.battery_level));
        if (status.presence_flags & IOT_PRESENCE_FLAG_BATTERY_VOLTAGE)
            expected.append(format("\"battery_voltage\":%u,", status.battery_voltage));
        if (status.presence_flags & IOT_PRESENCE_FLAG_LAST_CELLULAR_CONNECTED_TIMESTAMP)
            expected.append(format("\"last_cellular_connected_timestamp\":%lu,", status.last_cellular_connected_timestamp));
        if (status.presence_flags & IOT_PRESENCE_FLAG_LAST_SAT_TX_TIMESTAMP)
            expected.append(format("\"last_sat_tx_timestamp\":%lu,", status.last_sat_tx_timestamp));
        if (status.presence_flags & IOT_PRESENCE_FLAG_NEXT_SAT_TX_TIMESTAMP)
            expected.append(format("\"next_sat_tx_timestamp\":%lu,", status.next_sat_tx_timestamp));
        if (status.presence_flags & IOT_PRESENCE_FLAG_CONFIGURATION_VERSION)
            expected.append(format("\"configuration_version\":%lu,", status.configuration_version));
        if (status.presence_flags & IOT_PRESENCE_FLAG_FIRMWARE_VERSION)
            expected.append(format("\"firmware_version\":%lu,", status.firmware_version));

        // Remove any hanging comma (,)
        if (expected.back() == ',')
            expected.pop_back();

        expected.append("}}}}");

        return !strcmp(json, expected.c_str());
    }

};

////////////////////////////////////////////////////////////////////////////////
///////////////////// aws_json_dumps_device_status() ///////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST_F(AWSTest, jsonDumpTooSmall)
{
    char buffer[4] = {0};
    iot_device_status_t status = random_device_status();
    EXPECT_EQ(AWS_ERROR_BUFFER_TOO_SMALL, aws_json_dumps_device_status(&status, buffer, sizeof(buffer)));
}

TEST_F(AWSTest, jsonDumpNoFields)
{
    char buffer[512] = {0};
    iot_device_status_t status;
    status.presence_flags = 0;
    EXPECT_LT(0, aws_json_dumps_device_status(&status, buffer, sizeof(buffer)));
    EXPECT_TRUE(check_json_matches_data(buffer, status));
}

TEST_F(AWSTest, jsonDumpMultipleRandom)
{
    char buffer[512];

    for (auto i = 0; i < 10000; ++i)
    {
        iot_device_status_t status = random_device_status();
        memset(buffer, 0, sizeof(buffer));
        EXPECT_LT(0, aws_json_dumps_device_status(&status, buffer, sizeof(buffer)));
        bool json_matches = check_json_matches_data(buffer, status);
        if (!json_matches)
        {
            printf("%s\r\n", buffer);
        }
        EXPECT_TRUE(check_json_matches_data(buffer, status));
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////// aws_json_gets_device_shadow() ///////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST_F(AWSTest, jsonGetShadowDeviceStatus)
{
    iot_device_shadow_t shadow;
    char json[2048];
    FILE * fp;

    fp = fopen("../json/test_device_status.json", "r");
    ASSERT_NE(nullptr, fp);
    fread(json, 1, sizeof(json), fp);
    fclose(fp);

    EXPECT_EQ(AWS_NO_ERROR, aws_json_gets_device_shadow(json, &shadow, sizeof(json)));

    EXPECT_EQ(0, shadow.device_update.presence_flags);

    EXPECT_EQ(0x1FF, shadow.device_status.presence_flags);

    EXPECT_EQ(1421, shadow.device_status.last_log_file_read_pos);
    EXPECT_FLOAT_EQ(-12.3, shadow.device_status.last_gps_location.longitude);
    EXPECT_FLOAT_EQ(23.43233, shadow.device_status.last_gps_location.latitude);
    EXPECT_EQ(5635123, shadow.device_status.last_gps_location.timestamp);
    EXPECT_EQ(40, shadow.device_status.battery_level);
    EXPECT_EQ(23, shadow.device_status.battery_voltage);
    EXPECT_EQ(232, shadow.device_status.last_cellular_connected_timestamp);
    EXPECT_EQ(234443, shadow.device_status.last_sat_tx_timestamp);
    EXPECT_EQ(12332, shadow.device_status.next_sat_tx_timestamp);
    EXPECT_EQ(4, shadow.device_status.configuration_version);
    EXPECT_EQ(3, shadow.device_status.firmware_version);
}

TEST_F(AWSTest, jsonGetShadowFirmwareUpdate)
{
    iot_device_shadow_t shadow;
    char json[2048];
    FILE * fp;

    fp = fopen("../json/test_firmware_update.json", "r");
    ASSERT_NE(nullptr, fp);
    fread(json, 1, sizeof(json), fp);
    fclose(fp);

    EXPECT_EQ(AWS_NO_ERROR, aws_json_gets_device_shadow(json, &shadow, sizeof(json)));

    EXPECT_EQ(0, shadow.device_status.presence_flags);

    EXPECT_EQ(IOT_DEVICE_UPDATE_PRESENCE_FLAG_FIRMWARE_UPDATE, shadow.device_update.presence_flags);

    EXPECT_EQ(0, strcmp(shadow.device_update.firmware_update.url.domain, "arribada.s3.amazonaws.com"));
    EXPECT_EQ(0, strcmp(shadow.device_update.firmware_update.url.path, "/f99f1395-dbef-4032-aebc-d01d21b55cf6?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1585394885&Signature=%2F%2Fabi%2BYo8%2FGd69yfB3L2Pk3MEGs%3D"));
    EXPECT_EQ(443, shadow.device_update.firmware_update.url.port);
    EXPECT_EQ(2, shadow.device_update.firmware_update.version);
}

TEST_F(AWSTest, jsonGetShadowConfigurationUpdate)
{
    iot_device_shadow_t shadow;
    char json[2048];
    FILE * fp;

    fp = fopen("../json/test_configuration_update.json", "r");
    ASSERT_NE(nullptr, fp);
    fread(json, 1, sizeof(json), fp);
    fclose(fp);

    EXPECT_EQ(AWS_NO_ERROR, aws_json_gets_device_shadow(json, &shadow, sizeof(json)));

    EXPECT_EQ(0, shadow.device_status.presence_flags);

    EXPECT_EQ(IOT_DEVICE_UPDATE_PRESENCE_FLAG_CONFIGURATION_UPDATE, shadow.device_update.presence_flags);

    EXPECT_EQ(0, strcmp(shadow.device_update.configuration_update.url.domain, "arribada2.s3.amazonaws.com"));
    EXPECT_EQ(0, strcmp(shadow.device_update.configuration_update.url.path, "/dd2eaae2-b564-415c-bd14-67a7e03a9b7b?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1585394952&Signature=eYfbXSIui5XNQ4i0L%2BpbxNOuXrw%3D"));
    EXPECT_EQ(1231, shadow.device_update.configuration_update.url.port);
    EXPECT_EQ(1, shadow.device_update.configuration_update.version);
}

TEST_F(AWSTest, jsonGetShadowFull)
{
    iot_device_shadow_t shadow;
    char json[2048];
    FILE * fp;

    fp = fopen("../json/test_full.json", "r");
    ASSERT_NE(nullptr, fp);
    fread(json, 1, sizeof(json), fp);
    fclose(fp);

    EXPECT_EQ(AWS_NO_ERROR, aws_json_gets_device_shadow(json, &shadow, sizeof(json)));

    EXPECT_EQ(IOT_DEVICE_UPDATE_PRESENCE_FLAG_CONFIGURATION_UPDATE | IOT_DEVICE_UPDATE_PRESENCE_FLAG_FIRMWARE_UPDATE, shadow.device_update.presence_flags);

    // Firmware Update
    EXPECT_EQ(0, strcmp(shadow.device_update.firmware_update.url.domain, "arribada.s3.amazonaws.com"));
    EXPECT_EQ(0, strcmp(shadow.device_update.firmware_update.url.path, "/f99f1395-dbef-4032-aebc-d01d21b55cf6?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1585394885&Signature=%2F%2Fabi%2BYo8%2FGd69yfB3L2Pk3MEGs%3D"));
    EXPECT_EQ(443, shadow.device_update.firmware_update.url.port);
    EXPECT_EQ(2, shadow.device_update.firmware_update.version);

    // Configuration update
    EXPECT_EQ(0, strcmp(shadow.device_update.configuration_update.url.domain, "arribada2.s3.amazonaws.com"));
    EXPECT_EQ(0, strcmp(shadow.device_update.configuration_update.url.path, "/dd2eaae2-b564-415c-bd14-67a7e03a9b7b?AWSAccessKeyId=AKIAIPZT2EDQFBIJHOWA&Expires=1585394952&Signature=eYfbXSIui5XNQ4i0L%2BpbxNOuXrw%3D"));
    EXPECT_EQ(1231, shadow.device_update.configuration_update.url.port);
    EXPECT_EQ(1, shadow.device_update.configuration_update.version);

    // Device Status
    EXPECT_EQ(0x1FF, shadow.device_status.presence_flags);

    EXPECT_EQ(1421, shadow.device_status.last_log_file_read_pos);
    EXPECT_FLOAT_EQ(-12.3, shadow.device_status.last_gps_location.longitude);
    EXPECT_FLOAT_EQ(23.43233, shadow.device_status.last_gps_location.latitude);
    EXPECT_EQ(5635123, shadow.device_status.last_gps_location.timestamp);
    EXPECT_EQ(40, shadow.device_status.battery_level);
    EXPECT_EQ(23, shadow.device_status.battery_voltage);
    EXPECT_EQ(232, shadow.device_status.last_cellular_connected_timestamp);
    EXPECT_EQ(234443, shadow.device_status.last_sat_tx_timestamp);
    EXPECT_EQ(12332, shadow.device_status.next_sat_tx_timestamp);
    EXPECT_EQ(4, shadow.device_status.configuration_version);
    EXPECT_EQ(3, shadow.device_status.firmware_version);
}

TEST_F(AWSTest, jsonCircular)
{
    // Use one function to generate test data for the other
    char json[512];

    for (auto i = 0; i < 10000; ++i)
    {
        iot_device_status_t device_status = random_device_status();
        memset(json, 0, sizeof(json));
        int json_length = aws_json_dumps_device_status(&device_status, json, sizeof(json));
        EXPECT_LT(0, json_length);

        iot_device_shadow_t shadow;
        memset(&shadow, 0, sizeof(shadow));
        shadow.device_update.presence_flags = 0;

        EXPECT_EQ(AWS_NO_ERROR, aws_json_gets_device_shadow(json, &shadow, json_length));

        // Check the values match
        EXPECT_EQ(0, shadow.device_update.presence_flags);

        EXPECT_EQ(device_status.presence_flags, shadow.device_status.presence_flags);
        if (device_status.presence_flags & IOT_PRESENCE_FLAG_LAST_LOG_FILE_READ_POS)
            EXPECT_EQ(device_status.last_log_file_read_pos, shadow.device_status.last_log_file_read_pos);
        if (device_status.presence_flags & IOT_PRESENCE_FLAG_LAST_GPS_LOCATION)
        {
            EXPECT_FLOAT_EQ(shadow.device_status.last_gps_location.longitude, shadow.device_status.last_gps_location.longitude);
            EXPECT_FLOAT_EQ(shadow.device_status.last_gps_location.latitude,  shadow.device_status.last_gps_location.latitude);
            EXPECT_EQ(device_status.last_gps_location.timestamp,              shadow.device_status.last_gps_location.timestamp);
        }
        if (device_status.presence_flags & IOT_PRESENCE_FLAG_BATTERY_LEVEL)
            EXPECT_EQ(device_status.battery_level, shadow.device_status.battery_level);
        if (device_status.presence_flags & IOT_PRESENCE_FLAG_BATTERY_VOLTAGE)
            EXPECT_EQ(device_status.battery_voltage, shadow.device_status.battery_voltage);
        if (device_status.presence_flags & IOT_PRESENCE_FLAG_LAST_CELLULAR_CONNECTED_TIMESTAMP)
            EXPECT_EQ(device_status.last_cellular_connected_timestamp, shadow.device_status.last_cellular_connected_timestamp);
        if (device_status.presence_flags & IOT_PRESENCE_FLAG_LAST_SAT_TX_TIMESTAMP)
            EXPECT_EQ(device_status.last_sat_tx_timestamp, shadow.device_status.last_sat_tx_timestamp);
        if (device_status.presence_flags & IOT_PRESENCE_FLAG_NEXT_SAT_TX_TIMESTAMP)
            EXPECT_EQ(device_status.next_sat_tx_timestamp, shadow.device_status.next_sat_tx_timestamp);
        if (device_status.presence_flags & IOT_PRESENCE_FLAG_CONFIGURATION_VERSION)
            EXPECT_EQ(device_status.configuration_version, shadow.device_status.configuration_version);
        if (device_status.presence_flags & IOT_PRESENCE_FLAG_FIRMWARE_VERSION)
            EXPECT_EQ(device_status.firmware_version, shadow.device_status.firmware_version);
    }
}