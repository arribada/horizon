// Buffer.cpp - Buffer unit tests
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
#include <assert.h>
#include <stdint.h>
#include "unity.h"
#include "buffer.h"
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

// This is used as a memory heap for all the buffers we create
static char heap[1024];

class BufferTest : public ::testing::Test {

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
 };

// Feature: Initialize buffer
// Scenario:

TEST_F(BufferTest, CircularInitialize)
{
    buffer_t buffer;

    buffer_init(&buffer, (uintptr_t)heap, 4);
    buffer_init_policy(circular, &buffer, (uintptr_t)heap, 4);
}

TEST_F(BufferTest, CircularWriteReadBasic)
{
    uintptr_t addr;
    buffer_t buffer;
    unsigned int avail;

    buffer_init(&buffer, (uintptr_t)heap, 4);
    avail = buffer_write(&buffer, &addr);
    EXPECT_EQ((uint32_t)4, avail);
    EXPECT_EQ((uintptr_t)heap, addr);
    avail = buffer_read(&buffer, &addr);
    EXPECT_EQ((uint32_t)0, avail);
    buffer_write_advance(&buffer, 2);
    avail = buffer_write(&buffer, &addr);
    EXPECT_EQ((uint32_t)2, avail);
    EXPECT_EQ(addr, (uintptr_t)(heap+2));
    avail = buffer_read(&buffer, &addr);
    EXPECT_EQ((uint32_t)2, avail);
}

TEST_F(BufferTest, CircularWriteReadOverflow)
{
    uintptr_t addr;
    buffer_t buffer;
    unsigned int avail;

    buffer_init(&buffer, (uintptr_t)heap, 4);
    // Fill buffer without overflowing
    buffer_write_advance(&buffer, 4);
    EXPECT_EQ((uint32_t)4, buffer_occupancy(&buffer));
    EXPECT_EQ((uint32_t)0, buffer_free(&buffer));
    EXPECT_EQ((uint32_t)0, buffer_overflows(&buffer));
    // Check we have 4 bytes available
    avail = buffer_read(&buffer, &addr);
    EXPECT_EQ((uint32_t)4, avail);
    // Advance by 1 more byte to cause overflow
    // of first byte
    buffer_write_advance(&buffer, 1);
    // Occupancy should still be 4
    EXPECT_EQ((uint32_t)4, buffer_occupancy(&buffer));
    EXPECT_EQ((uint32_t)0, buffer_free(&buffer));
    // Overflow counter should be set
    EXPECT_EQ((uint32_t)1, buffer_overflows(&buffer));
    // We can read 3 contigous bytes in one pass to end of buffer
    // and the read pointer should be offset 1 from start
    avail = buffer_read(&buffer, &addr);
    EXPECT_EQ((uintptr_t)heap+1, addr);
    EXPECT_EQ((uint32_t)3, avail);
    buffer_read_advance(&buffer, 3);
    EXPECT_EQ((uint32_t)1, buffer_occupancy(&buffer));
    EXPECT_EQ((uint32_t)3, buffer_free(&buffer));
    // Now we can read the final byte in the buffer in 2nd pass
    avail = buffer_read(&buffer, &addr);
    EXPECT_EQ((uint32_t)1, avail);
    buffer_read_advance(&buffer, 1);
    // Buffer should now be empty
    EXPECT_EQ((uint32_t)0, buffer_occupancy(&buffer));
    EXPECT_EQ((uint32_t)4, buffer_free(&buffer));
}

TEST_F(BufferTest, CircularWriteMultiOverflow)
{
    buffer_t buffer;

    buffer_init(&buffer, (uintptr_t)heap, 4);
    buffer_write_advance(&buffer, 5);
    buffer_write_advance(&buffer, 4);
    buffer_write_advance(&buffer, 4);
    EXPECT_EQ((uint32_t)3, buffer_overflows(&buffer));
}

TEST_F(BufferTest, CircularReset)
{
    buffer_t buffer;

    buffer_init(&buffer, (uintptr_t)heap, 4);
    buffer_write_advance(&buffer, 5);
    buffer_write_advance(&buffer, 4);
    buffer_write_advance(&buffer, 4);
    ASSERT_EQ((uint32_t)3, buffer_overflows(&buffer));
    ASSERT_EQ((uint32_t)4, buffer_occupancy(&buffer));
    buffer_reset(&buffer);
    EXPECT_EQ(0, buffer_overflows(&buffer));
    EXPECT_EQ(0, buffer_occupancy(&buffer));
}

TEST_F(BufferTest, PoolWriteReadBasic)
{
    uintptr_t addr;
    buffer_t buffer;
    unsigned int avail;

    buffer_init_policy(pool, &buffer, (uintptr_t)heap, 16, 4);
    avail = buffer_write(&buffer, &addr);
    EXPECT_EQ((uint32_t)4, avail);
    EXPECT_EQ((uintptr_t)heap, addr);
    avail = buffer_read(&buffer, &addr);
    EXPECT_EQ((uint32_t)0, avail);
    buffer_write_advance(&buffer, 2);
    avail = buffer_write(&buffer, &addr);
    EXPECT_EQ((uint32_t)4, avail);
    EXPECT_EQ((uintptr_t)(&heap[4]), addr);
    avail = buffer_read(&buffer, &addr);
    EXPECT_EQ((uint32_t)2, avail);
    EXPECT_EQ((uintptr_t)heap, addr);
}

TEST_F(BufferTest, PoolWriteReadOverflow)
{
    buffer_t buffer;
    uintptr_t addr;

    buffer_init_policy(pool, &buffer, (uintptr_t)heap, 16, 4);
    EXPECT_EQ((uint32_t)0, buffer_occupancy(&buffer));
    buffer_write_advance(&buffer, 4);
    buffer_write_advance(&buffer, 3);
    buffer_write_advance(&buffer, 2);
    buffer_write_advance(&buffer, 1);
    EXPECT_EQ((uint32_t)0, buffer_free(&buffer));
    EXPECT_EQ((uint32_t)4, buffer_occupancy(&buffer));
    buffer_write_advance(&buffer, 2);
    EXPECT_EQ((uint32_t)1, buffer_overflows(&buffer));
    EXPECT_EQ((uint32_t)4, buffer_occupancy(&buffer));
    EXPECT_EQ((uint32_t)3, buffer_read(&buffer, &addr));
    buffer_read_advance(&buffer, 2);
    EXPECT_EQ((uint32_t)2, buffer_read(&buffer, &addr));
    buffer_read_advance(&buffer, 2);
    EXPECT_EQ((uint32_t)1, buffer_read(&buffer, &addr));
    buffer_read_advance(&buffer, 1);
    EXPECT_EQ((uint32_t)2, buffer_read(&buffer, &addr));
    buffer_read_advance(&buffer, 2);
    EXPECT_EQ((uint32_t)0, buffer_occupancy(&buffer));
}

TEST_F(BufferTest, PoolWriteMultiOverflow)
{
    buffer_t buffer;
    uintptr_t addr;

    buffer_init_policy(pool, &buffer, (uintptr_t)heap, 16, 2);
    EXPECT_EQ((uint32_t)0, buffer_occupancy(&buffer));
    buffer_write_advance(&buffer, 4);
    buffer_write_advance(&buffer, 3);
    buffer_write_advance(&buffer, 2);
    buffer_write_advance(&buffer, 1);
    EXPECT_EQ((uint32_t)2, buffer_overflows(&buffer));
    EXPECT_EQ((uint32_t)2, buffer_occupancy(&buffer));
    EXPECT_EQ((uint32_t)2, buffer_read(&buffer, &addr));
    buffer_read_advance(&buffer, 2);
    EXPECT_EQ((uint32_t)1, buffer_read(&buffer, &addr));
    buffer_read_advance(&buffer, 1);
    EXPECT_EQ((uint32_t)0, buffer_occupancy(&buffer));
}

TEST_F(BufferTest, PoolReset)
{
    buffer_t buffer;

    buffer_init_policy(pool, &buffer, (uintptr_t)heap, 16, 2);
    buffer_write_advance(&buffer, 4);
    buffer_write_advance(&buffer, 3);
    buffer_write_advance(&buffer, 2);
    buffer_write_advance(&buffer, 1);
    ASSERT_EQ((uint32_t)2, buffer_overflows(&buffer));
    ASSERT_EQ((uint32_t)2, buffer_occupancy(&buffer));
    buffer_reset(&buffer);
    EXPECT_EQ(0, buffer_overflows(&buffer));
    EXPECT_EQ(0, buffer_occupancy(&buffer));
}

