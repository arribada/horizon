/* syshal_adc.h - HAL for ADC
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

#ifndef _SYSHAL_ADC_H_
#define _SYSHAL_ADC_H_

#include <stdint.h>
#include <stdbool.h>

#include "bsp.h"

// Constants
#define SYSHAL_ADC_NO_ERROR               ( 0)
#define SYSHAL_ADC_ERROR_INVALID_INSTANCE (-1)
#define SYSHAL_ADC_ERROR_NOT_INITIALISED  (-2)

int syshal_adc_init(uint32_t instance);
int syshal_adc_term(uint32_t instance);
int syshal_adc_read(uint32_t instance, uint16_t * value);

#endif /* _SYSHAL_ADC_H_ */