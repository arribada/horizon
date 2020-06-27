// test_fs_script.cpp - File system script configuration interface backend tests
//
// Copyright (C) 2019 Arribada
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <https://www.gnu.org/licenses/>.

extern "C" {
#include <assert.h>
#include <stdint.h>
#include "unity.h"
#include "buffer.h"
#include <stdlib.h>
#include "fs.h"
#include "fs_priv.h"
#include "Mocksyshal_flash.h"
#include "fs_script.h"
#include "cmd.h"
}

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <deque>

// syshal_flash
fs_t file_system;
#define FS_DEVICE 0
#define FILE_SCRIPT_ID 6
#define FLASH_SIZE          (FS_PRIV_SECTOR_SIZE * FS_PRIV_MAX_SECTORS)
char flash_ram[FLASH_SIZE];

int syshal_flash_init_callback(uint32_t drive, uint32_t device, int cmock_num_calls) {return SYSHAL_FLASH_NO_ERROR;}

int syshal_flash_read_callback(uint32_t device, void * dest, uint32_t address, uint32_t size, int cmock_num_calls)
{
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

std::deque<fs_script_event_t> fs_script_events;
void fs_script_event_handler(fs_script_event_t * event)
{
    fs_script_event_t local_evt = *event;
    fs_script_events.push_back(local_evt);
}

class FsScriptTest : public ::testing::Test
{
    virtual void SetUp()
    {
        Mocksyshal_flash_Init();
        syshal_flash_init_StubWithCallback(syshal_flash_init_callback);
        syshal_flash_read_StubWithCallback(syshal_flash_read_callback);
        syshal_flash_write_StubWithCallback(syshal_flash_write_callback);
        syshal_flash_erase_StubWithCallback(syshal_flash_erase_callback);

        memset(flash_ram, 0xFF, sizeof(flash_ram));
        EXPECT_EQ(FS_NO_ERROR, fs_init(FS_DEVICE));
        EXPECT_EQ(FS_NO_ERROR, fs_mount(FS_DEVICE, &file_system));
        EXPECT_EQ(FS_NO_ERROR, fs_format(file_system));

        fs_script_events.clear();

        srand(time(NULL));
    }

    virtual void TearDown()
    {
        EXPECT_EQ(FS_NO_ERROR, fs_term(FS_DEVICE));

        Mocksyshal_flash_Verify();
        Mocksyshal_flash_Destroy();
    }

public:

    std::vector<cmd_t> generate_script_file(uint32_t number_of_commands)
    {
        std::vector<cmd_t> commands;
        fs_handle_t file_system_handle;
        uint32_t bytes_written_fs;

        EXPECT_EQ(FS_NO_ERROR, fs_open(file_system, &file_system_handle, FILE_SCRIPT_ID, FS_MODE_CREATE, NULL));

        for (auto i = 0; i < number_of_commands; ++i)
        {
            cmd_id_t cmd_id;
            size_t command_size;
            do
            {
                cmd_id = (cmd_id_t) (rand() % UINT8_MAX);
            }
            while (CMD_ERROR_INVALID_PARAMETER == cmd_get_size(cmd_id, &command_size));

            cmd_t cmd;
            for (auto j = 0; j < CMD_MAX_SIZE; ++j)
                *((uint8_t *)&cmd) = rand();

            cmd.hdr.sync = CMD_SYNCWORD;
            cmd.hdr.cmd = cmd_id;
            commands.push_back(cmd);

            EXPECT_EQ(FS_NO_ERROR, fs_write(file_system_handle, &cmd, command_size, &bytes_written_fs));
        }

        EXPECT_EQ(FS_NO_ERROR, fs_close(file_system_handle));

        return commands;
    }

    fs_script_event_t get_last_event()
    {
        EXPECT_LE(1, fs_script_events.size());
        fs_script_event_t event = fs_script_events.front();
        fs_script_events.pop_front();
        return event;
    }
};

TEST_F(FsScriptTest, NoScriptFile)
{
    EXPECT_EQ(FS_SCRIPT_ERROR_FILE_NOT_FOUND, fs_script_init(file_system, FILE_SCRIPT_ID));
}

TEST_F(FsScriptTest, ReadScriptFile)
{
    const uint32_t number_of_commands = 10;
    uint8_t buffer[CMD_MAX_SIZE];
    std::vector<cmd_t> commands = generate_script_file(number_of_commands);

    EXPECT_EQ(FS_SCRIPT_NO_ERROR, fs_script_init(file_system, FILE_SCRIPT_ID));
    EXPECT_EQ(FS_SCRIPT_FILE_OPENED, get_last_event().id);

    for (auto i = 0; i < number_of_commands; ++i)
    {
        EXPECT_EQ(FS_SCRIPT_NO_ERROR, fs_script_receive(buffer, CMD_MAX_SIZE));
        cmd_t * cmd = (cmd_t *) &buffer[0];
        size_t command_size;
        EXPECT_EQ(CMD_SYNCWORD, cmd->hdr.sync);
        EXPECT_EQ(CMD_NO_ERROR, cmd_get_size((cmd_id_t) cmd->hdr.cmd, &command_size));

        fs_script_event_t event = get_last_event();

        EXPECT_EQ(FS_SCRIPT_RECEIVE_COMPLETE, event.id);
        EXPECT_EQ(command_size, event.receive.size);
    }

    EXPECT_EQ(FS_SCRIPT_ERROR_END_OF_FILE, fs_script_receive(buffer, CMD_MAX_SIZE));
    EXPECT_EQ(FS_SCRIPT_FILE_CLOSED, get_last_event().id);
}

TEST_F(FsScriptTest, SendRandomData)
{
    generate_script_file(100);

    EXPECT_EQ(FS_SCRIPT_NO_ERROR, fs_script_init(file_system, FILE_SCRIPT_ID));
    EXPECT_EQ(FS_SCRIPT_FILE_OPENED, get_last_event().id);

    for (auto i = 0; i < 100; ++i)
    {
        uint8_t test_data[1024];

        for (auto j = 0; j < sizeof(test_data); ++j)
            test_data[j] = rand();

        EXPECT_EQ(FS_SCRIPT_NO_ERROR, fs_script_send(test_data, rand() % sizeof(test_data)));
        EXPECT_EQ(FS_SCRIPT_SEND_COMPLETE, get_last_event().id);
    }

    EXPECT_EQ(0, fs_script_events.size());
}