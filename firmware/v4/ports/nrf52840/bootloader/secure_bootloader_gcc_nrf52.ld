/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0xec000, LENGTH = 0x12000
  RAM (rwx) :  ORIGIN = 0x20005100, LENGTH = 0x3AF00
  uicr_bootloader_start_address (r) : ORIGIN = 0x10001014, LENGTH = 0x4
  uicr_mbr_params_page (r) : ORIGIN = 0x10001018, LENGTH = 0x4
  bootloader_version (r) : ORIGIN = 0x000FDFFC, LENGTH = 0x4
  mbr_params_page (r) : ORIGIN = 0x000FE000, LENGTH = 0x1000
  bootloader_settings_page (r) : ORIGIN = 0x000FF000, LENGTH = 0x1000
}

SECTIONS
{
  .uicr_bootloader_start_address :
  {
    PROVIDE(__start_uicr_bootloader_start_address = .);
    KEEP(*(SORT(.uicr_bootloader_start_address*)))
    PROVIDE(__stop_uicr_bootloader_start_address = .);
  } > uicr_bootloader_start_address
  .bootloader_version :
  {
    KEEP(*(.bootloader_version));
  } > bootloader_version
  .mbr_params_page(NOLOAD) :
  {
    PROVIDE(__start_mbr_params_page = .);
    KEEP(*(SORT(.mbr_params_page*)))
    PROVIDE(__stop_mbr_params_page = .);
  } > mbr_params_page
  .bootloader_settings_page(NOLOAD) :
  {
    PROVIDE(__start_bootloader_settings_page = .);
    KEEP(*(SORT(.bootloader_settings_page*)))
    PROVIDE(__stop_bootloader_settings_page = .);
  } > bootloader_settings_page
  .uicr_mbr_params_page :
  {
    PROVIDE(__start_uicr_mbr_params_page = .);
    KEEP(*(SORT(.uicr_mbr_params_page*)))
    PROVIDE(__stop_uicr_mbr_params_page = .);
  } > uicr_mbr_params_page
}

SECTIONS
{
  . = ALIGN(4);
  .mem_section_dummy_ram :
  {
  }
  .fs_data :
  {
    PROVIDE(__start_fs_data = .);
    KEEP(*(.fs_data))
    PROVIDE(__stop_fs_data = .);
  } > RAM
  .log_dynamic_data :
  {
    PROVIDE(__start_log_dynamic_data = .);
    KEEP(*(SORT(.log_dynamic_data*)))
    PROVIDE(__stop_log_dynamic_data = .);
  } > RAM
  .log_filter_data :
  {
    PROVIDE(__start_log_filter_data = .);
    KEEP(*(SORT(.log_filter_data*)))
    PROVIDE(__stop_log_filter_data = .);
  } > RAM

} INSERT AFTER .data;

SECTIONS
{
  .mem_section_dummy_rom :
  {
  }
  .crypto_data :
  {
    PROVIDE(__start_crypto_data = .);
    KEEP(*(SORT(.crypto_data*)))
    PROVIDE(__stop_crypto_data = .);
  } > FLASH
  .dfu_trans :
  {
    PROVIDE(__start_dfu_trans = .);
    KEEP(*(SORT(.dfu_trans*)))
    PROVIDE(__stop_dfu_trans = .);
  } > FLASH
    .nrf_queue :
  {
    PROVIDE(__start_nrf_queue = .);
    KEEP(*(.nrf_queue))
    PROVIDE(__stop_nrf_queue = .);
  } > FLASH
  .log_const_data :
  {
    PROVIDE(__start_log_const_data = .);
    KEEP(*(SORT(.log_const_data*)))
    PROVIDE(__stop_log_const_data = .);
  } > FLASH
    .nrf_balloc :
  {
    PROVIDE(__start_nrf_balloc = .);
    KEEP(*(.nrf_balloc))
    PROVIDE(__stop_nrf_balloc = .);
  } > FLASH
  .log_backends :
  {
    PROVIDE(__start_log_backends = .);
    KEEP(*(SORT(.log_backends*)))
    PROVIDE(__stop_log_backends = .);
  } > FLASH

} INSERT AFTER .text

INCLUDE "nrf_common.ld"
