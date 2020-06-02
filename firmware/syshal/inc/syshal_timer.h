/* syshal_timer.h - HAL for MCU timers
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

#ifndef _SYSHAL_TIMER_H_
#define _SYSHAL_TIMER_H_

#include <stdint.h>

#define SYSHAL_TIMER_RUNNING                     ( 1)
#define SYSHAL_TIMER_NOT_RUNNING                 ( 0)
#define SYSHAL_TIMER_NO_ERROR                    ( 0)
#define SYSHAL_TIMER_ERROR_NO_FREE_TIMER         (-1)
#define SYSHAL_TIMER_ERROR_INVALID_TIMER_HANDLE  (-2)
#define SYSHAL_TIMER_ERROR_INVALID_TIME          (-3)

#define SYSHAL_TIMER_NUMBER_OF_TIMERS  (24)

typedef enum
{
    one_shot,  // Only trigger the timer once
    periodic   // Continuely reset the timer everytime it is triggered
} syshal_timer_mode_t;

typedef uint32_t timer_handle_t;

int syshal_timer_init(timer_handle_t *handle, void (*callback)(void));
int syshal_timer_term(timer_handle_t handle);
int syshal_timer_set(timer_handle_t handle, syshal_timer_mode_t mode, uint32_t seconds);
int syshal_timer_set_ms(timer_handle_t handle, syshal_timer_mode_t mode, uint32_t milliseconds);
int syshal_timer_reset(timer_handle_t handle);
int syshal_timer_running(timer_handle_t handle);
int syshal_timer_cancel(timer_handle_t handle);
int syshal_timer_cancel_all(void);
int syshal_timer_recalculate_next_alarm(void);

void syshal_timer_tick(void);

#endif /* _SYSHAL_TIMER_H_ */