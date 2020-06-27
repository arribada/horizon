/* logging.c - Logging functions
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

#include "logging.h"

#define LOGGING_EXPAND_AS_SWITCH_LOOKUP(id, name, struct) case name: *size = sizeof(struct); break;

int logging_tag_size(uint8_t tag_id, size_t * size)
{
    switch (tag_id)
    {
        // Create a lookup for each logging value to retrieve its size
        LOGGING_VALUES(LOGGING_EXPAND_AS_SWITCH_LOOKUP)
        default:
            return LOGGING_ERROR_INVALID_TAG;
            break;
    }

    return LOGGING_NO_ERROR;
}
