/* syshal_pmu.h - HAL for CPU power management
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

#ifndef _SYSHAL_PMU_H_
#define _SYSHAL_PMU_H_

#include <stdint.h>

typedef enum
{
	SLEEP_LIGHT,
	SLEEP_DEEP
} syshal_pmu_sleep_mode_t;

void syshal_pmu_init(void);
void syshal_pmu_sleep(syshal_pmu_sleep_mode_t mode);
void syshal_pmu_reset(void);
uint32_t syshal_pmu_get_startup_status(void);
void syshal_pmu_kick_watchdog(void);

void syshal_pmu_assert_callback(uint16_t line_num, const uint8_t *file_name);

#define SYS_ASSERT(expr)                                                      \
    if (expr)                                                                 \
    {                                                                         \
                                                                              \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        syshal_pmu_assert_callback((uint16_t)__LINE__, (uint8_t *)__FILE__);  \
    }

#endif /* _SYSHAL_PMU_H_ */
