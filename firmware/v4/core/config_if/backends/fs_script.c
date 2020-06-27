/* fs_script.c - A configuration interface backend for executing scripts from
 * the fs system
 *
 * Copyright (C) 2019 Arribada
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "fs_script.h"
#include "cmd.h"
#include "debug.h"

static fs_t file_system;
static uint8_t script_file_id;
static fs_handle_t file_handle;
static bool file_open;
static uint32_t last_pos_in_file;

static inline void close_file(void)
{
    if (!file_open)
        return;

    fs_close(file_handle);
    file_open = false;
    fs_script_event_t event;
    event.id = FS_SCRIPT_FILE_CLOSED;
    fs_script_event_handler(&event);
}

int fs_script_init(fs_t fs, uint8_t file_id)
{
    int ret = fs_open(fs, &file_handle, file_id, FS_MODE_READONLY, NULL);
    if (FS_ERROR_FILE_NOT_FOUND == ret)
        return FS_SCRIPT_ERROR_FILE_NOT_FOUND;
    else if (ret)
        return FS_SCRIPT_ERROR_FS;

    file_system = fs;
    script_file_id = file_id;
    last_pos_in_file = 0;
    file_open = true;

    fs_script_event_t event;
    event.id = FS_SCRIPT_FILE_OPENED;
    fs_script_event_handler(&event);

    return FS_SCRIPT_NO_ERROR;
}

int fs_script_term(void)
{
    close_file();

    return FS_SCRIPT_NO_ERROR;
}

int fs_script_send(uint8_t * buffer, uint32_t length)
{
    // Ignore any sent messages
    fs_script_event_t event;
    event.id = FS_SCRIPT_SEND_COMPLETE;
    event.send.buffer = buffer;
    event.send.size = length;
    fs_script_event_handler(&event);

    return FS_SCRIPT_NO_ERROR;
}

int fs_script_receive(uint8_t * buffer, uint32_t length)
{
    uint32_t total_bytes_read;
    uint32_t bytes_read;
    size_t expected_size;
    int ret;
    cmd_t * cmd;

    if (!file_open)
        return FS_SCRIPT_ERROR_FILE_NOT_OPEN;

    if (length < CMD_SIZE_HDR)
        return FS_SCRIPT_ERROR_BUFFER_TOO_SMALL;

    // Read the command header only
    ret = fs_read(file_handle, buffer, CMD_SIZE_HDR, &bytes_read);
    if (FS_ERROR_END_OF_FILE == ret ||
        bytes_read < CMD_SIZE_HDR)
    {
        close_file();
        return FS_SCRIPT_ERROR_END_OF_FILE;
    }
    else if (ret)
    {
        return FS_SCRIPT_ERROR_FS;
    }

    total_bytes_read = bytes_read;

    // Check the command header is correct
    cmd = (cmd_t *) buffer;
    if (CMD_SYNCWORD != cmd->hdr.sync)
    {
        close_file();
        return FS_SCRIPT_ERROR_INVALID_FILE_FORMAT;
    }

    ret = cmd_get_size(cmd->hdr.cmd, &expected_size);
    if (ret)
    {
        close_file();
        return FS_SCRIPT_ERROR_INVALID_FILE_FORMAT;
    }

    if (length < expected_size)
    {
        // Not enough room to read the data this time around but we have already progressed our
        // read pointer so close the file and return to our original location
        fs_close(file_handle);
        fs_open(&file_system, &file_handle, script_file_id, FS_MODE_READONLY, NULL);
        fs_seek(file_handle, last_pos_in_file);
        return FS_SCRIPT_ERROR_BUFFER_TOO_SMALL;
    }

    // Read the remainder of the command payload
    size_t bytes_to_read = expected_size - CMD_SIZE_HDR;
    if (bytes_to_read)
    {
        ret = fs_read(file_handle, &buffer[CMD_SIZE_HDR], bytes_to_read, &bytes_read);
        if (FS_ERROR_END_OF_FILE == ret ||
            bytes_read < bytes_to_read)
        {
            close_file();
            return FS_SCRIPT_ERROR_END_OF_FILE;
        }
        else if (ret)
        {
            close_file(); // This is unrecoverable
            return FS_SCRIPT_ERROR_FS;
        }

        total_bytes_read += bytes_read;
    }

    last_pos_in_file += total_bytes_read;

    // Generate a receive complete event
    fs_script_event_t event;
    event.id = FS_SCRIPT_RECEIVE_COMPLETE;
    event.receive.buffer = buffer;
    event.receive.size = total_bytes_read;
    fs_script_event_handler(&event);

    return FS_SCRIPT_NO_ERROR;
}

int fs_script_receive_byte_stream(uint8_t * buffer, uint32_t length)
{
    uint32_t bytes_read;
    int ret;

    if (!file_open)
        return FS_SCRIPT_ERROR_FILE_NOT_OPEN;

    if (!length)
        return FS_SCRIPT_NO_ERROR;

    ret = fs_read(file_handle, buffer, length, &bytes_read);
    if (FS_ERROR_END_OF_FILE == ret)
    {
        close_file();
        return FS_SCRIPT_ERROR_END_OF_FILE;
    }
    else if (ret)
    {
        close_file(); // This is unrecoverable
        return FS_SCRIPT_ERROR_FS;
    }

    // Generate a receive complete event
    fs_script_event_t event;
    event.id = FS_SCRIPT_RECEIVE_COMPLETE;
    event.receive.buffer = buffer;
    event.receive.size = bytes_read;
    fs_script_event_handler(&event);

    return FS_SCRIPT_NO_ERROR;
}

__attribute__((weak)) void fs_script_event_handler(fs_script_event_t * event)
{
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);
}