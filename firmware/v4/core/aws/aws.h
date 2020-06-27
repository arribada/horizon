/* aws.h - Amazon Web Services abstraction layer
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

#ifndef _AWS_H_
#define _AWS_H_

#include <stdint.h>
#include "iot.h"

#define AWS_NO_ERROR      			( 0)
#define AWS_ERROR_BUFFER_TOO_SMALL  (-1)
#define AWS_ERROR_ENCODING          (-2)
#define AWS_ERROR_GENERIC  			(-3) // PLACEHOLDER FOR UNIT TESTS. PLEASE REPLACE

int aws_json_dumps_device_status(const iot_device_status_t * source, char * dest, size_t buffer_size);
int aws_json_gets_device_shadow(const char * source, iot_device_shadow_t * dest, size_t buffer_size);

#endif /* _AWS_H_ */
