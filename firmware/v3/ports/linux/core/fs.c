#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "fs_priv.h"
#include "fs.h"



int fs_write(fs_handle_t handle, const void *src, uint32_t size, uint32_t *written)
{
    *written = fwrite(src, 1, size, (FILE *) handle);
    if (*written <= 0) return FS_ERROR_FILESYSTEM_FULL;
    return FS_NO_ERROR;

}
int fs_read(fs_handle_t handle, void *dest, uint32_t size, uint32_t *read)
{
    FILE *fp  = (FILE *) handle;
    *read = fread(dest, 1, size, fp);
    if (*read <= 0) return FS_ERROR_FILESYSTEM_FULL;
    return FS_NO_ERROR;
}