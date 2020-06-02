/* buffer.c - Buffer library for handling circular and pool buffers
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

#include <stdint.h>
#include "buffer.h"

#define MIN(x,y) (x) < (y) ? (x) : (y)

#define __FUNCTIONS__ \
    uint32_t (*write)(void *, uintptr_t *addr); \
    uint32_t (*read)(void *, uintptr_t *addr); \
    void     (*write_advance)(void *, uint32_t sz); \
    void     (*read_advance)(void *, uint32_t sz); \
    uint32_t (*occupancy)(void *); \
    uint32_t (*free)(void *); \
    uint32_t (*overflows)(void *); \
    void     (*reset)(void *);

typedef struct
{
    __FUNCTIONS__
    uint32_t  buffer_rd_idx;
    uint32_t  buffer_wr_idx;
    uintptr_t base_addr;
    uint32_t  buffer_size;
    uint32_t  overflow_count;
} circular_buffer_t;

typedef struct
{
    __FUNCTIONS__
    uint32_t  buffer_rd_idx;
    uint32_t  buffer_wr_idx;
    uintptr_t base_addr[BUFFER_MAX_POOL_BUFFERS];
    uint32_t  bytes[BUFFER_MAX_POOL_BUFFERS];
    uint32_t  n_buffers;
    uint32_t  buffer_size;
    uint32_t  overflow_count;
} pool_buffer_t;

static uint32_t circular_buffer_occupancy(void *b)
{
    circular_buffer_t *buf = (circular_buffer_t *)b;
    return (buf->buffer_wr_idx - buf->buffer_rd_idx);
}

static uint32_t circular_buffer_write(void *b, uintptr_t *addr)
{
    circular_buffer_t *buf = (circular_buffer_t *)b;
    *addr = buf->base_addr + (buf->buffer_wr_idx % buf->buffer_size);
    return buf->buffer_size - (buf->buffer_wr_idx % buf->buffer_size);
}

static uint32_t circular_buffer_read(void *b, uintptr_t *addr)
{
    circular_buffer_t *buf = (circular_buffer_t *)b;
    *addr = buf->base_addr + (buf->buffer_rd_idx % buf->buffer_size);
    return MIN(circular_buffer_occupancy(b), (buf->buffer_size - (buf->buffer_rd_idx % buf->buffer_size)));
}

static uint32_t circular_buffer_free(void *b)
{
    circular_buffer_t *buf = (circular_buffer_t *)b;
    return buf->buffer_size - circular_buffer_occupancy(b);
}

static uint32_t circular_buffer_overflows(void *b)
{
    circular_buffer_t *buf = (circular_buffer_t *)b;
    return buf->overflow_count;
}

static void circular_buffer_write_advance(void *b, uint32_t sz)
{
    circular_buffer_t *buf = (circular_buffer_t *)b;
    uint32_t free = circular_buffer_free(b);

    if (free < sz)
    {
        buf->buffer_rd_idx += (sz - free);
        buf->overflow_count++;
    }

    buf->buffer_wr_idx += sz;
}

static void circular_buffer_read_advance(void *b, uint32_t sz)
{
    circular_buffer_t *buf = (circular_buffer_t *)b;
    buf->buffer_rd_idx += sz;
}

static void circular_buffer_reset(void *b)
{
    circular_buffer_t *buf = (circular_buffer_t *)b;
    buf->buffer_wr_idx = buf->buffer_rd_idx;
    buf->overflow_count = 0;
}

void circular_buffer_init(void *b, uintptr_t base_addr, uint32_t sz)
{
    circular_buffer_t *buf = (circular_buffer_t *)b;
    buf->write = circular_buffer_write;
    buf->read = circular_buffer_read;
    buf->write_advance = circular_buffer_write_advance;
    buf->read_advance = circular_buffer_read_advance;
    buf->overflows = circular_buffer_overflows;
    buf->free = circular_buffer_free;
    buf->occupancy = circular_buffer_occupancy;
    buf->reset = circular_buffer_reset;
    buf->base_addr = base_addr;
    buf->buffer_rd_idx = buf->buffer_wr_idx = buf->overflow_count = 0;
    buf->buffer_size = sz;
}

static uint32_t pool_buffer_occupancy(void *b)
{
    pool_buffer_t *buf = (pool_buffer_t *)b;
    return (buf->buffer_wr_idx - buf->buffer_rd_idx);
}

static uint32_t pool_buffer_write(void *b, uintptr_t *addr)
{
    pool_buffer_t *buf = (pool_buffer_t *)b;
    *addr = buf->base_addr[buf->buffer_wr_idx % buf->n_buffers];
    //printf("%p: write at %u\n", buf, buf->buffer_wr_idx % buf->n_buffers);
    return buf->buffer_size;
}

static uint32_t pool_buffer_read(void *b, uintptr_t *addr)
{
    pool_buffer_t *buf = (pool_buffer_t *)b;
    *addr = buf->base_addr[buf->buffer_rd_idx % buf->n_buffers];
    //printf("%p: read %u at %u\n", buf, buf->bytes[buf->buffer_rd_idx % buf->n_buffers], buf->buffer_rd_idx % buf->n_buffers);
    return buf->bytes[buf->buffer_rd_idx % buf->n_buffers];
}

static uint32_t pool_buffer_free(void *b)
{
    pool_buffer_t *buf = (pool_buffer_t *)b;
    return (buf->n_buffers - pool_buffer_occupancy(b));
}

static uint32_t pool_buffer_overflows(void *b)
{
    pool_buffer_t *buf = (pool_buffer_t *)b;
    return buf->overflow_count;
}

static void pool_buffer_read_advance(void *b, uint32_t sz)
{
    (void)sz;
    pool_buffer_t *buf = (pool_buffer_t *)b;
    //printf("%p: read adv %u at %u\n", buf, sz, buf->buffer_rd_idx % buf->n_buffers);
    buf->bytes[buf->buffer_rd_idx++ % buf->n_buffers] = 0;
}

static void pool_buffer_write_advance(void *b, uint32_t sz)
{
    pool_buffer_t *buf = (pool_buffer_t *)b;

    if (!pool_buffer_free(b))
    {
        //printf("%p: overflowed at %u\n", buf, buf->buffer_wr_idx % buf->n_buffers);
        pool_buffer_read_advance(b, 0);
        buf->overflow_count++;
    }

    //printf("%p: write adv %u bytes at idx %u\n", buf, sz, buf->buffer_wr_idx % buf->n_buffers);
    buf->bytes[buf->buffer_wr_idx++ % buf->n_buffers] = sz;
}

static void pool_buffer_reset(void *b)
{
    pool_buffer_t *buf = (pool_buffer_t *)b;
    buf->buffer_wr_idx = buf->buffer_rd_idx;
    buf->overflow_count = 0;
    for (unsigned int i = 0; i < buf->n_buffers; i++)
        buf->bytes[i] = 0;
}

void pool_buffer_init(void *b, uintptr_t base_addr, uint32_t sz, uint32_t n_buffers)
{
    pool_buffer_t *buf = (pool_buffer_t *)b;
    buf->write = pool_buffer_write;
    buf->read = pool_buffer_read;
    buf->write_advance = pool_buffer_write_advance;
    buf->read_advance = pool_buffer_read_advance;
    buf->overflows = pool_buffer_overflows;
    buf->free = pool_buffer_free;
    buf->occupancy = pool_buffer_occupancy;
    buf->reset = pool_buffer_reset;
    buf->buffer_rd_idx = buf->buffer_wr_idx = buf->overflow_count = 0;
    buf->buffer_size = sz / n_buffers;
    buf->n_buffers = n_buffers;
    for (unsigned int i = 0; i < n_buffers; i++)
    {
        buf->base_addr[i] = base_addr + (i * buf->buffer_size);
        buf->bytes[i] = 0;
    }
}

