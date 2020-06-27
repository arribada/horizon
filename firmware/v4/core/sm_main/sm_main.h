/* sm_main.h - State machine handling code
 *
 * Copyright (C) 2018 Arribada
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

#ifndef _SM_MAIN_H_
#define _SM_MAIN_H_

#include "sm.h"
#include "fs.h"
#include "cexception.h"

#define FILE_ID_CONF_PRIMARY         (0) // The File ID of the first configuration in a ping-pong arrangement
#define FILE_ID_CONF_SECONDARY       (1) // The File ID of the second configuration in a ping-pong arrangement
#define FILE_ID_CONF_COMMANDS        (3) // A list of configuration interfaced commands to be executed
#define FILE_ID_APP_FIRM_IMAGE       (4) // Application firmware update image
#define FILE_ID_LOG                  (5) // Logging data file
#define FILE_ID_SARA_U270_FIRM_IMAGE (6) // Sara U270 firmware image
#define FILE_ID_ARTIC_FIRM_IMAGE     (7) // Artic module firmware image

#define MAIN_THREAD_INVALIDER_VALUE (0xDEADBEAF)
#define MAIN_THREAD_RUNNING_VALUE   (0xCAFED00D)

extern sm_state_func_t sm_main_states[]; // State function lookup table is populate in sm_main.c
extern fs_t file_system;
extern volatile uint32_t main_thread_checker;

typedef enum
{
    SM_MAIN_BOOT,
    SM_MAIN_ERROR,
    SM_MAIN_BATTERY_CHARGING,
    SM_MAIN_BATTERY_LEVEL_LOW,
    SM_MAIN_LOG_FILE_FULL,
    SM_MAIN_PROVISIONING_NEEDED,
    SM_MAIN_PROVISIONING,
    SM_MAIN_OPERATIONAL,
    SM_MAIN_DEACTIVATED,
} sm_main_states_t;

void sm_main_exception_handler(CEXCEPTION_T e);

#ifdef GTEST
extern fs_handle_t sm_main_file_handle;
#endif

#endif /* _SM_MAIN_H_ */
