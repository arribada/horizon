/* fs.c - Flash File system
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

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "fs_priv.h"
#include "fs.h"
#include "syshal_flash.h"
#include "syshal_firmware.h"

/* Constants */

/* Macros */
#undef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#ifdef GTEST
#include <assert.h>
#define SYS_ASSERT(x) assert(x)
#else
#include "syshal_pmu.h"
#endif 

/* Types */

/* Static Data */

/* List of file system devices */
static fs_priv_t fs_priv_list[FS_PRIV_MAX_DEVICES];

/* List of file handles (shared across all devices) */
#ifdef GTEST
fs_priv_handle_t fs_priv_handle_list[FS_PRIV_MAX_HANDLES];
#else
static fs_priv_handle_t fs_priv_handle_list[FS_PRIV_MAX_HANDLES];
#endif

/* Current file handle index that increases with each handle assigned */
#ifdef GTEST
uint32_t curr_index = 1;
#else
static uint32_t curr_index = 1;
#endif

/* Static Functions */
static int read_allocation_unit(fs_priv_t *fs_priv, uint8_t sector);

static void read_allocation_unit_cached(fs_priv_t *fs_priv, uint8_t sector)
{
    if (fs_priv->sector != (uint32_t)sector)
    {
        /* Read the sector header into the cache local copy */
        (void)read_allocation_unit(fs_priv, sector);
    }
}

static inline uint8_t get_user_flags(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    return fs_priv->alloc_unit.file_info.file_flags.user_flags;
}

static inline void set_user_flags(fs_priv_t *fs_priv, uint8_t sector, uint8_t user_flags)
{
    read_allocation_unit_cached(fs_priv, sector);
    fs_priv->alloc_unit.file_info.file_flags.user_flags = user_flags;
}

static inline uint8_t get_mode_flags(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    return fs_priv->alloc_unit.file_info.file_flags.mode_flags;
}

static inline void set_mode_flags(fs_priv_t *fs_priv, uint8_t sector, uint8_t mode_flags)
{
    read_allocation_unit_cached(fs_priv, sector);
    fs_priv->alloc_unit.file_info.file_flags.mode_flags = mode_flags;
}

static inline uint8_t get_file_protect(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    return fs_priv->alloc_unit.file_info.file_protect;
}

static inline void set_file_protect(fs_priv_t *fs_priv, uint8_t sector, uint8_t file_protect)
{
    read_allocation_unit_cached(fs_priv, sector);
    fs_priv->alloc_unit.file_info.file_protect = file_protect;
}

static inline uint32_t get_alloc_counter(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    return fs_priv->alloc_unit.alloc_counter;
}

static inline void set_alloc_counter(fs_priv_t *fs_priv, uint8_t sector, uint32_t counter)
{
    read_allocation_unit_cached(fs_priv, sector);
    fs_priv->alloc_unit.alloc_counter = counter;
}

static inline uint8_t get_file_id(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    return fs_priv->alloc_unit.file_info.file_id;
}

static inline void set_file_id(fs_priv_t *fs_priv, uint8_t sector, uint8_t file_id)
{
    read_allocation_unit_cached(fs_priv, sector);
    fs_priv->alloc_unit.file_info.file_id = file_id;
}

static inline bool is_last_allocation_unit(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    return (fs_priv->alloc_unit.file_info.next_allocation_unit ==
            (uint8_t)FS_PRIV_NOT_ALLOCATED);
}

static inline uint8_t next_allocation_unit(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    return fs_priv->alloc_unit.file_info.next_allocation_unit;
}

static inline void set_next_allocation_unit(fs_priv_t *fs_priv, uint8_t sector, uint8_t next)
{
    read_allocation_unit_cached(fs_priv, sector);
    fs_priv->alloc_unit.file_info.next_allocation_unit = next;
}

static inline uint32_t get_sector_raw_size(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    return fs_priv->alloc_unit.file_info.sector_raw_size;
}

static inline void set_sector_raw_size(fs_priv_t *fs_priv, uint8_t sector, uint32_t sz)
{
    read_allocation_unit_cached(fs_priv, sector);
    fs_priv->alloc_unit.file_info.sector_raw_size = sz;
}

static inline uint32_t get_sector_user_size(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    return fs_priv->alloc_unit.file_info.sector_user_size;
}

static inline void set_sector_user_size(fs_priv_t *fs_priv, uint8_t sector, uint32_t sz)
{
    read_allocation_unit_cached(fs_priv, sector);
    fs_priv->alloc_unit.file_info.sector_user_size = sz;
}

/*! \brief Initialize private file system device structure.
 *
 * Scan through each sector and read the allocation unit
 * header into our local structure to prevent having to
 * read the flash device each time.
 *
 * \param device[in] the device instance number
 * \return \ref FS_ERROR_FLASH_MEDIA if a flash read failed
 * \return \ref FS_ERROR_BAD_DEVICE if the device number is bad
 * \return \ref FS_NO_ERROR on success
 */
static int init_fs_priv(uint32_t device)
{
    fs_priv_t *fs_priv;

    /* This device parameter is used as an index into the file
     * system device list.  We have to range check it before
     * proceeding to avoid bad stuff happening.
     */
    if (device >= FS_PRIV_MAX_DEVICES)
        return FS_ERROR_BAD_DEVICE;

    fs_priv = &fs_priv_list[device];
    fs_priv->device = device;  /* Keep a copy of the device index */
    fs_priv->sector = (uint32_t)FS_PRIV_NOT_ALLOCATED;

    /* TODO: we should probably implement some kind of file system
     * validation check here to avoid using a corrupt file system.
     */
    return FS_NO_ERROR;
}

/*! \brief Return the number of bytes cached on the file handle.
 *
 * \param fs_priv_handle[in] pointer to private flash handle.
 * \return number of bytes cached which shall be limited to the cache size.
 */
static inline uint16_t cached_bytes(fs_priv_handle_t *fs_priv_handle)
{
    return (fs_priv_handle->curr_data_offset - fs_priv_handle->last_data_offset);
}

/*! \brief Ascertain the number of bytes free in the allocation unit.
 *
 * \param fs_priv_handle[in] pointer to private flash handle.
 * \return integer number of bytes that may still be written.
 */
static inline uint32_t remaining_bytes(fs_priv_handle_t *fs_priv_handle)
{
    return (FS_PRIV_USABLE_SIZE - fs_priv_handle->curr_data_offset);
}

/*! \brief Ascertain the number of bytes free in the page.
 *
 * \param fs_priv_handle[in] pointer to private flash handle.
 * \return integer number of bytes that may still be written.
 */
static inline uint16_t remaining_page(fs_priv_handle_t *fs_priv_handle)
{
    return FS_PRIV_PAGE_SIZE - (fs_priv_handle->curr_data_offset & (FS_PRIV_PAGE_SIZE - 1));
}

/*! \brief Find a free allocation unit (sector) in the file system.
 *
 * The \ref fs_priv_file_info_t.file_id field is used to determine whether a
 * sector is allocated or not, so check each sector until we find one whose
 * \ref fs_priv_file_info_t.file_id is set to \ref FS_PRIV_NOT_ALLOCATED.
 *
 * \param fs_priv[in] a pointer to a file system device structure that
 *        we want to search
 * \return \ref FS_PRIV_NOT_ALLOCATED if no free sector found
 * \return sector number if a free sector was found
 */
static uint8_t find_free_allocation_unit(fs_priv_t *fs_priv)
{
    uint32_t min_allocation_counter = (uint32_t)FS_PRIV_NOT_ALLOCATED;
    uint8_t free_sector = (uint8_t)FS_PRIV_NOT_ALLOCATED;

    /* In the worst case, we have to check every sector on the disk to
     * find a free sector.
     */
    for (uint8_t sector = 0; sector < FS_PRIV_MAX_SECTORS; sector++)
    {
        /* Consider only unallocated sectors */
        if ((uint8_t)FS_PRIV_NOT_ALLOCATED == get_file_id(fs_priv, sector))
        {
            /* Special case for unformatted sector */
            if ((uint32_t)FS_PRIV_NOT_ALLOCATED == get_alloc_counter(fs_priv, sector))
            {
                /* Choose this sector since it has never been used */
                free_sector = sector;
                break;
            }
            else if (get_alloc_counter(fs_priv, sector) < min_allocation_counter)
            {
                /* This is now the least used sector */
                min_allocation_counter = get_alloc_counter(fs_priv, sector);
                free_sector = sector;
            }
        }
    }

    return free_sector;
}

/*! \brief Allocate a file handle.
 *
 * The first free handle is found in \ref fs_priv_handle_list and
 * the \ref fs_priv_handle_t pointer shall be populated with it on success.
 *
 * \param fs_priv[in] a pointer to a file system device that we want to associate
 *        the file handle with
 * \param handle[out] a pointer to the file handle.  This shall be
 *        set to to the free handle found.
 * \param priv_handle[out] a pointer to the private file handle struct.  This shall be
 *        set to point to the free handle struct found.
 * \return \ref FS_ERROR_NO_FREE_HANDLE if all handles are in use.
 * \return \ref FS_NO_ERROR on success.
 */
static int allocate_handle(fs_priv_t *fs_priv, fs_handle_t *handle, fs_priv_handle_t **priv_handle)
{
    for (uint8_t i = 0; i < FS_PRIV_MAX_HANDLES; i++)
    {
        uint32_t index_offset = (i + curr_index) % FS_PRIV_MAX_HANDLES;
        if (FS_PRIV_INVALID_INDEX == fs_priv_handle_list[index_offset].assigned_index)
        {
            *priv_handle = &fs_priv_handle_list[index_offset];
            *handle = curr_index + i;
            fs_priv_handle_list[index_offset].fs_priv = fs_priv;
            fs_priv_handle_list[index_offset].assigned_index = curr_index + i;
            curr_index = (curr_index + i + 1) % INT32_MIN; /* Do not permit negative numbers when converted to an int32_t */

            return FS_NO_ERROR;
        }
    }

    return FS_ERROR_NO_FREE_HANDLE;
}

/*! \brief Get the internal handle struct for the given handle.
 *
 * \param handle[in] the file handle to retrieve the struct of.
 * \params handle[out] the private file struct associated with this handle
 * \return true if the given handle is valid.
 * \return false if the given handle is invalid.
 */
static __RAMFUNC bool get_handle_struct(fs_handle_t handle, fs_priv_handle_t **priv_handle)
{
    if (handle < 0)
        return false;

    if (fs_priv_handle_list[handle % FS_PRIV_MAX_HANDLES].assigned_index != handle)
        return false;

    *priv_handle = &fs_priv_handle_list[handle % FS_PRIV_MAX_HANDLES];
    return true;
}

/*! \brief Free a file handle.
 *
 * The handle is freed by setting its \ref fs_priv_handle_t.fs_priv
 * member to NULL.
 *
 * \param handle[in] the file handle to be freed.
 */
static void free_handle(fs_handle_t handle)
{
    fs_priv_handle_t *fs_priv_handle;

    if (get_handle_struct(handle, &fs_priv_handle))
        fs_priv_handle->assigned_index = FS_PRIV_INVALID_INDEX;
}

/*! \brief Check a file's protection bits (see \ref
 *  fs_priv_file_info_t.file_protect) to see if the file
 *  is write protected or not.
 *
 * The file protection scheme counts the number of set bits
 * to determine if a file is protected or not.  If there
 * are zero or an even number of bits, the file is not protected.
 * If there are an odd number of bits, the file is protected.
 *
 * \param protection_bits[in] taken from \ref
 *  fs_priv_file_info_t.file_protect
 * \return true if the file is protected, false otherwise
 */
static bool is_protected(uint8_t protection_bits)
{
    uint8_t count_bits;
    for (count_bits = 0; protection_bits; count_bits++)
        protection_bits &= protection_bits - 1; /* Clear LSB */

    /* Odd number of bits means the file is protected */
    return (count_bits & 1) ? true : false;
}

/*! \brief Set a file's protection bits (see \ref
 *  fs_priv_file_info_t.file_protect).
 *
 * If the protection state is to be modified then the
 * number of set bits shall be modified.  Caution
 * should be taken to ensure that the value is not
 * already zero since this results in no change to
 * the protection bits i.e., a file will remain
 * permanently unprotected until erased in this
 * situation.
 *
 * \param protected[in] the boolean state that we wish to apply
 *        to the protected bits
 * \param protected_bits[in] current file protection bits
 *        value as per \ref fs_priv_file_info_t.file_protect
 * \return new value of the file's protection bits which
 *         shall only be different if the file's protection
 *         state changed
 */
static uint8_t set_protected(bool protected, uint8_t protected_bits)
{
    /* Only update protected bits if the current bits do not
     * match the required protection state
     */
    if (protected != is_protected(protected_bits))
        protected_bits &= protected_bits - 1; /* Clear LSB */

    /* Return new protected bits */
    return protected_bits;
}

/*! \brief Find the root sector in a chain of linked
 *  allocation units on the file system for a given file.
 *
 * Each allocation unit has a \ref fs_priv_file_info_t.next_allocation_unit
 * pointer which may be used to form an order chain of sectors associated
 * with a given file.  This routine shall scan all sectors associated
 * with a file and determine the root sector i.e., the sector which
 * has no parent node.
 *
 * \param fs_priv[in] a pointer to the private file system structure.
 * \param file_id[in] unique file identifier
 * \return sector[in] number of the root sector on success
 * \return \ref FS_PRIV_NOT_ALLOCATED if the file was not found
 */
static uint8_t find_file_root(fs_priv_t *fs_priv, uint8_t file_id)
{
    uint8_t root = (uint8_t)FS_PRIV_NOT_ALLOCATED;
    uint8_t parent[FS_PRIV_MAX_SECTORS];

    /* Do not allow FS_PRIV_NOT_ALLOCATED as file_id */
    if ((uint8_t)FS_PRIV_NOT_ALLOCATED == file_id)
        return FS_PRIV_NOT_ALLOCATED;

    /* Reset parent list to known values */
    memset(parent, (uint8_t)FS_PRIV_NOT_ALLOCATED, sizeof(parent));

    /* Scan all sectors and build a list of parent nodes for each
     * sector allocated against the specified file_id
     */
    for (uint8_t sector = 0; sector < FS_PRIV_MAX_SECTORS; sector++)
    {
        /* Filter by file_id */
        if (file_id == get_file_id(fs_priv, sector))
        {
            if (!is_last_allocation_unit(fs_priv, sector))
                parent[next_allocation_unit(fs_priv, sector)] = sector;
            /* Arbitrarily choose first found sector as the candidate root node */
            if (root == (uint8_t)FS_PRIV_NOT_ALLOCATED)
                root = sector;
        }
    }

    /* Start with candidate root sector and walk all the parent nodes until we terminate */
    while (root != (uint8_t)FS_PRIV_NOT_ALLOCATED)
    {
        /* Does this node have a parent?  If not then it is the root node */
        if (parent[root] == (uint8_t)FS_PRIV_NOT_ALLOCATED)
            break;
        else
            root = parent[root]; /* Try next one in chain */
    }

    /* Return the root node which may be FS_PRIV_NOT_ALLOCATED if no file was found */
    return root;
}

/*! \brief Check consistency of file open mode versus file's stored flags.
 *
 * Certain file open modes are erroneous e.g., creating a file whose
 * file_id already exists, opening a file for writing that has been
 * protected.  This routine traps an such errors.
 *
 * \param fs_priv[in] a pointer to the private file system structure.
 * \param root[in] the root sector number containing the file's properties.
 * \param mode[in] desired mode requested for opening the file.
 * \return \ref FS_NO_ERROR if all checks pass
 * \return \ref FS_ERROR_FILE_NOT_FOUND if the root node was not found
 * \return \ref FS_ERROR_WRITE_PROTECTED if the file is write protected
 * \return \ref FS_ERROR_FILE_ALREADY_EXISTS if the file already exists
 */
static int check_file_flags(fs_priv_t *fs_priv, uint8_t root, fs_mode_t mode)
{
    if ((uint8_t)FS_PRIV_NOT_ALLOCATED == root)
    {
        /* File does not exist so unless this is a create request then
         * return an error.
         */
        if ((mode & FS_FILE_CREATE) == 0)
            return FS_ERROR_FILE_NOT_FOUND;
    }
    else
    {
        /* Don't allow the file to be created since it already exists */
        if (mode & FS_FILE_CREATE)
            return FS_ERROR_FILE_ALREADY_EXISTS;

        /* If opened as writeable then make sure file is not protected */
        const uint8_t protection_bits = get_file_protect(fs_priv, root);
        if ((mode & FS_FILE_WRITEABLE) && is_protected(protection_bits))
            return FS_ERROR_FILE_PROTECTED;
    }

    return FS_NO_ERROR;
}

/*! \brief Scans to find next available write position in a sector.
 *
 * Before writing to a sector of a file, it is necessary to find
 * the last known write position and also the next available session
 * write offset, so the new write position can be tracked.
 *
 * \param fs_priv[in] a pointer to the private file system structure.
 * \param sector[in] the sector number to check.
 * \param data_offset[out] pointer to data offset relative to start of sector
 *        which shall be used to store the current write offset
 * \param sector_size[out] pointer to sector size which will be used to
 *        store the number of user data bytes stored in the sector
 *        This parameter is optional and may be set to NULL.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_PRIV_NOT_ALLOCATED an integrity error occurred.
 */
__RAMFUNC static int find_end_of_sector(fs_priv_t *fs_priv, uint8_t sector, uint32_t *data_offset,
        uint32_t *sector_size)
{
    uint32_t sector_bytes = 0;
    uint32_t address = FS_PRIV_SECTOR_ADDR(sector) + FS_PRIV_ALLOC_UNIT_SIZE;
    uint32_t pages = 0;
    uint32_t page_offset;
    uint8_t  page_data[FS_PRIV_PAGE_SIZE];
    int error = FS_NO_ERROR;

    /* If the sector is full we don't need to read every page to ascertain its
     * true size or total user bytes.  We can use the stored header information
     * in the sector.  This gives us a significant speed-up when opening an
     * existing file or indeed performing an fs_stat().
     */
    if (get_sector_raw_size(fs_priv, sector) != (uint32_t)FS_PRIV_NOT_ALLOCATED)
    {
        /* The sector has valid header information -- so use this instead */
        *data_offset = get_sector_raw_size(fs_priv, sector);
        if (NULL != sector_size)
            *sector_size = get_sector_user_size(fs_priv, sector);
        return error;
    }

    while (pages++ < (FS_PRIV_PAGES_PER_SECTOR - 1))
    {
        /* Read page from flash memory */
        syshal_flash_read(fs_priv->device, page_data, address, FS_PRIV_PAGE_SIZE);
        page_offset = 0;

        /* Iterate through write segment operations in this page */
        while (page_offset < (FS_PRIV_PAGE_SIZE - 2))
        {
            uint16_t write_header_len = ((uint16_t)page_data[page_offset] |
                ((uint16_t)page_data[page_offset + 1] << 8));

            /* Write header must be at least 3 bytes */
            if (write_header_len <= 2)
            {
                if (NULL != sector_size)
                    *sector_size = sector_bytes;
                return FS_ERROR_FILESYSTEM_CORRUPTED;
            }

            if (0xFFFF == write_header_len)
            {
                /* Last used page in sector found */
                *data_offset = (address % FS_PRIV_SECTOR_SIZE) + page_offset - FS_PRIV_ALLOC_UNIT_SIZE;
                if (NULL != sector_size)
                    *sector_size = sector_bytes;
                return error;
            }
            else if ((page_offset + write_header_len) > FS_PRIV_PAGE_SIZE)
            {
                if (NULL != sector_size)
                    *sector_size = sector_bytes;
                /* Write header exceeds the page boundary */
                return FS_ERROR_FILESYSTEM_CORRUPTED;
            }

            page_offset = page_offset + write_header_len;
            sector_bytes = sector_bytes + write_header_len - 2;
        }

        address = address + FS_PRIV_PAGE_SIZE;
    }

    /* Reached the end of the sector without finding EOF */
    *data_offset = FS_PRIV_SECTOR_SIZE - FS_PRIV_ALLOC_UNIT_SIZE;
    if (NULL != sector_size)
        *sector_size = sector_bytes;

    return error;
}

/*! \brief Find the last allocation unit in the chain of sectors
 *  for a given file.
 *
 * Simply start from the root sector and iterate until we reach
 * a sector whose \ref fs_priv_file_info_t.next_allocation_unit
 * is set to \ref FS_PRIV_NOT_ALLOCATED.
 *
 * \param fs_priv[in] a pointer to the private file system structure.
 * \param root[in] the root sector number to start from.
 * \return sector offset for the last sector of the file.
 */
static uint8_t find_last_allocation_unit(fs_priv_t *fs_priv, uint8_t root)
{
    /* Loop through until we reach the end of the file chain */
    while (root != (uint8_t)FS_PRIV_NOT_ALLOCATED)
    {
        if (next_allocation_unit(fs_priv, root) != (uint8_t)FS_PRIV_NOT_ALLOCATED)
            root = next_allocation_unit(fs_priv, root);
        else
            break;
    }

    return root;
}

/*! \brief Find the last allocation unit and also the last known
 * write position in that allocation unit.
 *
 * See \ref find_last_allocation_unit and \ref find_end_of_sector.
 *
 * \param fs_priv[in] a pointer to the private file system structure.
 * \param root[in] the root sector number to start from.
 * \param last_alloc_unit[out] pointer for storing the sector offset of
 *         the last allocation unit in the file chain.
 * \param data_offset pointer[out] for storing the last known write position
 *        in the last sector of the file.
 * \param user_data_size[out] for storing the number of user bytes in the last sector
 * \return session write offset e.g., 0 means the first session entry.
 * \return \ref FS_PRIV_NOT_ALLOCATED if no session entry is free.
 */
static int find_eof(fs_priv_t *fs_priv, uint8_t root, uint8_t *last_alloc_unit,
        uint32_t *data_offset, uint32_t *user_data_size)
{
    *last_alloc_unit = find_last_allocation_unit(fs_priv, root);
    return find_end_of_sector(fs_priv, *last_alloc_unit, data_offset, user_data_size);
}

/*! \brief Determines if a handle is in the end of a file condition.
 *
 * When a file is in write mode, this condition is only met if all
 * bytes in the page cache have been flushed to flash.
 *
 * When a file id in read mode then the condition is only met if the
 * current data offset is pointing to the last byte of the last sector
 * in the file chain.
 *
 * \param fs_priv_handle[in] pointer to private file handle
 * \return true if the handle is EOF, false otherwise
 */
__RAMFUNC static bool is_eof(fs_priv_handle_t *fs_priv_handle)
{
    fs_priv_t *fs_priv = fs_priv_handle->fs_priv;

    return ((fs_priv_handle->last_data_offset <= fs_priv_handle->curr_data_offset) &&
            next_allocation_unit(fs_priv, fs_priv_handle->curr_allocation_unit) ==
                    (uint8_t)FS_PRIV_NOT_ALLOCATED);
}

/*! \brief Erase allocation unit in flash.
 *
 * This will physically erase the associated sector on flash media
 * and also update the allocation counter for the allocation unit.
 * Both the alloction unit header stored locally and stored in
 * flash are synchronized.
 *
 * \param fs_priv[in] a pointer to the private file system structure.
 * \param sector the sector number to erase.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA on error.
 */
static int erase_allocation_unit(fs_priv_t *fs_priv, uint8_t sector)
{
    /* Read existing allocation counter and increment for next allocation */
     uint32_t new_alloc_counter = get_alloc_counter(fs_priv, sector) + 1;

    /* Erase the entire sector (should be all FF) */
    if (syshal_flash_erase(fs_priv->device, FS_PRIV_SECTOR_ADDR(sector), FS_PRIV_SECTOR_SIZE))
        return FS_ERROR_FLASH_MEDIA;

    /* Reset local copy of allocation unit header */
    memset(&fs_priv->alloc_unit, 0xFF, sizeof(fs_priv->alloc_unit));

    /* Set allocation counter locally */
    set_alloc_counter(fs_priv, sector, new_alloc_counter);

    /* Write only the allocation counter to flash */
    if (syshal_flash_write(fs_priv->device, &new_alloc_counter,
            FS_PRIV_SECTOR_ADDR(sector) + FS_PRIV_ALLOC_COUNTER_OFFSET,
            sizeof(uint32_t)))
        return FS_ERROR_FLASH_MEDIA;

    return FS_NO_ERROR;
}

/*! \brief Read allocation unit from flash into local cache.
 *
 * \param fs_priv[in] a pointer to the private file system structure.
 * \param sector the sector number to read.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA on error.
 */
static int read_allocation_unit(fs_priv_t *fs_priv, uint8_t sector)
{
    if (syshal_flash_read(fs_priv->device, &fs_priv->alloc_unit,
            FS_PRIV_SECTOR_ADDR(sector), sizeof(fs_priv_alloc_unit_header_t)))
        return FS_ERROR_FLASH_MEDIA;

    fs_priv->sector = (uint32_t)sector;

    return FS_NO_ERROR;
}

/*! \brief Write allocation unit in flash.
 *
 * This will physically write the first page of the sector to flash.
 *
 * \param fs_priv[in] a pointer to the private file system structure.
 * \param sector the sector number to write.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA on error.
 */
static int write_allocation_unit(fs_priv_t *fs_priv, uint8_t sector)
{
    read_allocation_unit_cached(fs_priv, sector);
    if (syshal_flash_write(fs_priv->device, &fs_priv->alloc_unit,
            FS_PRIV_SECTOR_ADDR(sector),
            sizeof(fs_priv_file_info_t)))
        return FS_ERROR_FLASH_MEDIA;
    return FS_NO_ERROR;
}

/*! \brief Write cached file data to flash memory.
 *
 * \param fs_priv_handle[in] pointer to private flash handle.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA if a flash write failed.
 */
static int flush_page_cache(fs_priv_handle_t *fs_priv_handle)
{
    uint32_t size, address;

    /* Compute number of bytes in page cache; note that the cache fill policy
     * means that we can never exceed the next page boundary, so we don't need to
     * worry about crossing a page boundary.
     */
    size = cached_bytes(fs_priv_handle);

    if (size > 0)
    {
        /* Physical address calculation */
        address = FS_PRIV_SECTOR_ADDR(fs_priv_handle->curr_allocation_unit) +
                FS_PRIV_ALLOC_UNIT_SIZE + fs_priv_handle->last_data_offset;

        /* Write cached data to flash */
        if (syshal_flash_write(fs_priv_handle->fs_priv->device, fs_priv_handle->page_cache,
                address, size))
            return FS_ERROR_FLASH_MEDIA;

        /* Mark the cache as empty and advance the pointer */
        fs_priv_handle->last_data_offset = fs_priv_handle->curr_data_offset;
    }

    return FS_NO_ERROR;
}

/*! \brief Flush file handle.
 *
 * Forces the writing of any cached data and the session write
 * offset to flash memory.
 *
 * \param fs_priv_handle[in] pointer to private flash handle.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA if a flash write failed.
 */
static int flush_handle(fs_priv_handle_t *fs_priv_handle)
{
    /* Flush any bytes in the page cache */
    return flush_page_cache(fs_priv_handle);
}

/*! \brief Allocate new sector to a file.
 *
 * When creating or writing to a file it will be necessary to
 * sometimes allocate a new sector.  This involves find a free sector
 * and then updating the file allocation table to reflect that
 * a new sector has been chained to the end of the file.  Note
 * that any file allocation data is written to flash immediately to
 * ensure integrity of the file allocation information.
 *
 * \param fs_priv_handle[in] pointer to private flash handle for which we
 * want to allocate a new sector for.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA if a flash write failed.
 */
static int allocate_new_sector_to_file(fs_priv_handle_t *fs_priv_handle)
{
    uint8_t sector;
    fs_priv_t *fs_priv = fs_priv_handle->fs_priv;
    uint8_t file_protect = 0xFF;

    if ((uint8_t)FS_PRIV_NOT_ALLOCATED != fs_priv_handle->root_allocation_unit)
    {
        file_protect = get_file_protect(fs_priv, fs_priv_handle->root_allocation_unit);
    }

    /* Find a free allocation unit */
    sector = find_free_allocation_unit(fs_priv);
    if ((uint8_t)FS_PRIV_NOT_ALLOCATED == sector)
    {
        /* File system is full but if the file type is circular
         * then we should erase the root sector and try to recycle it.
         */
        if ((fs_priv_handle->flags.mode_flags & FS_FILE_CIRCULAR) == 0 ||
            fs_priv_handle->root_allocation_unit == (uint8_t)FS_PRIV_NOT_ALLOCATED)
            return FS_ERROR_FILESYSTEM_FULL;

        /* Erase the current root sector so it can be recycled */
        uint8_t new_root = next_allocation_unit(fs_priv, fs_priv_handle->root_allocation_unit);
        if (erase_allocation_unit(fs_priv, fs_priv_handle->root_allocation_unit))
            return FS_ERROR_FLASH_MEDIA;

        /* Set new root sector and also link the new sector's next pointer to
         * the new root sector.
         */
        sector = fs_priv_handle->root_allocation_unit;
        fs_priv_handle->root_allocation_unit = new_root;
    }

    /* Update file system allocation table information for this allocation unit */
    set_file_id(fs_priv, sector, fs_priv_handle->file_id);
    set_next_allocation_unit(fs_priv, sector, (uint8_t)FS_PRIV_NOT_ALLOCATED);
    set_mode_flags(fs_priv, sector, (fs_priv_handle->flags.mode_flags & FS_FILE_CIRCULAR));
    set_user_flags(fs_priv, sector, fs_priv_handle->flags.user_flags);
    set_sector_raw_size(fs_priv, sector, (uint32_t)FS_PRIV_NOT_ALLOCATED);
    set_sector_user_size(fs_priv, sector, (uint32_t)FS_PRIV_NOT_ALLOCATED);

    /* Check if a root sector is already set for this handle */
    if ((uint8_t)FS_PRIV_NOT_ALLOCATED == fs_priv_handle->root_allocation_unit)
    {
        /* Assign this sector as the handle's root node */
        fs_priv_handle->root_allocation_unit = sector;

        /* Reset file protect bits */
        set_file_protect(fs_priv, sector, file_protect);

        /* Write file information header contents to flash for new sector */
        if (write_allocation_unit(fs_priv, sector))
            return FS_ERROR_FLASH_MEDIA;
    }
    else
    {
        /* Set file protect bits for new sector using root sector file protect bits */
        set_file_protect(fs_priv, sector, file_protect);

        /* Write file information header contents to flash for new sector */
        if (write_allocation_unit(fs_priv, sector))
            return FS_ERROR_FLASH_MEDIA;

        /* Chain newly allocated sector onto the end of the current sector */
        set_next_allocation_unit(fs_priv, fs_priv_handle->curr_allocation_unit, sector);

        /* Set the sector raw and user data size fields */
        set_sector_raw_size(fs_priv, fs_priv_handle->curr_allocation_unit, fs_priv_handle->curr_data_offset);
        set_sector_user_size(fs_priv, fs_priv_handle->curr_allocation_unit, fs_priv_handle->user_data_size);

        /* Write updated file information header contents to flash for the current sector */
        if (write_allocation_unit(fs_priv, fs_priv_handle->curr_allocation_unit))
            return FS_ERROR_FLASH_MEDIA;
    }

    /* Reset handle pointers to start of new sector */
    fs_priv_handle->curr_allocation_unit = sector;
    fs_priv_handle->last_data_offset = 0;
    fs_priv_handle->curr_data_offset = 0;
    fs_priv_handle->user_data_size = 0;

    return FS_NO_ERROR;
}

/*! \brief Ascertain if the allocation unit is full.
 *
 * A sector is considered full if all usable bytes have been
 * written in the sector.
 *
 * \param fs_priv_handle[in] pointer to private flash handle.
 * \return true if full, false if not full.
 */
static inline bool is_full(fs_priv_handle_t *fs_priv_handle)
{
    return (fs_priv_handle->curr_data_offset >= FS_PRIV_USABLE_SIZE);
}

/*! \brief Write data to a file handle through its cache.
 *
 * The cache fill policy ensures that the cache may never fill more
 * than the number of bytes to the next page boundary i.e., the cache
 * size is at most 1 full page before all data will be emptied from
 * the cache and written into flash.
 *
 * \param fs_priv_handle[in] pointer to private flash handle.
 * \param src[in] pointer to buffer containing bytes to write.
 * \param size[in] number of bytes to write.
 * \param written[out] pointer for storing number of bytes written.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA on a flash write error.
 */
static int write_through_cache(fs_priv_handle_t *fs_priv_handle, const void *src, uint32_t size, uint32_t *written)
{
    uint32_t cached, page_boundary, sz;
    int ret;

    /* Check if the current sector is full */
    if (is_full(fs_priv_handle))
    {
        /* Flush file to clear cache */
        flush_handle(fs_priv_handle);

        /* Allocate new sector to file chain */
        ret = allocate_new_sector_to_file(fs_priv_handle);
        if (ret) return ret;
    }

    /* Cache operation
     *
     * Rule 1: Write the cache to flash only when there are at least "page_boundary" bytes in it.
     * Rule 2: Keep caching data until Rule 1 applies.
     *
     * cache = { 0 ... 512 } => number of bytes in the cache (including 2 byte write header)
     * page_boundary = { 0 ... 512 } => number of bytes until the next page boundary
     * 0 <= (page_boundary - cache) <= 512 => cache occupancy may never exceed page_boundary
     *
     * The page cache in RAM will always look like this:
     *
     *   0   1                                       i            511
     * +-------------------------------------------------------------+
     * |   |   |                                   |   |     |   |   |
     * | i | i |  ..Data...                    .   |FF | ... |FF |FF |
     * |   |   |                                   |   |     |   |   |
     * +-------------------------------------------------------------+
     *
     * In the above case it contains 'i' bytes with i-2 user data bytes.  The remainder
     * of the cache is filled with FF.
     *
     * Note that since the page cache can be independently flushed we may end up
     * with pages that looks something like this inside the flash memory:
     *
     *                    Flush                  Flush(EOF)
     *                      |                      |
     *                      V                      V
     *   0   1                i  i+1                i+j           511
     * +-------------------------------------------------------------+
     * |   |   |            |   |   |              |   |     |   |   |
     * | i | i |  ..Data... | j | j | ...Data...   |FF | ... |FF |FF |
     * |   |   |            |   |   |              |   |     |   |   |
     * +-------------------------------------------------------------+
     *
     * In certain situations a flush may happen when there are only 1 or 2 bytes
     * remaining in the page.  The last 1 or 2 bytes are not used in these cases,
     * since we need to least 3 bytes to store any user data.
     */
    cached = cached_bytes(fs_priv_handle);

    /* Check if we need to reset the cache state */
    if (0 == cached)
    {
        /* Check page can store enough data */
        if (remaining_page(fs_priv_handle) <= 2)
        {
            /* Skip to the next page */
            fs_priv_handle->last_data_offset += remaining_page(fs_priv_handle);
            fs_priv_handle->curr_data_offset = fs_priv_handle->last_data_offset;
        }

        /* Check if the sector can store any more data */
        if (remaining_bytes(fs_priv_handle) <= 2)
        {
            /* Allocate new sector to file chain */
            ret = allocate_new_sector_to_file(fs_priv_handle);
            if (ret) return ret;
        }

        /* Reset page cache to prepare next flash write operation */
        memset(fs_priv_handle->page_cache, 0xFF, FS_PRIV_PAGE_SIZE);
        cached = 2;
        fs_priv_handle->write_header = 2;
        fs_priv_handle->curr_data_offset += 2;
    }

    page_boundary = FS_PRIV_PAGE_SIZE - (fs_priv_handle->last_data_offset & (FS_PRIV_PAGE_SIZE - 1));
    SYS_ASSERT(cached <= page_boundary);
    *written = 0;

    /* Append to the cache up to the limit of the next page boundary */
    sz = MIN((page_boundary - cached), size);
    if (sz > 0)
    {
        memcpy(&fs_priv_handle->page_cache[cached], src, sz);
        size -= sz;
        cached += sz;
        *written += sz;
        fs_priv_handle->curr_data_offset += sz;
        fs_priv_handle->write_header += sz;
        fs_priv_handle->user_data_size += sz;
    }

    /* Cache can never exceed page boundary but we should
     * remove data from the cache once there is sufficient
     * data to write to the next page boundary.
     */
    if (cached == page_boundary)
    {
        /* Write through to page boundary */
        uint32_t address = FS_PRIV_SECTOR_ADDR(fs_priv_handle->curr_allocation_unit) +
                FS_PRIV_ALLOC_UNIT_SIZE + fs_priv_handle->last_data_offset;
        if (syshal_flash_write(fs_priv_handle->fs_priv->device,
                fs_priv_handle->page_cache,
                address,
                page_boundary))
            return FS_ERROR_FLASH_MEDIA;

        /* Advance last write position to the next page boundary */
        fs_priv_handle->last_data_offset += page_boundary;
    }

    return FS_NO_ERROR;
}

/* Exported Functions */

/*! \brief Initialize flash file system.
 *
 * Initializes the associated flash memory device and ensures all
 * file handles are marked as free.
 *
 * \param device[in] flash media device number to use for the file system
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA on flash initialization error.
 * \return \ref FS_ERROR_BAD_DEVICE device number out of range.
 */
int fs_init(uint32_t device)
{
    /* This device parameter is used as an index into the file
     * system device list -- we can't accept devices that are
     * out of range.
     */
    if (device >= FS_PRIV_MAX_DEVICES)
        return FS_ERROR_BAD_DEVICE;

    /* Mark all handles as free */
    for (unsigned int i = 0; i < FS_PRIV_MAX_HANDLES; i++)
        fs_priv_handle_list[i].assigned_index = FS_PRIV_INVALID_INDEX;

    return FS_NO_ERROR;
}

/*! \brief Terminates flash file system.
 *
 * Terminates the associated flash memory device.
 *
 * \param device[in] flash media device number to terminate
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA on flash terminate error.
 * \return \ref FS_ERROR_BAD_DEVICE device number out of range.
 */
int fs_term(uint32_t device)
{
    /* This device parameter is used as an index into the file
     * system device list -- we can't accept devices that are
     * out of range.
     */
    if (device >= FS_PRIV_MAX_DEVICES)
        return FS_ERROR_BAD_DEVICE;

    /* No action needed at present */

    return FS_NO_ERROR;
}

/*! \brief Mount a flash file system.
 *
 * The mount operation scans all sectors on the flash
 * memory and stores alloction unit headers locally to
 * keep track of the overall file system state.
 *
 * \param device[in] flash media device number to mount file system on
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_BAD_DEVICE if the device number is out of range
 */
int fs_mount(uint32_t device, fs_t *fs)
{
    int ret;

    ret = init_fs_priv(device);
    if (!ret)
        *fs = (fs_t)&fs_priv_list[device];

    return ret;
}

/*! \brief Format a flash file system.
 *
 * All flash media sectors shall be erased and locally stored
 * allocation unit headers shall also be reset.  Note that the
 * allocation counters, for wear levelling, are preserved and
 * incremented as per \ref erase_allocation_unit.
 *
 * \param fs[in] file system object to format
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA on flash media error.
 */
int fs_format(fs_t fs)
{
    int ret;
    fs_priv_t *fs_priv = (fs_priv_t *)fs;

    for (uint8_t sector = 0; sector < FS_PRIV_MAX_SECTORS; sector++)
    {
        ret = erase_allocation_unit(fs_priv, sector);
        if (ret) break;
    }

    return ret;
}

/*! \brief Open a file on the file system.
 *
 * Allows new files to be created or an existing file to
 * be opened.  Newly created files are created write only whereas an
 * existing file may be opened as read only or write only subject
 * to the \ref mode parameter.
 *
 * The \ref user_flags field allows application-specific bits to
 * be stored (created file) and retrieved (existing file).  Only
 * the lower nibble is stored.
 *
 * All files are identified by a unique \ref file_id.
 *
 * Upon opening a file, a file handle is allocated to the user
 * which then allows the \ref fs_read or \ref fs_write functions
 * to be used to access the file.  When writing to a file
 * the data contents can be flushed to the flash memory by
 * using the \ref fs_flush function.  When finished using a
 * file then \ref fs_close function should be called.
 *
 * \param fs[in] file system on which to open a file.
 * \param handle[out] pointer for storing allocated file handle.
 * \param file_id[in] file identifier to open.
 * \param mode[in] file open mode.
 * \param user_flags sets the user flags on create, otherwise
 * retrieves the user flags.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FILE_NOT_FOUND if the \ref file_id was not
 * found.
 * \return \ref FS_ERROR_FILE_ALREADY_EXISTS if the \ref file_id has
 * already been created.
 * \return \ref FS_ERROR_FILE_PROTECTED if the \ref file_id has
 * been protected and thus can not be opened in write mode.
 * \return \ref FS_ERROR_NO_FREE_HANDLE if no file handle could be
 * allocated.
 */
int fs_open(fs_t fs, fs_handle_t *handle, uint8_t file_id, fs_mode_t mode, uint8_t *user_flags)
{
    int ret;
    fs_priv_t *fs_priv = (fs_priv_t *)fs;
    fs_priv_handle_t *fs_priv_handle;
    *handle = FS_PRIV_INVALID_INDEX;

    /* Find the root allocation unit for this file (if file exists) */
    uint8_t root = find_file_root(fs_priv, file_id);

    /* Check file identifier versus requested open mode */
    ret = check_file_flags(fs_priv, root, mode);
    if (ret)
        return ret;

    /* Allocate a free handle */
    ret = allocate_handle(fs_priv, handle, &fs_priv_handle);
    if (ret)
        return ret;

    /* Reset file handle */
    fs_priv_handle->file_id = file_id;
    fs_priv_handle->root_allocation_unit = (uint8_t)FS_PRIV_NOT_ALLOCATED;

    if (root != (uint8_t)FS_PRIV_NOT_ALLOCATED)
    {
        /* Existing file: populate file handle */
        fs_priv_handle->root_allocation_unit = root;
        fs_priv_handle->flags.user_flags = get_user_flags(fs_priv, root);
        fs_priv_handle->flags.mode_flags = get_mode_flags(fs_priv, root) | mode;

        /* Retrieve existing user flags if requested */
        if (user_flags)
            *user_flags = fs_priv_handle->flags.user_flags;

        if ((mode & FS_FILE_WRITEABLE) == 0)
        {
            /* Read only - reset to beginning of root sector */
            fs_priv_handle->curr_data_offset = 0;
            fs_priv_handle->curr_allocation_unit = root;

            /* Find the last known write position in this sector so we
             * can check for when to advance to next sector or catch EOF
             */
            ret = find_end_of_sector(fs_priv, root,
                    &fs_priv_handle->last_data_offset, &fs_priv_handle->user_data_size);
            if (ret)
            {
                free_handle(*handle);
                return ret;
            }
        }
        else
        {
            /* Write only: find end of file for appending new data */
            ret = find_eof(fs_priv, root,
                    &fs_priv_handle->curr_allocation_unit,
                    &fs_priv_handle->curr_data_offset,
                    &fs_priv_handle->user_data_size);
            if (ret)
            {
                free_handle(*handle);
                return ret;
            }

            /* Page write cache should be marked empty */
            fs_priv_handle->last_data_offset = fs_priv_handle->curr_data_offset;
        }
    }
    else
    {
        /* Set file flags since we are creating a new file */
        fs_priv_handle->flags.mode_flags = mode;
        fs_priv_handle->flags.user_flags = user_flags ? *user_flags : 0;

        /* Allocate new sector to file handle */
        ret = allocate_new_sector_to_file(fs_priv_handle);
        if (ret)
            free_handle(*handle);
    }

    return ret;
}

/*! \brief Close an open file handle.
 *
 * Shall try to execute a flush operation and then frees
 * the file handle.  Note that any error during the
 * flush operation shall not be seen at this level so
 * it is advisable to flush separately first before
 * closing the file handle.
 *
 * \param handle[in] file handle to close.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_INVALID_HANDLE if the given handle is invalid.
 */
int fs_close(fs_handle_t handle)
{
    if (fs_priv_handle_list[handle % FS_PRIV_MAX_HANDLES].assigned_index != handle)
        return FS_ERROR_INVALID_HANDLE;

    fs_flush(handle);
    free_handle(handle);

    return FS_NO_ERROR;
}

/*! \brief Write data to an open file handle.
 *
 * Data bytes shall be written to the file handle via
 * a page cache.  The page cache will write-through to
 * flash memory whenever current write pointer reaches a
 * page boundary and there is data in the cache.
 *
 * The session write pointer is not stored to flash unless the current
 * sector is filled whereupon a new sector is allocated.  Use
 * \ref fs_flush to periodically store the session write pointer
 * to flash.
 *
 * The write process may continue indefinitely if the file
 * type is circular.  In the case of a circular file type,
 * the root sector is reclaimed.  Otherwise, once all
 * sectors have been exhausted a \ref FS_ERROR_FILESYSTEM_FULL
 * error is returned.
 *
 * \param handle[in] file handle to write to.
 * \param src[in] pointer to memory containing bytes to write.
 * \param size[in] number of bytes to write.
 * \param written[out] pointer for storing number of bytes actually
 * written, which may be less than the amount requested.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA on flash write error.
 * \return \ref FS_ERROR_FILESYSTEM_FULL unable to write all data as the
 * file system is full.
 * \return \ref FS_ERROR_INVALID_HANDLE if the given handle is invalid.
 */
int fs_write(fs_handle_t handle, const void *src, uint32_t size, uint32_t *written)
{
    fs_priv_handle_t *fs_priv_handle;
    int ret = FS_NO_ERROR;

    if (!get_handle_struct(handle, &fs_priv_handle))
        return FS_ERROR_INVALID_HANDLE;

    /* Reset counter */
    *written = 0;

    /* Check the file is writable */
    if ((fs_priv_handle->flags.mode_flags & FS_FILE_WRITEABLE) == 0)
        return FS_ERROR_INVALID_MODE;

    while (size > 0 && !ret)
    {
        uint32_t actual_write = 0;

        /* Write data through the cache.  Note that the actual write size
         * could be less than requested so we should keep looping.
         * The 'write_through_cache' function will handle reaching
         * the end of the current sector and allocating a new sector.
         */
        ret = write_through_cache(fs_priv_handle, src, size, &actual_write);
        src = ((uint8_t *)src) + actual_write;
        size -= actual_write;
        *written += actual_write;
    }

    return ret;
}

/*! \brief Read data from an open file handle.
 *
 * The requested number of bytes shall be read from the file.
 * The file's read position shall be updated accordingly.
 * The actual number of bytes read may be less than the number of
 * bytes requested e.g., EOF.
 *
 * \param handle[in] file handle to read.
 * \param dest[out] pointer to buffer for storing read data.
 * \param size[in] number of bytes to read.
 * \param read[out] pointer for storing number of bytes actually read.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA if a flash read failed.
 * \return \ref FS_ERROR_INVALID_MODE if the file handle is not readable.
 * \return \ref FS_ERROR_END_OF_FILE if the end of file has already been reached.
 * \return \ref FS_ERROR_INVALID_HANDLE if the given handle is invalid.
 */
__RAMFUNC int fs_read(fs_handle_t handle, void *dest, uint32_t size, uint32_t *read)
{
    unsigned int last_read, zero_read_iter = 0;
    fs_priv_handle_t *fs_priv_handle;
    int ret;

    if (!get_handle_struct(handle, &fs_priv_handle))
        return FS_ERROR_INVALID_HANDLE;

    fs_priv_t *fs_priv = fs_priv_handle->fs_priv;

    /* Reset counter */
    *read = 0;

    /* Check the file is read only */
    if (fs_priv_handle->flags.mode_flags & FS_FILE_WRITEABLE)
        return FS_ERROR_INVALID_MODE;

    /* Check for end of file */
    if (is_eof(fs_priv_handle))
        return FS_ERROR_END_OF_FILE;

    while (size > 0)
    {
        if (fs_priv_handle->last_data_offset <= fs_priv_handle->curr_data_offset)
        {
            /* Check if we reached the end of the file chain */
            if (is_last_allocation_unit(fs_priv, fs_priv_handle->curr_allocation_unit))
                break;

            /* Not the end of the file chain */
            uint8_t sector = next_allocation_unit(fs_priv, fs_priv_handle->curr_allocation_unit);

            /* Find the last known write position in this sector so we
             * can check for when to advance to next sector or catch EOF
             */
            ret = find_end_of_sector(fs_priv, sector, &fs_priv_handle->last_data_offset, NULL);
            if (ret) return ret;

            /* Reset data offset pointer */
            fs_priv_handle->curr_allocation_unit = sector;
            fs_priv_handle->curr_data_offset = 0;
        }

        uint16_t read_offset = 0;
        uint16_t write_offset = fs_priv_handle->curr_data_offset % FS_PRIV_PAGE_SIZE;

        /* Read current page into working buffer */
        if (0 == write_offset)
        {
            uint32_t address = FS_PRIV_SECTOR_ADDR(fs_priv_handle->curr_allocation_unit) +
                    FS_PRIV_FILE_DATA_REL_ADDRESS + (fs_priv_handle->curr_data_offset & ~(FS_PRIV_PAGE_SIZE-1));
            if (syshal_flash_read(fs_priv->device,
                    fs_priv_handle->page_cache,
                    address,
                    FS_PRIV_PAGE_SIZE))
                return FS_ERROR_FLASH_MEDIA;
        }

        /* Iterate through the page's stored write operations -- each page will typically
         * look something like this:
         *
         *   0   1                i  i+1               i+j            511
         * +-------------------------------------------------------------+
         * |   |   |            |   |   |              |   |     |   |   |
         * | i | i | ...Data... | j | j | ...Data...   |FF | ... |FF |FF |
         * |   |   |            |   |   |              |   |     |   |   |
         * +-------------------------------------------------------------+
         *
         * This page contains two write operations of length i and j respectively.
         * Two bytes are used to store the write operation length (which includes
         * the header itself).
         */
        last_read = 0;
        while (read_offset < (FS_PRIV_PAGE_SIZE - 2) && size)
        {
            /* The read_offset should be aligned to the next operation in the page */
            uint16_t header_len = ((uint16_t)fs_priv_handle->page_cache[read_offset] |
                    ((uint16_t)fs_priv_handle->page_cache[read_offset + 1] << 8));

            /* If the header length is FFFF then we assume we reached EOF */
            if (0xFFFF == header_len)
            {
                /* Assume we reached EOF */
                break;
            }

            /* Make sure the header field is within acceptable bounds */
            if (header_len <= 2 ||
                    (read_offset + header_len) > FS_PRIV_PAGE_SIZE ||
                    header_len > FS_PRIV_PAGE_SIZE
            )
            {
                return FS_ERROR_FILESYSTEM_CORRUPTED;
            }

            /* Check to see if we already read these bytes from the page */
            if ((read_offset + header_len) > write_offset)
            {
                /* We didn't read at least some of these bytes, so compute
                 * how many bytes to read.
                 */
                uint16_t available = (read_offset + header_len) - write_offset;

                /* If this is the first read for this header, then we must skip
                 * over the header bytes which are not part of the user data.
                 */
                if (write_offset == read_offset)
                {
                    /* Skip header bytes */
                    available -= 2;
                    write_offset += 2;
                    fs_priv_handle->curr_data_offset += 2;
                }

                /* Copy into user data buffer from page buffer */
                uint16_t sz = MIN(size, available);
                //memcpy(dest, &fs_priv_handle->page_cache[write_offset], sz);
                for (unsigned int i = 0; i < sz; i++)
                    ((char *)dest)[i] = fs_priv_handle->page_cache[write_offset+i];
                size -= sz;
                *read += sz;
                last_read += sz;
                dest = ((char *)dest) + sz;
                write_offset += sz;
                fs_priv_handle->curr_data_offset += sz;
            }

            /* Skip to next header in this page (if any) */
            read_offset += header_len;
        }

        /* Ensure any surplus bytes in the page are skipped */
        if (size)
        {
            fs_priv_handle->curr_data_offset += (FS_PRIV_PAGE_SIZE - write_offset);
        }

        /* Check for zero read bytes condition */
        if (last_read == 0)
        {
            zero_read_iter++;
            if (zero_read_iter >= 3)
                return FS_ERROR_FILESYSTEM_CORRUPTED;
        }
        else
        {
            zero_read_iter = 0;
        }
    }

    return FS_NO_ERROR;
}

/*! \brief Relative seek forwards in an open file handle.
 *
 * The file's read position shall be updated accordingly.
 *
 * \param handle[in] file handle to read.
 * \param seek[in] number of bytes to advance.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA if a flash read failed.
 * \return \ref FS_ERROR_INVALID_MODE if the file handle is not readable.
 * \return \ref FS_ERROR_END_OF_FILE if the end of file was reached.
 * \return \ref FS_ERROR_INVALID_HANDLE if the given handle is invalid.
 */
int fs_seek(fs_handle_t handle, uint32_t seek)
{
    fs_priv_handle_t *fs_priv_handle;
    int ret;

    if (!get_handle_struct(handle, &fs_priv_handle))
        return FS_ERROR_INVALID_HANDLE;

    fs_priv_t *fs_priv = fs_priv_handle->fs_priv;

    /* Check the file is read only */
    if (fs_priv_handle->flags.mode_flags & FS_FILE_WRITEABLE)
        return FS_ERROR_INVALID_MODE;

    /* Check for end of file */
    if (is_eof(fs_priv_handle))
        return FS_ERROR_END_OF_FILE;

    while (seek > 0)
    {
        /* Check to see if we need to move to the next sector in the file chain */
        if (fs_priv_handle->last_data_offset <= fs_priv_handle->curr_data_offset)
        {
            /* Check if we reached the end of the file chain */
            if (is_last_allocation_unit(fs_priv, fs_priv_handle->curr_allocation_unit))
                break;

            /* Not the end of the file chain */
            uint8_t sector = next_allocation_unit(fs_priv, fs_priv_handle->curr_allocation_unit);

            /* Find the last known write position in this sector so we
             * can check for when to advance to next sector or catch EOF
             */
            ret = find_end_of_sector(fs_priv, sector, &fs_priv_handle->last_data_offset, NULL);
            if (ret) return ret;

            /* Reset data offset pointer */
            fs_priv_handle->curr_allocation_unit = sector;
            fs_priv_handle->curr_data_offset = 0;
        }

        /* Read current page into working buffer */
        uint32_t address = FS_PRIV_SECTOR_ADDR(fs_priv_handle->curr_allocation_unit) +
                FS_PRIV_FILE_DATA_REL_ADDRESS + (fs_priv_handle->curr_data_offset & ~(FS_PRIV_PAGE_SIZE-1));
        if (syshal_flash_read(fs_priv->device,
                fs_priv_handle->page_cache,
                address,
                FS_PRIV_PAGE_SIZE))
            return FS_ERROR_FLASH_MEDIA;
        uint16_t read_offset = 0;
        uint16_t write_offset = fs_priv_handle->curr_data_offset % FS_PRIV_PAGE_SIZE;

        /* Iterate through the page's stored write operations -- each page will typically
         * look something like this:
         *
         *   0   1                i  i+1               i+j            511
         * +-------------------------------------------------------------+
         * |   |   |            |   |   |              |   |     |   |   |
         * | i | i |  ..Data... | j | j | ...Data...   |FF | ... |FF |FF |
         * |   |   |            |   |   |              |   |     |   |   |
         * +-------------------------------------------------------------+
         *
         * This page contains two write operations of length i and j respectively.
         * Two bytes are used to store the write operation length (which includes
         * the header itself).
         */
        while (read_offset < (FS_PRIV_PAGE_SIZE - 2) && seek)
        {
            /* The read_offset should be aligned to the next operation in the page */
            uint16_t header_len = ((uint16_t)fs_priv_handle->page_cache[read_offset] |
                    ((uint16_t)fs_priv_handle->page_cache[read_offset + 1] << 8));

            /* If the header length is FFFF then we assume we reached EOF */
            if (0xFFFF == header_len)
            {
                /* Assume we reached EOF */
                break;
            }

            /* Make sure the header field is within acceptable bounds */
            if (header_len <= 2 ||
                    (read_offset + header_len) > FS_PRIV_PAGE_SIZE ||
                    header_len > FS_PRIV_PAGE_SIZE
            )
            {
                return FS_ERROR_FILESYSTEM_CORRUPTED;
            }

            /* Check to see if we already read these bytes from the page */
            if ((read_offset + header_len) > write_offset)
            {
                /* We didn't read at least some of these bytes, so compute
                 * how many bytes to read.
                 */
                uint16_t available = (read_offset + header_len) - write_offset;

                /* If this is the first read for this header, then we must skip
                 * over the header bytes which are not part of the user data.
                 */
                if (write_offset == read_offset)
                {
                    /* Skip header bytes */
                    available -= 2;
                    write_offset += 2;
                    fs_priv_handle->curr_data_offset += 2;
                }

                /* Advance file position */
                uint16_t sz = MIN(seek, available);
                seek -= sz;
                write_offset += sz;
                fs_priv_handle->curr_data_offset += sz;
            }

            /* Skip to next header in this page (if any) */
            read_offset += header_len;
        }

        /* Ensure any surplus bytes in the page are skipped */
        if (seek)
        {
            fs_priv_handle->curr_data_offset += (FS_PRIV_PAGE_SIZE - write_offset);
        }
    }

    return FS_NO_ERROR;
}

/*! \brief Flush an open file.
 *
 * Shall result in any cached data being purged from the
 * page cache and being written to the underlying flash memory.
 * The current session information shall also be stored to
 * flash memory to reflect a new write position, if
 * changed.
 *
 * \param handle[in] file handle to flush.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FLASH_MEDIA if a flash write failed.
 * \return \ref FS_ERROR_INVALID_MODE if the file handle is not writeable.
 * \return \ref FS_ERROR_FILE_PROTECTED if the file is protected.
 * \return \ref FS_ERROR_INVALID_HANDLE if the given handle is invalid.
 */
int fs_flush(fs_handle_t handle)
{
    fs_priv_handle_t *fs_priv_handle;

    if (!get_handle_struct(handle, &fs_priv_handle))
        return FS_ERROR_INVALID_HANDLE;

    /* Make sure the file is writeable */
    if ((fs_priv_handle->flags.mode_flags & FS_FILE_WRITEABLE) == 0)
        return FS_ERROR_INVALID_MODE;

    /* Flush the handle */
    return flush_handle(fs_priv_handle);
}

/*! \brief Protect a file from the file system.
 *
 * If \ref file_id references a file that can be found on
 * the flash memory and the file is not protected, then
 * the file's protection bits shall be modified.
 *
 * \param fs[in] file system on which to perform protect operation.
 * \param file_id[in] file identifier to protect.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FILE_NOT_FOUND if the \ref file_id was not found.
 * \return \ref FS_ERROR_FLASH_MEDIA if a flash write failed.
 */
int fs_protect(fs_t fs, uint8_t file_id)
{
    fs_priv_t *fs_priv = (fs_priv_t *)fs;

    /* Find the root allocation unit for this file */
    uint8_t root = find_file_root(fs_priv, file_id);
    if ((uint8_t)FS_PRIV_NOT_ALLOCATED == root)
        return FS_ERROR_FILE_NOT_FOUND;

    /* No action needed if already protected */
    if (is_protected(get_file_protect(fs_priv, root)))
        return FS_NO_ERROR;

    uint8_t file_protect = get_file_protect(fs_priv, root);
    file_protect = set_protected(true, file_protect);

    /* Write updated file protect bits to flash */
    if (syshal_flash_write(fs_priv->device, &file_protect,
            FS_PRIV_SECTOR_ADDR(root) + FS_PRIV_FILE_PROTECT_OFFSET,
            sizeof(uint8_t)))
        return FS_ERROR_FLASH_MEDIA;

    set_file_protect(fs_priv, root, file_protect);

    return FS_NO_ERROR;
}

/*! \brief Unprotect a file from the file system.
 *
 * If \ref file_id references a file that can be found on
 * the flash memory and the file is protected, then
 * the file's protection bits shall be modified.
 *
 * \param fs[in] file system on which to perform unprotect operation.
 * \param file_id[in] file identifier to unprotect.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FILE_NOT_FOUND if the \ref file_id was not found.
 * \return \ref FS_ERROR_FLASH_MEDIA if a flash write failed.
 */
int fs_unprotect(fs_t fs, uint8_t file_id)
{
    fs_priv_t *fs_priv = (fs_priv_t *)fs;

    /* Find the root allocation unit for this file */
    uint8_t root = find_file_root(fs_priv, file_id);
    if ((uint8_t)FS_PRIV_NOT_ALLOCATED == root)
        return FS_ERROR_FILE_NOT_FOUND;

    /* No action needed if already unprotected */
    if (!is_protected(get_file_protect(fs_priv, root)))
        return FS_NO_ERROR;

    uint8_t file_protect = get_file_protect(fs_priv, root);
    file_protect = set_protected(false, file_protect);

    /* Write updated file protect bits to flash */
    if (syshal_flash_write(fs_priv->device, &file_protect,
            FS_PRIV_SECTOR_ADDR(root) + FS_PRIV_FILE_PROTECT_OFFSET,
            sizeof(uint8_t)))
        return FS_ERROR_FLASH_MEDIA;

    set_file_protect(fs_priv, root, file_protect);

    return FS_NO_ERROR;
}

/*! \brief Delete a file from the file system.
 *
 * If \ref file_id references a file that can be found on
 * the flash memory and the file is not protected, then
 * the file shall be removed.  This entails erasing every
 * sector in flash associated with the file.
 *
 * \param fs[in] file system on which to perform file delete.
 * \param file_id[in] file identifier to delete.
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FILE_NOT_FOUND if the \ref file_id was not found.
 * \return \ref FS_ERROR_FILE_PROTECTED if the file is protected.
 */
int fs_delete(fs_t fs, uint8_t file_id)
{
    int ret;
    fs_priv_t *fs_priv = (fs_priv_t *)fs;

    /* Find the root allocation unit for this file */
    uint8_t root = find_file_root(fs_priv, file_id);
    if ((uint8_t)FS_PRIV_NOT_ALLOCATED == root)
        return FS_ERROR_FILE_NOT_FOUND;

    /* Make sure the file is not protected */
    if (is_protected(get_file_protect(fs_priv, root)))
        return FS_ERROR_FILE_PROTECTED;

    /* Erase each allocation unit associated with the file */
    while ((uint8_t)FS_PRIV_NOT_ALLOCATED != root)
    {
        uint8_t temp = root;

        /* Grab next sector in file chain before erasing this one */
        root = next_allocation_unit(fs_priv, root);

        /* This will erase both the flash sector and the local copy
         * of the allocation unit's header.
         */
        ret = erase_allocation_unit(fs_priv, temp);
        if (ret)
            return ret;
    }

    return FS_NO_ERROR;
}

/*! \brief Obtain file or file system status.
 *
 * If \ref file_id is set to \ref FS_FILE_ID_NONE then the operation
 * shall be performed with respect to the file system.  The only
 * field populated shall be \ref fs_stat_t.size and this shall
 * reflect the total number of free bytes in the file system.
 *
 * Otherwise, the specific \ref file_id is retrieved and its
 * total size is scanned along with the file's attributes.
 *
 * \param fs[in] file system on which to perform stat operation.
 * \param file_id[in] shall be FS_FILE_ID_NONE or a valid file identifier.
 * \param stat[out] pointer to file status structure
 * \return \ref FS_NO_ERROR on success.
 * \return \ref FS_ERROR_FILE_NOT_FOUND if the \ref file_id was not
 * found.
 */
int fs_stat(fs_t fs, uint8_t file_id, fs_stat_t *stat)
{
    fs_priv_t *fs_priv = (fs_priv_t *)fs;

    if (FS_FILE_ID_NONE == file_id)
    {
        stat->size = 0; /* Reset size to zero before we start */

        /* Find the total amount of free space on the disk */
        for (uint8_t sector = 0; sector < FS_PRIV_MAX_SECTORS; sector++)
        {
            if (get_file_id(fs_priv, sector) == (uint8_t)FS_PRIV_NOT_ALLOCATED)
            {
                /* Spare sector, so increment by the sector size less the
                 * space we allocate for the file allocation unit management.
                 */
                stat->size += (FS_PRIV_SECTOR_SIZE - FS_PRIV_ALLOC_UNIT_SIZE);
            }
            else
            {
                /* Used sector, so retrieve how much data in the sector has
                 * been used and deduct this from the sector size.
                 */
                uint32_t data_offset;
                if (find_end_of_sector(fs_priv, sector, &data_offset, NULL) == FS_NO_ERROR)
                    stat->size += ((FS_PRIV_SECTOR_SIZE - FS_PRIV_ALLOC_UNIT_SIZE) -
                            data_offset);
                else
                    return FS_ERROR_FILESYSTEM_CORRUPTED;
            }
        }
    }
    else
    {
        /* A file identifier was provided, so find its root sector */
        uint8_t root = find_file_root(fs_priv, file_id);
        if ((uint8_t)FS_PRIV_NOT_ALLOCATED == root)
            return FS_ERROR_FILE_NOT_FOUND;

        /* These fields are all in the header of the root sector */
        stat->is_circular = get_mode_flags(fs_priv, root);
        stat->user_flags = get_user_flags(fs_priv, root);
        stat->is_protected = is_protected(get_file_protect(fs_priv, root));

        /* We have to compute the size of the file on the disk iteratively by
         * visiting each sector in the file chain.
         */
        stat->size = 0; /* Reset size to zero before we start */

        while ((uint8_t)FS_PRIV_NOT_ALLOCATED != root)
        {
            uint32_t sector_size;
            uint32_t data_offset;
            if (find_end_of_sector(fs_priv, root, &data_offset, &sector_size) == FS_NO_ERROR)
            {
                stat->size += sector_size;
            }
            else
                return FS_ERROR_FILESYSTEM_CORRUPTED;
            root = next_allocation_unit(fs_priv, root);
        }
    }

    return FS_NO_ERROR;
}
