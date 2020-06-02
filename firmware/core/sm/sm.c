/* sm.c - State machine handling code
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

#include <stddef.h> // include NULL
#include <limits.h> // INT_MAX
#include "sm.h"

#define FUNCTION_NOT_SET (INT_MAX)

/**
 * @brief      Initialise the state machine
 *
 * @param[out] handle       The state handler
 * @param[in]  state_table  The state table
 */
void sm_init(sm_handle_t * handle, sm_state_func_t * state_table)
{
    handle->state_function_lookup_table = state_table;
    handle->last_state = FUNCTION_NOT_SET;
    handle->current_state = FUNCTION_NOT_SET;
    handle->next_state = FUNCTION_NOT_SET;
    handle->first_time_running = true;
}

/**
 * @brief      Execute one tick of the state machine
 *
 * @param      handle  The state handler
 */
void sm_tick(sm_handle_t * handle)
{
    handle->state_function_lookup_table[handle->current_state](handle);

    handle->first_time_running = false;

    if (handle->current_state != handle->next_state)
    {
        handle->first_time_running = true;
        handle->last_state = handle->current_state;
        handle->current_state = handle->next_state;
    }
};

/**
 * @brief      Have we just transitioned to this state from a different state?
 *
 * @param[in]  handle  The state handler
 *
 * @return     true/false
 */
bool sm_is_first_entry(const sm_handle_t * handle)
{
    return handle->first_time_running;
}


/**
 * @brief      Is this the last time this state will be run?
 *
 * @param[in]  handle  The state handler
 *
 * @return     true/false
 */
bool sm_is_last_entry(const sm_handle_t * handle)
{
    return sm_get_next_state(handle) != sm_get_current_state(handle);
}

/**
 * @brief      Set the current state to be execute
 *
 * @warning    This should only be used in unit tests
 *
 * @param      handle  The state handler
 * @param[in]  state   The next state
 */
void sm_set_current_state(sm_handle_t * handle, int state)
{
    if (handle->current_state != state)
        handle->first_time_running = true;

    handle->last_state = handle->current_state;
    handle->current_state = state;
    handle->next_state = state;
}

/**
 * @brief      Set the next state to be execute
 *
 * @param      handle  The state handler
 * @param[in]  state   The next state
 */
void sm_set_next_state(sm_handle_t * handle, int state)
{
    if (FUNCTION_NOT_SET == handle->current_state)
        handle->current_state = state;

    handle->next_state = state;
}

/**
 * @brief      Returns the state that preceded this one
 *
 * @param[in]  handle  The state handle
 *
 * @return     The state that preceded this one
 */
int sm_get_last_state(const sm_handle_t * handle)
{
    return handle->last_state;
}

/**
 * @brief      Returns the current state
 *
 * @param[in]  handle  The state handle
 *
 * @return     The current state
 */
int sm_get_current_state(const sm_handle_t * handle)
{
    return handle->current_state;
}

/**
 * @brief      Returns the state that will be executed next
 *
 * @param[in]  handle  The state handle
 *
 * @return     The state that will be executed next
 */
int sm_get_next_state(const sm_handle_t * handle)
{
    return handle->next_state;
}