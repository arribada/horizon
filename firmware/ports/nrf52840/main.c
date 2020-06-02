/* Copyright (C) 2019 Arribada
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

#include "cexception.h"
#include "syshal_pmu.h"
#include "nrf_sdh.h"
#include "sm_main.h"
#include "sm.h"

int main(void)
{
    sm_handle_t state_handle;

    // Start the softdevice
    nrf_sdh_enable_request();

    sm_init(&state_handle, sm_main_states);
    sm_set_next_state(&state_handle, SM_MAIN_BOOT);

    while (1)
    {
        CEXCEPTION_T e = CEXCEPTION_NONE;

        Try
        {
            sm_tick(&state_handle);

            // We only kick the IWDG from here -- this assumes that
            // the state machine tick is called sufficiently often
            syshal_pmu_kick_watchdog();
        }
        Catch (e)
        {
            sm_main_exception_handler(e);
        }

        main_thread_checker = MAIN_THREAD_RUNNING_VALUE;
    }
}
