/* sm.h - State machine handling code
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

#ifndef _SM_H_
#define _SM_H_

#include <stdbool.h>

struct state;
typedef void (*sm_state_func_t)(struct state *);
typedef struct state
{
    sm_state_func_t * state_function_lookup_table;

    int last_state;
    int current_state;
    int next_state;

    bool first_time_running;
} sm_handle_t;

void sm_init(sm_handle_t * handle, sm_state_func_t * state_table);
void sm_set_next_state(sm_handle_t * handle, int state);

bool sm_is_first_entry(const sm_handle_t * handle);
bool sm_is_last_entry(const sm_handle_t * handle);
int sm_get_last_state(const sm_handle_t * handle);
int sm_get_current_state(const sm_handle_t * handle);
int sm_get_next_state(const sm_handle_t * handle);

#ifdef GTEST
void sm_set_current_state(sm_handle_t * handle, int state);
#endif

void sm_tick(sm_handle_t * handle);

#endif /* _SM_H_ */