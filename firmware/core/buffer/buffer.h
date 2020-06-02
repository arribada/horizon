/* buffer.h - Buffer library for handling circular and pool buffers
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

#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdint.h>

#ifndef BUFFER_MAX_POOL_BUFFERS
#define BUFFER_MAX_POOL_BUFFERS		(32)
#endif

// IMPORTANT NOTE: This is a private define for dimensioning the memory size allocated
// for each buffer's internal struct.  It has been dimensioned according to
// the maximum number of pool buffers which is a compile-time option set via
// BUFFER_MAX_POOL_BUFFERS.  There is a fixed overhead of 20 for other housekeeping
// state stored by the internal struct.  The constant is defined in units of "void*"
// so that it will scale accordingly for 32-bit or 64-bit architectures.
#define _BUF_PRIV_SZ      (20 + (2 * BUFFER_MAX_POOL_BUFFERS))

typedef struct
{
    void *__priv[_BUF_PRIV_SZ];
} buffer_t;

void circular_buffer_init(void *buf, uintptr_t base_addr, uint32_t sz);
void pool_buffer_init(void *buf, uintptr_t base_addr, uint32_t sz, uint32_t n_buffers);

#define buffer_init(buf, ...)                buffer_init_policy(circular, buf, ## __VA_ARGS__)
#define buffer_init_policy(policy, buf, ...) policy ## _buffer_init((void *)buf, ## __VA_ARGS__)

#define buffer_write(buf, addr)       ( ( (uint32_t(*)(void *, uintptr_t *)) ((buf)->__priv[0]) ) ) ((void *)buf, addr)
#define buffer_read(buf, addr)        ( ( (uint32_t(*)(void *, uintptr_t *)) ((buf)->__priv[1]) ) ) ((void *)buf, addr)
#define buffer_write_advance(buf, sz) ( ( (void(*)(void *, uint32_t))        ((buf)->__priv[2]) ) ) ((void *)buf, sz)
#define buffer_read_advance(buf, sz)  ( ( (void(*)(void *, uint32_t))        ((buf)->__priv[3]) ) ) ((void *)buf, sz)
#define buffer_occupancy(buf)         ( ( (uint32_t(*)(void *))              ((buf)->__priv[4]) ) ) ((void *)buf)
#define buffer_free(buf)              ( ( (uint32_t(*)(void *))              ((buf)->__priv[5]) ) ) ((void *)buf)
#define buffer_overflows(buf)         ( ( (uint32_t(*)(void *))              ((buf)->__priv[6]) ) ) ((void *)buf)
#define buffer_reset(buf)             ( ( (void(*)(void *))                  ((buf)->__priv[7]) ) ) ((void *)buf)

#endif /* _BUFFER_H */
