// test_json.cpp - Json parser unit tests
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
#include "json.h"
}

#include <gtest/gtest.h>
#include <cstdarg>
#include <chrono>
#include <random>

char json_full[2048];
char json_mini[2048];
char json_original[2048];

class JSONTest : public ::testing::Test
{
    virtual void SetUp()
    {
        FILE * fp;

        fp = fopen("../json/test_device_status.json", "r");
        ASSERT_NE(nullptr, fp);
        fread(json_full, 1, sizeof(json_full), fp);
        fclose(fp);

        fp = fopen("../json/test_minified.json", "r");
        ASSERT_NE(nullptr, fp);
        fread(json_mini, 1, sizeof(json_mini), fp);
        fclose(fp);

        fp = fopen("../json/test_original.json", "r");
        ASSERT_NE(nullptr, fp);
        fread(json_original, 1, sizeof(json_original), fp);
        fclose(fp);

        srand(time(NULL));
    }

    virtual void TearDown()
    {

    }

public:

    void setup_json_test(const char ** result, const char * json_str, size_t json_len, size_t * len)
    {
        *result = json_parse("state", 0, json_str, json_len, len);
        ASSERT_NE(nullptr, result);

        *result = json_parse("desired", 0, *result, *len, len);
        ASSERT_NE(nullptr, result);

        *result = json_parse("device_status", 0, *result, *len, len);
        ASSERT_NE(nullptr, result);
    }

};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// original tests /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST_F(JSONTest, jsonOriginalTest)
{
    const char * ret;
    size_t len;

    ret = json_parse("test", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(0, strncmp("value", ret, len));
}

TEST_F(JSONTest, jsonOriginalFoo)
{
    const char * ret;
    size_t len;

    ret = json_parse("foo", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(13, len);
    EXPECT_EQ(0, strncmp("b\\\"a and \\\\ r", ret, len));
}

TEST_F(JSONTest, jsonOriginalArray)
{
    const char * array;
    const char * member;
    size_t len_array, len_member;

    array = json_parse("array", 0, json_original, sizeof(json_original), &len_array);
    ASSERT_NE(nullptr, array);
    EXPECT_EQ(0, strncmp("[1,2,\"three\",4]", array, len_array));

    member = json_parse(NULL, 1, array, len_array, &len_member);
    EXPECT_EQ(0, strncmp("2", member, len_member));

    member = json_parse(NULL, 0, array, len_array, &len_member);
    EXPECT_EQ(0, strncmp("1", member, len_member));

    member = json_parse(NULL, 3, array, len_array, &len_member);
    EXPECT_EQ(0, strncmp("4", member, len_member));

    member = json_parse(NULL, 2, array, len_array, &len_member);
    EXPECT_EQ(0, strncmp("three", member, len_member));
}

TEST_F(JSONTest, jsonOriginalBaz)
{
    const char * ret;
    size_t len;

    ret = json_parse("baz", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(0, strncmp("{\"a\":\"b\"}", ret, len));
}

TEST_F(JSONTest, jsonOriginalNum)
{
    const char * ret;
    size_t len;

    ret = json_parse("num", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(0, strncmp("123.45", ret, len));
}

TEST_F(JSONTest, jsonOriginalBool)
{
    const char * ret;
    size_t len;

    ret = json_parse("bool", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(0, strncmp("true", ret, len));
}

TEST_F(JSONTest, jsonOriginalUtf8)
{
    const char * ret;
    size_t len;

    ret = json_parse("utf8", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(0, strncmp("$¢€𤪤", ret, len));
}

TEST_F(JSONTest, jsonOriginalKey)
{
    const char * ret;
    size_t len;

    ret = json_parse("key", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(0, strncmp("value\\n\\\"newline\\\"", ret, len));
}

TEST_F(JSONTest, jsonOriginalObj)
{
    const char * ret;
    size_t len;

    ret = json_parse("obj", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(0, strncmp("{\"true\":true}", ret, len));
}

TEST_F(JSONTest, jsonOriginalValue)
{
    const char * ret;
    size_t len;

    ret = json_parse("value", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(0, strncmp("real", ret, len));
}

TEST_F(JSONTest, jsonOriginalName)
{
    const char * ret;
    size_t len;

    ret = json_parse("name", 0, json_original, sizeof(json_original), &len);
    ASSERT_NE(nullptr, ret);
    EXPECT_EQ(0, strncmp("value", ret, len));
}

TEST_F(JSONTest, jsonOriginalNotFound)
{
    size_t len;

    EXPECT_EQ(NULL, json_parse("x", 0, "{}", 2, &len));
}

TEST_F(JSONTest, jsonOriginalParseError)
{
    size_t len;

    EXPECT_EQ(NULL, json_parse("x", 0, "{", 1, &len));
    EXPECT_EQ(1, len);
}

TEST_F(JSONTest, jsonOriginalNotFoundNull)
{
    size_t len;

    EXPECT_EQ(NULL, json_parse("x", 0, "{\0}", 3, &len));
    EXPECT_EQ(1, len);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// indicative tests ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST_F(JSONTest, jsonFull_last_log_file_read_pos)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_full, sizeof(json_full), &len);

    ret = json_parse("last_log_file_read_pos", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("1421", ret, len));
}

TEST_F(JSONTest, jsonFull_last_gps_location)
{
    const char * ret;
    const char * value;
    size_t len, len_val;

    setup_json_test(&ret, json_full, sizeof(json_full), &len);

    ret = json_parse("last_gps_location", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    value = json_parse("longitude", 0, ret, len, &len_val);
    ASSERT_NE(nullptr, value);
    EXPECT_EQ(0, strncmp("-12.3", value, len_val));

    value = json_parse("latitude", 0, ret, len, &len_val);
    ASSERT_NE(nullptr, value);
    EXPECT_EQ(0, strncmp("23.43233", value, len_val));

    value = json_parse("timestamp", 0, ret, len, &len_val);
    ASSERT_NE(nullptr, value);
    EXPECT_EQ(0, strncmp("5635123", value, len_val));
}

TEST_F(JSONTest, jsonFull_last_cellular_connected_timestamp)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_full, sizeof(json_full), &len);

    ret = json_parse("last_cellular_connected_timestamp", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("232", ret, len));
}

TEST_F(JSONTest, jsonFull_last_sat_tx_timestamp)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_full, sizeof(json_full), &len);

    ret = json_parse("last_sat_tx_timestamp", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("234443", ret, len));
}

TEST_F(JSONTest, jsonFull_next_sat_tx_timestamp)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_full, sizeof(json_full), &len);

    ret = json_parse("next_sat_tx_timestamp", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("12332", ret, len));
}

TEST_F(JSONTest, jsonFull_battery_level)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_full, sizeof(json_full), &len);

    ret = json_parse("battery_level", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("40", ret, len));
}

TEST_F(JSONTest, jsonFull_battery_voltage)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_full, sizeof(json_full), &len);

    ret = json_parse("battery_voltage", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("23", ret, len));
}

TEST_F(JSONTest, jsonFull_configuration_version)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_full, sizeof(json_full), &len);

    ret = json_parse("configuration_version", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("4", ret, len));
}

TEST_F(JSONTest, jsonFull_firmware_version)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_full, sizeof(json_full), &len);

    ret = json_parse("firmware_version", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("3", ret, len));
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// minified tests /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST_F(JSONTest, jsonMini_last_log_file_read_pos)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_mini, sizeof(json_mini), &len);

    ret = json_parse("last_log_file_read_pos", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("1421", ret, len));
}

TEST_F(JSONTest, jsonMini_last_gps_location)
{
    const char * ret;
    const char * value;
    size_t len, len_val;

    setup_json_test(&ret, json_mini, sizeof(json_mini), &len);

    ret = json_parse("last_gps_location", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    value = json_parse("longitude", 0, ret, len, &len_val);
    ASSERT_NE(nullptr, value);
    EXPECT_EQ(0, strncmp("-12.3", value, len_val));

    value = json_parse("latitude", 0, ret, len, &len_val);
    ASSERT_NE(nullptr, value);
    EXPECT_EQ(0, strncmp("23.43233", value, len_val));

    value = json_parse("timestamp", 0, ret, len, &len_val);
    ASSERT_NE(nullptr, value);
    EXPECT_EQ(0, strncmp("5635123", value, len_val));
}

TEST_F(JSONTest, jsonMini_last_cellular_connected_timestamp)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_mini, sizeof(json_mini), &len);

    ret = json_parse("last_cellular_connected_timestamp", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("232", ret, len));
}

TEST_F(JSONTest, jsonMini_last_sat_tx_timestamp)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_mini, sizeof(json_mini), &len);

    ret = json_parse("last_sat_tx_timestamp", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("234443", ret, len));
}

TEST_F(JSONTest, jsonMini_next_sat_tx_timestamp)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_mini, sizeof(json_mini), &len);

    ret = json_parse("next_sat_tx_timestamp", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("12332", ret, len));
}

TEST_F(JSONTest, jsonMini_battery_level)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_mini, sizeof(json_mini), &len);

    ret = json_parse("battery_level", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("40", ret, len));
}

TEST_F(JSONTest, jsonMini_battery_voltage)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_mini, sizeof(json_mini), &len);

    ret = json_parse("battery_voltage", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("23", ret, len));
}

TEST_F(JSONTest, jsonMini_configuration_version)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_mini, sizeof(json_mini), &len);

    ret = json_parse("configuration_version", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("4", ret, len));
}

TEST_F(JSONTest, jsonMini_firmware_version)
{
    const char * ret;
    size_t len;

    setup_json_test(&ret, json_mini, sizeof(json_mini), &len);

    ret = json_parse("firmware_version", 0, ret, len, &len);
    ASSERT_NE(nullptr, ret);

    EXPECT_EQ(0, strncmp("3", ret, len));
}