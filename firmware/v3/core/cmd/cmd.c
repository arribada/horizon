/* cmd.c - Command protocol functions
 *
 * Copyright (C) 2019 Arribada
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "cmd.h"

#define CMD_EXPAND_AS_SWITCH_LOOKUP(id, struct, name) case id: *size = CMD_SIZE(struct); break;

/**
 * @brief      Gets the size of the given command
 *
 * @param[in]  command  The command
 * @param[out] size     The size
 *
 * @return     CMD_NO_ERROR on success
 * @return     CMD_ERROR_INVALID_PARAMETER if the given command is not recognised
 */
int cmd_get_size(cmd_id_t command, size_t * size)
{
    switch (command)
    {
            // Create a lookup for each command id to retrieve its size
            CMD_VALUES(CMD_EXPAND_AS_SWITCH_LOOKUP)
        default:
            return CMD_ERROR_INVALID_PARAMETER;
            break;
    }

    return CMD_NO_ERROR;
}

/**
 * @brief      Check if the given command and size are correct
 *
 * @param[in]  command  The command
 * @param[in]  size     The expected size
 *
 * @return     true  if size matches the expected command
 * @return     false if size is incorrect
 */
bool cmd_check_size(cmd_id_t command, size_t size)
{
    size_t cmd_size;
    int ret = cmd_get_size(command, &cmd_size);

    if (ret || cmd_size != size)
        return false;
    else
        return true;
}
