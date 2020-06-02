/* fs_priv.h - Flash file system private definitions
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
#ifndef _FS_PRIV_H_
#define _FS_PRIV_H_

#include <stdint.h>

/* Constants */

#define FS_PRIV_NOT_ALLOCATED          -1

#ifndef FS_PRIV_MAX_DEVICES
#define FS_PRIV_MAX_DEVICES             1
#endif

#ifndef FS_PRIV_MAX_HANDLES
#define FS_PRIV_MAX_HANDLES             3
#endif

/* This defines the maximum number of sectors supported
 * by the implementation.
 *
 * NOTE: We use sector numbers 0-254 since 255 (0xFF) is a reserved
 * sector number to denote the end of a file chain.  We could extend
 * to using 0xFFFF as the end of a file chain but that would require
 * extra RAM storage!  So, the last sector is unused.
 */
#ifndef FS_PRIV_MAX_SECTORS
#define FS_PRIV_MAX_SECTORS             255
#endif

#ifndef FS_PRIV_MAX_FILES
#define FS_PRIV_MAX_FILES               255
#endif

/* This defines the number of pages in each sector
 */
#ifndef FS_PRIV_PAGES_PER_SECTOR
#define FS_PRIV_PAGES_PER_SECTOR        256
#endif

/* This defines the page size in bytes
 */
#ifndef FS_PRIV_PAGE_SIZE
#define FS_PRIV_PAGE_SIZE               256
#endif

#ifndef FS_PRIV_SECTOR_SIZE
#define FS_PRIV_SECTOR_SIZE             (FS_PRIV_PAGES_PER_SECTOR * FS_PRIV_PAGE_SIZE)
#endif

#define FS_PRIV_USABLE_SIZE             (FS_PRIV_SECTOR_SIZE - FS_PRIV_ALLOC_UNIT_SIZE)

#define FS_PRIV_SECTOR_ADDR(s)          (s * FS_PRIV_SECTOR_SIZE)

/* Relative addresses to sector boundary for data structures */
#define FS_PRIV_ALLOC_UNIT_HEADER_REL_ADDRESS      0x00000000
#define FS_PRIV_ALLOC_UNIT_SIZE                    FS_PRIV_PAGE_SIZE
#define FS_PRIV_FILE_DATA_REL_ADDRESS \
    (FS_PRIV_ALLOC_UNIT_HEADER_REL_ADDRESS + FS_PRIV_ALLOC_UNIT_SIZE)

/* Address offsets in allocation unit */
#define FS_PRIV_FILE_ID_OFFSET          0
#define FS_PRIV_FILE_PROTECT_OFFSET     1
#define FS_PRIV_NEXT_ALLOC_UNIT_OFFSET  2
#define FS_PRIV_FLAGS_OFFSET            3
#define FS_PRIV_SECTOR_RAW_OFFSET       4
#define FS_PRIV_SECTOR_USER_OFFSET      8
#define FS_PRIV_ALLOC_COUNTER_OFFSET    12
#define FS_PRIV_SESSION_OFFSET          8

#define FS_PRIV_INVALID_INDEX         (-1)

/* Macros */

/* Types */

#pragma pack(4)

typedef union
{
    uint8_t flags;
    struct
    {
        uint8_t mode_flags:4;
        uint8_t user_flags:4;
    };
} fs_priv_flags_t;


typedef struct
{
    uint8_t file_id;
    uint8_t  file_protect;
    uint8_t next_allocation_unit;
    fs_priv_flags_t file_flags;
    uint32_t sector_raw_size;
    uint32_t sector_user_size;
} fs_priv_file_info_t;

typedef struct
{
    fs_priv_file_info_t file_info;
    uint32_t            alloc_counter;
} fs_priv_alloc_unit_header_t;

typedef struct
{
    fs_priv_alloc_unit_header_t header;
} fs_priv_alloc_unit_t;

typedef struct
{
    uint32_t device;
    uint32_t sector;
    fs_priv_alloc_unit_header_t alloc_unit;
} fs_priv_t;

typedef struct
{
    fs_priv_t      *fs_priv;              /*!< File system object pointer */
    fs_priv_flags_t flags;                /*!< File open mode flags */
    uint8_t         file_id;              /*!< File identifier for this file */
    uint8_t         root_allocation_unit; /*!< Root sector of file */
    uint8_t         curr_allocation_unit; /*!< Current accessed sector of file */
    uint32_t        last_data_offset;     /*!< Read: last readable offset, Write: last flash write position */
    uint32_t        curr_data_offset;     /*!< Current read/write data offset in sector */
    uint32_t        user_data_size;       /*!< User data bytes stored in sector (excludes write headers) */
    int             assigned_index;       /*!< Assigned index that is associated with this handle */
    union
    {
        uint16_t    write_header;
        uint8_t     page_cache[FS_PRIV_PAGE_SIZE];  
    };
} fs_priv_handle_t;

#pragma pack()

#endif /* _FS_PRIV_H_ */
