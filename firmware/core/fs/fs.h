/* fs.h - Flash File system
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

#ifndef _FS_H_
#define _FS_H_

#include <stdint.h>
#include <stdbool.h>

/* Constants */

#define FS_FILE_ID_NONE    0xFF
#define FS_FILE_CREATE     0x08 /*!< File create flag */
#define FS_FILE_WRITEABLE  0x04 /*!< File is writeable flag */
#define FS_FILE_CIRCULAR   0x02 /*!< File is circular flag */

#define FS_NO_ERROR                    (  0)
#define FS_ERROR_FLASH_MEDIA           ( -1)
#define FS_ERROR_FILE_ALREADY_EXISTS   ( -2)
#define FS_ERROR_FILE_NOT_FOUND        ( -3)
#define FS_ERROR_FILE_PROTECTED        ( -4)
#define FS_ERROR_NO_FREE_HANDLE        ( -5)
#define FS_ERROR_INVALID_MODE          ( -6)
#define FS_ERROR_FILESYSTEM_FULL       ( -7)
#define FS_ERROR_END_OF_FILE           ( -8)
#define FS_ERROR_BAD_DEVICE            ( -9)
#define FS_ERROR_FILESYSTEM_CORRUPTED  (-10)
#define FS_ERROR_INVALID_HANDLE        (-11)

/* Macros */

/* Types */

typedef const void *fs_t;
typedef int32_t fs_handle_t;

typedef enum
{
    FS_MODE_CREATE = FS_FILE_CREATE | FS_FILE_WRITEABLE,
    FS_MODE_CREATE_CIRCULAR = FS_FILE_CREATE | FS_FILE_WRITEABLE | FS_FILE_CIRCULAR,
    FS_MODE_WRITEONLY = FS_FILE_WRITEABLE,
    FS_MODE_READONLY = 0x00
} fs_mode_t;

typedef struct
{
    uint8_t   user_flags;      /* User flags when file was created (ignore for FS_FILE_ID_NONE) */
    bool      is_circular;     /* File type is circular */
    bool      is_protected;    /* File protection status (ignore for FS_FILE_ID_NONE) */
    uint32_t  size;            /* File size (total bytes free for FS_FILE_ID_NONE) */
} fs_stat_t;

/* Functions */

int fs_init(uint32_t device);
int fs_term(uint32_t device);
int fs_mount(uint32_t device, fs_t *fs);
int fs_format(fs_t fs);
int fs_open(fs_t fs, fs_handle_t *handle, uint8_t file_id, fs_mode_t mode, uint8_t *user_flags);
int fs_close(fs_handle_t handle);
int fs_write(fs_handle_t handle, const void *src, uint32_t size, uint32_t *written);
int fs_read(fs_handle_t handle, void *dest, uint32_t size, uint32_t *read);
int fs_seek(fs_handle_t handle, uint32_t seek);
int fs_flush(fs_handle_t handle);
int fs_protect(fs_t fs, uint8_t file_id);
int fs_unprotect(fs_t fs, uint8_t file_id);
int fs_delete(fs_t fs, uint8_t file_id);
int fs_stat(fs_t fs, uint8_t file_id, fs_stat_t *stat);

#endif /* _FS_H_ */
