cmake_minimum_required(VERSION 3.0)

project(Horizon
    VERSION 1.2.2.0
    LANGUAGES C ASM
)

# Construct the version string
if(${PROJECT_VERSION_PATCH} MATCHES 0)
    set(VERSION_TYPE_STR "a")  # Alpha
elseif(${PROJECT_VERSION_PATCH} MATCHES 1)
    set(VERSION_TYPE_STR "b")  # Beta
elseif(${PROJECT_VERSION_PATCH} MATCHES 2)
    set(VERSION_TYPE_STR "rc") # Release candidate
elseif(${PROJECT_VERSION_PATCH} MATCHES 3)
    set(VERSION_TYPE_STR "r")  # Release
else()
    message(FATAL_ERROR "Invalid project version")
endif()

set(VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}${VERSION_TYPE_STR})
if(NOT ${PROJECT_VERSION_TWEAK} MATCHES 0)
    set(VERSION_STRING ${VERSION_STRING}${PROJECT_VERSION_TWEAK}) # e.g. produce 8.0rc instead of 8.0rc0 but permit 8.0rc1
endif()

# Pack the version information into 32-bits
math(EXPR VERSION_NUMBER "((${PROJECT_VERSION_MAJOR} % 255) << 24) | ((${PROJECT_VERSION_MINOR} % 255) << 16) | ((${PROJECT_VERSION_PATCH} % 255) << 8) | (${PROJECT_VERSION_TWEAK} % 255)")

set(BOARD "horizon_v3.0")

set(SDK_ROOT      "../drivers/nRF5_SDK_15.3.0")
set(OLD_SDK_ROOT  "../drivers/nRF5_SDK_15.2.0")
set(PROJ_DIR      ".")
set(CORE_DIR      "../../../core")
set(SYSHAL_DIR    "../../../syshal")

set(SRCS
    "${CORE_DIR}/debug/debug.c"
    "${PROJ_DIR}/main.c"
    "${PROJ_DIR}/public_key.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/dfu-cc.pb.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_flash.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_handling_error.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_mbr.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_req_handler.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_settings.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_transport.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_utils.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_validation.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_ver_validation.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/nrf_bootloader.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/nrf_bootloader_app_start.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/nrf_bootloader_dfu_timers.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/nrf_bootloader_fw_activation.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/nrf_bootloader_info.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/nrf_bootloader_wdt.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/serial_dfu/nrf_dfu_serial.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/serial_dfu/nrf_dfu_serial_usb.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/cc310_bl/cc310_bl_backend_ecc.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/cc310_bl/cc310_bl_backend_ecdsa.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/cc310_bl/cc310_bl_backend_hash.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/cc310_bl/cc310_bl_backend_init.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/cc310_bl/cc310_bl_backend_shared.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/oberon/oberon_backend_chacha_poly_aead.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/oberon/oberon_backend_ecc.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/oberon/oberon_backend_ecdh.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/oberon/oberon_backend_ecdsa.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/oberon/oberon_backend_eddsa.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/oberon/oberon_backend_hash.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/oberon/oberon_backend_hmac.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/nrf_crypto_ecc.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/nrf_crypto_ecdsa.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/nrf_crypto_hash.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/nrf_crypto_init.c"
    "${OLD_SDK_ROOT}/components/libraries/crypto/nrf_crypto_shared.c"
    "${SDK_ROOT}/components/boards/boards.c"
    "${SDK_ROOT}/components/drivers_nrf/nrf_soc_nosd/nrf_nvic.c"
    "${SDK_ROOT}/components/drivers_nrf/nrf_soc_nosd/nrf_soc.c"
    "${SDK_ROOT}/components/libraries/atomic/nrf_atomic.c"
    "${SDK_ROOT}/components/libraries/atomic_fifo/nrf_atfifo.c"
    "${SDK_ROOT}/components/libraries/balloc/nrf_balloc.c"
    "${SDK_ROOT}/components/libraries/crc32/crc32.c"
    "${SDK_ROOT}/components/libraries/fstorage/nrf_fstorage.c"
    "${SDK_ROOT}/components/libraries/fstorage/nrf_fstorage_nvmc.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_backend_rtt.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_backend_serial.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_backend_uart.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_default_backends.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_frontend.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_str_formatter.c"
    "${SDK_ROOT}/components/libraries/mem_manager/mem_manager.c"
    "${SDK_ROOT}/components/libraries/memobj/nrf_memobj.c"
    "${SDK_ROOT}/components/libraries/queue/nrf_queue.c"
    "${SDK_ROOT}/components/libraries/ringbuf/nrf_ringbuf.c"
    "${SDK_ROOT}/components/libraries/scheduler/app_scheduler.c"
    "${SDK_ROOT}/components/libraries/slip/slip.c"
    "${SDK_ROOT}/components/libraries/strerror/nrf_strerror.c"
    "${SDK_ROOT}/components/libraries/usbd/app_usbd.c"
    "${SDK_ROOT}/components/libraries/usbd/app_usbd_core.c"
    "${SDK_ROOT}/components/libraries/usbd/app_usbd_serial_num.c"
    "${SDK_ROOT}/components/libraries/usbd/app_usbd_string_desc.c"
    "${SDK_ROOT}/components/libraries/usbd/class/cdc/acm/app_usbd_cdc_acm.c"
    "${SDK_ROOT}/components/libraries/util/app_error_weak.c"
    "${SDK_ROOT}/components/libraries/util/app_util_platform.c"
    "${SDK_ROOT}/components/libraries/util/nrf_assert.c"
    "${SDK_ROOT}/external/fprintf/nrf_fprintf.c"
    "${SDK_ROOT}/external/fprintf/nrf_fprintf_format.c"
    "${SDK_ROOT}/external/nano-pb/pb_common.c"
    "${SDK_ROOT}/external/nano-pb/pb_decode.c"
    "${SDK_ROOT}/external/segger_rtt/SEGGER_RTT.c"
    "${SDK_ROOT}/external/segger_rtt/SEGGER_RTT_printf.c"
    "${SDK_ROOT}/external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c"
    "${SDK_ROOT}/external/utf_converter/utf.c"
    "${SDK_ROOT}/integration/nrfx/legacy/nrf_drv_clock.c"
    "${SDK_ROOT}/integration/nrfx/legacy/nrf_drv_power.c"
    "${SDK_ROOT}/integration/nrfx/legacy/nrf_drv_uart.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_clock.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_power.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_systick.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_timer.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_uart.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_uarte.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_usbd.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/prs/nrfx_prs.c"
    "${SDK_ROOT}/modules/nrfx/hal/nrf_nvmc.c"
    "${SDK_ROOT}/modules/nrfx/mdk/gcc_startup_nrf52840.S"
    "${SDK_ROOT}/modules/nrfx/mdk/system_nrf52840.c"
    "${SDK_ROOT}/modules/nrfx/soc/nrfx_atomic.c"
)

include_directories(
    "${PROJ_DIR}"
    "${CORE_DIR}/debug"
    "${SYSHAL_DIR}/inc"
    "${OLD_SDK_ROOT}/components/libraries/bootloader"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/serial_dfu"
    "${OLD_SDK_ROOT}/components/libraries/crypto"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/cc310"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/cc310_bl"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/cifra"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/mbedtls"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/micro_ecc"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/nrf_hw"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/nrf_sw"
    "${OLD_SDK_ROOT}/components/libraries/crypto/backend/oberon"
    "${SDK_ROOT}/components/boards"
    "${SDK_ROOT}/components/drivers_nrf/nrf_soc_nosd"
    "${SDK_ROOT}/components/libraries/atomic"
    "${SDK_ROOT}/components/libraries/atomic_fifo"
    "${SDK_ROOT}/components/libraries/balloc"
    "${SDK_ROOT}/components/libraries/crc32"
    "${SDK_ROOT}/components/libraries/delay"
    "${SDK_ROOT}/components/libraries/experimental_section_vars"
    "${SDK_ROOT}/components/libraries/fstorage"
    "${SDK_ROOT}/components/libraries/log"
    "${SDK_ROOT}/components/libraries/log/src"
    "${SDK_ROOT}/components/libraries/mem_manager"
    "${SDK_ROOT}/components/libraries/memobj"
    "${SDK_ROOT}/components/libraries/mutex"
    "${SDK_ROOT}/components/libraries/queue"
    "${SDK_ROOT}/components/libraries/ringbuf"
    "${SDK_ROOT}/components/libraries/scheduler"
    "${SDK_ROOT}/components/libraries/slip"
    "${SDK_ROOT}/components/libraries/stack_info"
    "${SDK_ROOT}/components/libraries/strerror"
    "${SDK_ROOT}/components/libraries/usbd"
    "${SDK_ROOT}/components/libraries/usbd/class/cdc"
    "${SDK_ROOT}/components/libraries/usbd/class/cdc/acm"
    "${SDK_ROOT}/components/libraries/util"
    "${SDK_ROOT}/components/softdevice/mbr/nrf52840/headers"
    "${SDK_ROOT}/components/toolchain/cmsis/include"
    "${SDK_ROOT}/external/fprintf"
    "${SDK_ROOT}/external/nano-pb"
    "${SDK_ROOT}/external/nrf_cc310/include"
    "${SDK_ROOT}/external/nrf_cc310_bl/include"
    "${SDK_ROOT}/external/nrf_oberon"
    "${SDK_ROOT}/external/nrf_oberon/include"
    "${SDK_ROOT}/external/segger_rtt"
    "${SDK_ROOT}/external/utf_converter"
    "${SDK_ROOT}/integration/nrfx"
    "${SDK_ROOT}/integration/nrfx/legacy"
    "${SDK_ROOT}/modules/nrfx"
    "${SDK_ROOT}/modules/nrfx/drivers/include"
    "${SDK_ROOT}/modules/nrfx/hal"
    "${SDK_ROOT}/modules/nrfx/mdk"
)

add_definitions(
    "-DBOARD_PCA10056"
    "-DCONFIG_GPIO_AS_PINRESET"
    "-DDEBUG_NRF"
    "-DDEBUG_DISABLED"
    "-DFLOAT_ABI_HARD"
    "-DNRF52840_XXAA"
    "-DNRF_DFU_SETTINGS_VERSION=1"
    "-DSVC_INTERFACE_CALL_AS_NORMAL_FUNCTION"
    "-DBOOTLOADER_FIRMWARE_VERSION=${VERSION_NUMBER}"
)

set(target_name horizon_bootloader_v${VERSION_STRING})

set(CMAKE_C_FLAGS
    # Warnings
    "-Wall"
    "-Wextra"
    "-Werror "
    "-Wduplicated-cond"
    "-Wduplicated-branches"
    "-Wlogical-op"
    "-Wnull-dereference"
    "-Wjump-misses-init"
    "-Wshadow"
    "-Wpointer-arith"
    "-Wdangling-else"
    "-Wrestrict"
    "-Wdouble-promotion"
    "-Wvla"
    # Disabled Warnings
    "-Wno-expansion-to-defined"
    "-Wno-unused-parameter"
    "-Wno-clobbered"

    # Build flags
    "-mcpu=cortex-m4"
    "-mthumb"
    "-mabi=aapcs"
    "-mfloat-abi=hard"
    "-mfpu=fpv4-sp-d16"
    "-ffunction-sections -fdata-sections -fno-strict-aliasing"
    "-fno-builtin -fshort-enums -Os"
)
string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}") # CMake makes list semicolon seperated so these must be replaced with spaces 

set(CMAKE_ASM_FLAGS ${CMAKE_C_FLAGS})

SET(CMAKE_EXE_LINKER_FLAGS
    "-T${CMAKE_SOURCE_DIR}/secure_bootloader_gcc_nrf52.ld"
    "-mthumb -mabi=aapcs -L${CMAKE_SOURCE_DIR}/${SDK_ROOT}/modules/nrfx/mdk"
    "-Wl,--gc-sections"
    "--specs=nano.specs"
    "-Xlinker -Map=${target_name}.map"
)
string(REPLACE ";" " " CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

function(create_dfu executable)
    add_custom_command(
        TARGET ${executable}
        POST_BUILD
        COMMENT "Generating ${executable}_dfu.zip"
        COMMAND nrfutil pkg generate --hw-version 52 --sd-req 0xAE,0xB6 --application-version ${VERSION_NUMBER} --application ${CMAKE_CURRENT_BINARY_DIR}/${executable}.hex --key-file ${PROJECT_SOURCE_DIR}/${PROJ_DIR}/nrfutil_pkg_key.pem ${CMAKE_CURRENT_BINARY_DIR}/${executable}_dfu.zip
    )
endfunction(create_dfu)

add_executable(${target_name} "${SRCS}")
target_link_libraries(${target_name} m)
target_link_libraries(${target_name} c)
target_link_libraries(${target_name} nosys)
target_link_libraries(${target_name} ${CMAKE_SOURCE_DIR}/${OLD_SDK_ROOT}/external/nrf_oberon/lib/nrf52/liboberon_2.0.5.a)
target_link_libraries(${target_name} ${CMAKE_SOURCE_DIR}/${OLD_SDK_ROOT}/external/nrf_cc310_bl/lib/libnrf_cc310_bl_0.9.10.a)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

create_hex(${target_name})

add_custom_target(flash
                  COMMENT "Flashing the bootloader"
                  COMMAND nrfjprog -f nrf52 --eraseall
                  COMMAND nrfjprog -f nrf52 --program ${PROJECT_SOURCE_DIR}/${SDK_ROOT}/components/softdevice/s140/hex/s140_nrf52_6.1.1_softdevice.hex --sectorerase
                  COMMAND nrfjprog -f nrf52 --program ${CMAKE_CURRENT_BINARY_DIR}/${target_name}.hex --sectoranduicrerase
                  COMMAND nrfjprog -f nrf52 --reset
)
add_dependencies(flash ${target_name})