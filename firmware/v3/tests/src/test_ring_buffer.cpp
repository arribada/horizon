// test_ring_buffer.cpp - Ringbuffer unit tests
//
// Copyright (C) 2018 Arribada
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
#include <stdint.h>
#include "ring_buffer.h"
#include <stdlib.h>
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include <list>
#include <vector>

using std::list;
using std::vector;

#define BUF_SIZE 100

static ring_buffer_t ring_buffer;
static uint8_t buffer[BUF_SIZE + 1]; // 1 space is unused for empty/full flag

class Ring_BufferTest : public ::testing::Test
{

    virtual void SetUp()
    {
        rb_init(&ring_buffer, BUF_SIZE, &buffer[0]); // Setup ring buffer
        EXPECT_EQ(BUF_SIZE, rb_capacity(&ring_buffer));
        EXPECT_EQ(BUF_SIZE, rb_free(&ring_buffer));
    }

    virtual void TearDown()
    {

    }

public:

    void GenerateTestData(uint8_t * data, uint32_t size)
    {
        for (uint32_t i = 0; i < size; ++i)
            data[i] = i + 1; // Add one so no value is 0
    }

};

TEST_F(Ring_BufferTest, BufferEmpty)
{
    EXPECT_EQ(0, rb_occupancy(&ring_buffer));
    EXPECT_TRUE(rb_is_empty(&ring_buffer));
    EXPECT_FALSE(rb_is_full(&ring_buffer));
}

TEST_F(Ring_BufferTest, WriteOne)
{
    uint8_t testData[1];

    GenerateTestData(testData, 1);

    rb_insert(&ring_buffer, testData[0]);

    EXPECT_EQ(1, rb_occupancy(&ring_buffer));
    EXPECT_EQ(BUF_SIZE - 1, rb_free(&ring_buffer));
    EXPECT_FALSE(rb_is_full(&ring_buffer));
    EXPECT_FALSE(rb_is_empty(&ring_buffer));

    // Read out and compare data
    EXPECT_EQ(testData[0], rb_safe_remove(&ring_buffer));
}

TEST_F(Ring_BufferTest, SafeRemoveNoContents)
{
    EXPECT_EQ(0, rb_occupancy(&ring_buffer));
    EXPECT_EQ(BUF_SIZE, rb_free(&ring_buffer));
    EXPECT_FALSE(rb_is_full(&ring_buffer));
    EXPECT_TRUE(rb_is_empty(&ring_buffer));

    // Read out and compare data
    EXPECT_EQ(-1, rb_safe_remove(&ring_buffer));
}


TEST_F(Ring_BufferTest, BufferFill)
{
    uint8_t testData[BUF_SIZE];

    GenerateTestData(testData, sizeof(testData));

    for (uint32_t i = 0; i < BUF_SIZE; ++i)
        rb_insert(&ring_buffer, testData[i]);

    EXPECT_EQ(BUF_SIZE, rb_occupancy(&ring_buffer));
    EXPECT_EQ(0, rb_free(&ring_buffer));
    EXPECT_TRUE(rb_is_full(&ring_buffer));
    EXPECT_FALSE(rb_is_empty(&ring_buffer));

    // Read out and compare data
    uint32_t difference = 0;
    for (uint32_t i = 0; i < BUF_SIZE; ++i)
        difference += testData[BUF_SIZE - 1 - i] - rb_remove(&ring_buffer);

    EXPECT_EQ(0, difference);
}

TEST_F(Ring_BufferTest, BufferFillDestructive)
{
    uint8_t testData[BUF_SIZE];
    GenerateTestData(testData, sizeof(testData));

    for (uint32_t i = 0; i < BUF_SIZE; ++i)
        rb_push_insert(&ring_buffer, testData[i]);

    EXPECT_EQ(BUF_SIZE, rb_occupancy(&ring_buffer));
    EXPECT_TRUE(rb_is_full(&ring_buffer));
    EXPECT_FALSE(rb_is_empty(&ring_buffer));

    // Read out and compare data
    uint32_t difference = 0;
    for (uint32_t i = 0; i < BUF_SIZE; ++i)
        difference += testData[BUF_SIZE - 1 - i] - rb_remove(&ring_buffer);

    EXPECT_EQ(0, difference);
}

TEST_F(Ring_BufferTest, BufferReset)
{
    uint8_t testData[BUF_SIZE];
    GenerateTestData(testData, sizeof(testData));

    EXPECT_EQ(0, rb_occupancy(&ring_buffer));
    EXPECT_TRUE(rb_is_empty(&ring_buffer));
    EXPECT_FALSE(rb_is_full(&ring_buffer));

    for (uint32_t i = 0; i < BUF_SIZE; ++i)
        rb_safe_insert(&ring_buffer, testData[i]);

    EXPECT_EQ(BUF_SIZE, rb_occupancy(&ring_buffer));
    EXPECT_TRUE(rb_is_full(&ring_buffer));
    EXPECT_FALSE(rb_is_empty(&ring_buffer));

    rb_reset(&ring_buffer);

    EXPECT_EQ(0, rb_occupancy(&ring_buffer));
    EXPECT_TRUE(rb_is_empty(&ring_buffer));
    EXPECT_FALSE(rb_is_full(&ring_buffer));
}

TEST_F(Ring_BufferTest, BufferOverfillByOneDestructive)
{
    uint8_t testData[BUF_SIZE + 1];
    GenerateTestData(testData, sizeof(testData));

    for (uint32_t i = 0; i < BUF_SIZE; ++i)
    {
        rb_push_insert(&ring_buffer, testData[i]);
    }

    // Add the extra one value
    EXPECT_TRUE(rb_is_full(&ring_buffer));
    rb_push_insert(&ring_buffer, testData[BUF_SIZE]);
    EXPECT_TRUE(rb_is_full(&ring_buffer));

    // Read out and compare data
    uint32_t difference = 0;
    for (uint32_t i = 0; i < BUF_SIZE ; ++i)
        difference += testData[BUF_SIZE - i] - rb_remove(&ring_buffer);

    EXPECT_EQ(0, difference);
}

TEST_F(Ring_BufferTest, BufferOverfillByOneSafe)
{
    uint8_t testData[BUF_SIZE + 1];
    GenerateTestData(testData, sizeof(testData));

    for (uint32_t i = 0; i < BUF_SIZE; ++i)
    {
        EXPECT_EQ(true, rb_safe_insert(&ring_buffer, testData[i]));
    }

    // Add the extra one value
    EXPECT_TRUE(rb_is_full(&ring_buffer));
    EXPECT_FALSE(rb_safe_insert(&ring_buffer, testData[BUF_SIZE]));
    EXPECT_TRUE(rb_is_full(&ring_buffer));

    // Read out and compare data
    uint32_t difference = 0;
    for (uint32_t i = 0; i < BUF_SIZE; ++i)
        difference += testData[BUF_SIZE - 1 - i] - rb_remove(&ring_buffer);

    EXPECT_EQ(0, difference);
}

TEST_F(Ring_BufferTest, Peek)
{
    uint8_t testData[BUF_SIZE];
    GenerateTestData(testData, sizeof(testData));

    for (uint32_t i = 0; i < BUF_SIZE; ++i)
    {
        rb_safe_insert(&ring_buffer, testData[i]);
        EXPECT_EQ(testData[0], rb_peek(&ring_buffer));
    }
}

TEST_F(Ring_BufferTest, PeekAt)
{
    uint8_t testData[BUF_SIZE];
    GenerateTestData(testData, sizeof(testData));

    // Fill the ring buffer
    for (uint32_t i = 0; i < BUF_SIZE; ++i)
        rb_safe_insert(&ring_buffer, testData[i]);

    // Peek at and compare data
    uint32_t difference = 0;
    for (uint32_t i = 0; i < BUF_SIZE; ++i)
        difference += testData[BUF_SIZE - 1 - i] - rb_peek_at(&ring_buffer, i);

    EXPECT_EQ(0, difference);
}

TEST_F(Ring_BufferTest, BufferOverfillByOneDestructivePeekAt)
{
    uint8_t testData[BUF_SIZE + 1];
    GenerateTestData(testData, sizeof(testData));

    for (uint32_t i = 0; i < BUF_SIZE; ++i)
    {
        rb_push_insert(&ring_buffer, testData[i]);
    }

    // Add the extra one value
    EXPECT_TRUE(rb_is_full(&ring_buffer));
    rb_push_insert(&ring_buffer, testData[BUF_SIZE]);
    EXPECT_TRUE(rb_is_full(&ring_buffer));

    // Read out and compare data
    uint32_t difference = 0;
    for (uint32_t i = 1; i < BUF_SIZE + 1; ++i)
    {
        difference += testData[i] - rb_peek_at(&ring_buffer, i);
    }

    EXPECT_EQ(0, difference);
}