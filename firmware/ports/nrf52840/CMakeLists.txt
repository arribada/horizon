cmake_minimum_required(VERSION 3.0)

project(Horizon
    VERSION 10.0.2.0
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

set(BOARD "horizon_v4.0")

set(SDK_ROOT      "drivers/nRF5_SDK_15.3.0")
set(OLD_SDK_ROOT  "drivers/nRF5_SDK_15.2.0")
set(PROJ_DIR      ".")
set(BOARD_DIR     "${PROJ_DIR}/boards/${BOARD}")
set(CORE_DIR      "../../core")
set(DEVICE_DIR    "../../devices")
set(SYSHAL_DIR    "../../syshal")

# Generate git hash string
execute_process(
    COMMAND git -C ${PROJECT_SOURCE_DIR} describe --always --dirty --abbrev=9 --exclude "*"
    OUTPUT_VARIABLE GIT_VERSION
)
string(REGEX REPLACE "\n$" "" GIT_VERSION "${GIT_VERSION}") # Remove new line from hash

# Are we running this with the clang static analyser?
if(CMAKE_C_COMPILER MATCHES ccc-analyzer)
    set(CLANG_STATIC_ANALYSIS TRUE)
endif()

set(SRCS
    "${BOARD_DIR}/bsp.c"
    "${CORE_DIR}/at/at.c"
    "${CORE_DIR}/aws/aws.c"
    "${CORE_DIR}/buffer/buffer.c"
    "${CORE_DIR}/cexception/cexception.c"
    "${CORE_DIR}/cmd/cmd.c"
    "${CORE_DIR}/config_if/backends/fs_script.c"
    "${CORE_DIR}/config_if/config_if.c"
    "${CORE_DIR}/crc32/crc_32.c"
    "${CORE_DIR}/debug/debug.c"
    "${CORE_DIR}/fs/fs.c"
    "${CORE_DIR}/gps_scheduler/gps_scheduler.c"
    "${CORE_DIR}/iot/iot.c"
    "${CORE_DIR}/json/json.c"
    "${CORE_DIR}/logging/logging.c"
    "${CORE_DIR}/sm/sm.c"
    "${CORE_DIR}/sm_iot/sm_iot.c"
    "${CORE_DIR}/sm_main/sm_main.c"
    "${CORE_DIR}/sys_config/sys_config.c"
    "${DEVICE_DIR}/axl/LSM9DS1/LSM9DS1.c"
    "${DEVICE_DIR}/battery/BQ27621/BQ27621.c"
    "${DEVICE_DIR}/cellular/SARAU270/syshal_cellular.c"
    "${DEVICE_DIR}/flash/IS25LP128F/IS25LP128F_SPI.c"
    "${DEVICE_DIR}/gps/M8N/M8N.c"
    "${DEVICE_DIR}/pressure/MS5837_xBA/MS5837_30BA/MS5837_30BA.c"
    "${DEVICE_DIR}/pressure/MS5837_xBA/MS5837_xBA.c"
    "${DEVICE_DIR}/satellite/artic/prepas.c"
    "${DEVICE_DIR}/satellite/artic/syshal_sat.c"
    "${PROJ_DIR}/drivers/ble/amts.c"
    "${PROJ_DIR}/main.c"
    "${PROJ_DIR}/syshal/syshal_adc.c"
    "${PROJ_DIR}/syshal/syshal_ble.c"
    "${PROJ_DIR}/syshal/syshal_device.c"
    "${PROJ_DIR}/syshal/syshal_firmware.c"
    "${PROJ_DIR}/syshal/syshal_gpio.c"
    "${PROJ_DIR}/syshal/syshal_i2c.c"
    "${PROJ_DIR}/syshal/syshal_led.c"
    "${PROJ_DIR}/syshal/syshal_pmu.c"
    "${PROJ_DIR}/syshal/syshal_qspi.c"
    "${PROJ_DIR}/syshal/syshal_rtc.c"
    "${PROJ_DIR}/syshal/syshal_spi.c"
    "${PROJ_DIR}/syshal/syshal_switch.c"
    "${PROJ_DIR}/syshal/syshal_temp.c"
    "${PROJ_DIR}/syshal/syshal_time.c"
    "${PROJ_DIR}/syshal/syshal_uart.c"
    "${PROJ_DIR}/syshal/syshal_usb.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_flash.c"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu/nrf_dfu_settings.c"
    "${SDK_ROOT}/components/ble/ble_db_discovery/ble_db_discovery.c"
    "${SDK_ROOT}/components/ble/common/ble_advdata.c"
    "${SDK_ROOT}/components/ble/common/ble_conn_state.c"
    "${SDK_ROOT}/components/ble/common/ble_srv_common.c"
    "${SDK_ROOT}/components/ble/nrf_ble_gatt/nrf_ble_gatt.c"
    "${SDK_ROOT}/components/ble/nrf_ble_qwr/nrf_ble_qwr.c"
    "${SDK_ROOT}/components/ble/peer_manager/gatt_cache_manager.c"
    "${SDK_ROOT}/components/ble/peer_manager/gatts_cache_manager.c"
    "${SDK_ROOT}/components/ble/peer_manager/id_manager.c"
    "${SDK_ROOT}/components/ble/peer_manager/peer_data_storage.c"
    "${SDK_ROOT}/components/ble/peer_manager/peer_database.c"
    "${SDK_ROOT}/components/ble/peer_manager/peer_id.c"
    "${SDK_ROOT}/components/ble/peer_manager/peer_manager.c"
    "${SDK_ROOT}/components/ble/peer_manager/pm_buffer.c"
    "${SDK_ROOT}/components/ble/peer_manager/security_dispatcher.c"
    "${SDK_ROOT}/components/ble/peer_manager/security_manager.c"
    "${SDK_ROOT}/components/boards/boards.c"
    "${SDK_ROOT}/components/libraries/atomic/nrf_atomic.c"
    "${SDK_ROOT}/components/libraries/atomic_fifo/nrf_atfifo.c"
    "${SDK_ROOT}/components/libraries/atomic_flags/nrf_atflags.c"
    "${SDK_ROOT}/components/libraries/balloc/nrf_balloc.c"
    "${SDK_ROOT}/components/libraries/button/app_button.c"
    "${SDK_ROOT}/components/libraries/crc32/crc32.c"
    "${SDK_ROOT}/components/libraries/experimental_section_vars/nrf_section_iter.c"
    "${SDK_ROOT}/components/libraries/fds/fds.c"
    "${SDK_ROOT}/components/libraries/fstorage/nrf_fstorage.c"
    "${SDK_ROOT}/components/libraries/fstorage/nrf_fstorage_nvmc.c"
    "${SDK_ROOT}/components/libraries/hardfault/hardfault_implementation.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_backend_rtt.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_backend_serial.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_backend_uart.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_default_backends.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_frontend.c"
    "${SDK_ROOT}/components/libraries/log/src/nrf_log_str_formatter.c"
    "${SDK_ROOT}/components/libraries/memobj/nrf_memobj.c"
    "${SDK_ROOT}/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c"
    "${SDK_ROOT}/components/libraries/queue/nrf_queue.c"
    "${SDK_ROOT}/components/libraries/ringbuf/nrf_ringbuf.c"
    "${SDK_ROOT}/components/libraries/scheduler/app_scheduler.c"
    "${SDK_ROOT}/components/libraries/strerror/nrf_strerror.c"
    "${SDK_ROOT}/components/libraries/timer/app_timer.c"
    "${SDK_ROOT}/components/libraries/util/app_error.c"
    "${SDK_ROOT}/components/libraries/util/app_error_handler_gcc.c"
    "${SDK_ROOT}/components/libraries/util/app_error_weak.c"
    "${SDK_ROOT}/components/libraries/util/app_util_platform.c"
    "${SDK_ROOT}/components/softdevice/common/nrf_sdh.c"
    "${SDK_ROOT}/components/softdevice/common/nrf_sdh_ble.c"
    "${SDK_ROOT}/components/softdevice/common/nrf_sdh_soc.c"
    "${SDK_ROOT}/external/fnmatch/fnmatch.c"
    "${SDK_ROOT}/external/fprintf/nrf_fprintf.c"
    "${SDK_ROOT}/external/fprintf/nrf_fprintf_format.c"
    "${SDK_ROOT}/integration/nrfx/legacy/nrf_drv_clock.c"
    "${SDK_ROOT}/integration/nrfx/legacy/nrf_drv_spis.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_clock.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_gpiote.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_power.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_ppi.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_pwm.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_qspi.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_rtc.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_saadc.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_spim.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_systick.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_timer.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_twim.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_usbd.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/nrfx_wdt.c"
    "${SDK_ROOT}/modules/nrfx/drivers/src/prs/nrfx_prs.c"
    "${SDK_ROOT}/modules/nrfx/hal/nrf_nvmc.c"
    "${SDK_ROOT}/modules/nrfx/mdk/gcc_startup_nrf52840.S"
    "${SDK_ROOT}/modules/nrfx/mdk/system_nrf52840.c"
    "${SDK_ROOT}/modules/nrfx/soc/nrfx_atomic.c"
    "${SYSHAL_DIR}/src/syshal_timer.c"
)

include_directories(
    "${PROJ_DIR}"
    "${SYSHAL_DIR}/inc"
    "${BOARD_DIR}"
    "${CORE_DIR}/at"
    "${CORE_DIR}/at"
    "${CORE_DIR}/aws"
    "${CORE_DIR}/buffer"
    "${CORE_DIR}/cexception"
    "${CORE_DIR}/cmd"
    "${CORE_DIR}/config"
    "${CORE_DIR}/config_if"
    "${CORE_DIR}/config_if/backends"
    "${CORE_DIR}/crc32"
    "${CORE_DIR}/debug"
    "${CORE_DIR}/fs"
    "${CORE_DIR}/gps_scheduler"
    "${CORE_DIR}/iot"
    "${CORE_DIR}/json"
    "${CORE_DIR}/logging"
    "${CORE_DIR}/ring_buffer"
    "${CORE_DIR}/sm"
    "${CORE_DIR}/sm_iot"
    "${CORE_DIR}/sm_main/"
    "${CORE_DIR}/sys_config"
    "${DEVICE_DIR}/axl/LSM9DS1/"
    "${DEVICE_DIR}/battery/BQ27621/"
    "${DEVICE_DIR}/cellular/SARAU270/"
    "${DEVICE_DIR}/flash/IS25LP128F/"
    "${DEVICE_DIR}/gps/M8N/"
    "${DEVICE_DIR}/pressure/MS5837_xBA/"
    "${DEVICE_DIR}/satellite/artic/"
    "${PROJ_DIR}/drivers/ble"
    "${PROJ_DIR}/syshal"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/ble_dfu"
    "${OLD_SDK_ROOT}/components/libraries/bootloader/dfu"
    "${SDK_ROOT}/components"
    "${SDK_ROOT}/components/ble/ble_advertising"
    "${SDK_ROOT}/components/ble/ble_db_discovery"
    "${SDK_ROOT}/components/ble/ble_dtm"
    "${SDK_ROOT}/components/ble/ble_racp"
    "${SDK_ROOT}/components/ble/ble_services/ble_ancs_c"
    "${SDK_ROOT}/components/ble/ble_services/ble_ans_c"
    "${SDK_ROOT}/components/ble/ble_services/ble_bas"
    "${SDK_ROOT}/components/ble/ble_services/ble_bas_c"
    "${SDK_ROOT}/components/ble/ble_services/ble_cscs"
    "${SDK_ROOT}/components/ble/ble_services/ble_cts_c"
    "${SDK_ROOT}/components/ble/ble_services/ble_dfu"
    "${SDK_ROOT}/components/ble/ble_services/ble_dis"
    "${SDK_ROOT}/components/ble/ble_services/ble_gls"
    "${SDK_ROOT}/components/ble/ble_services/ble_hids"
    "${SDK_ROOT}/components/ble/ble_services/ble_hrs"
    "${SDK_ROOT}/components/ble/ble_services/ble_hrs_c"
    "${SDK_ROOT}/components/ble/ble_services/ble_hts"
    "${SDK_ROOT}/components/ble/ble_services/ble_ias"
    "${SDK_ROOT}/components/ble/ble_services/ble_ias_c"
    "${SDK_ROOT}/components/ble/ble_services/ble_lbs"
    "${SDK_ROOT}/components/ble/ble_services/ble_lbs_c"
    "${SDK_ROOT}/components/ble/ble_services/ble_lls"
    "${SDK_ROOT}/components/ble/ble_services/ble_nus"
    "${SDK_ROOT}/components/ble/ble_services/ble_nus_c"
    "${SDK_ROOT}/components/ble/ble_services/ble_rscs"
    "${SDK_ROOT}/components/ble/ble_services/ble_rscs_c"
    "${SDK_ROOT}/components/ble/ble_services/ble_tps"
    "${SDK_ROOT}/components/ble/common"
    "${SDK_ROOT}/components/ble/nrf_ble_gatt"
    "${SDK_ROOT}/components/ble/nrf_ble_qwr"
    "${SDK_ROOT}/components/ble/peer_manager"
    "${SDK_ROOT}/components/boards"
    "${SDK_ROOT}/components/libraries/atomic"
    "${SDK_ROOT}/components/libraries/atomic_fifo"
    "${SDK_ROOT}/components/libraries/atomic_flags"
    "${SDK_ROOT}/components/libraries/balloc"
    "${SDK_ROOT}/components/libraries/bsp"
    "${SDK_ROOT}/components/libraries/button"
    "${SDK_ROOT}/components/libraries/cli"
    "${SDK_ROOT}/components/libraries/cli/rtt"
    "${SDK_ROOT}/components/libraries/cli/uart"
    "${SDK_ROOT}/components/libraries/crc16"
    "${SDK_ROOT}/components/libraries/crc32"
    "${SDK_ROOT}/components/libraries/crypto"
    "${SDK_ROOT}/components/libraries/csense"
    "${SDK_ROOT}/components/libraries/csense_drv"
    "${SDK_ROOT}/components/libraries/delay"
    "${SDK_ROOT}/components/libraries/ecc"
    "${SDK_ROOT}/components/libraries/experimental_section_vars"
    "${SDK_ROOT}/components/libraries/experimental_task_manager"
    "${SDK_ROOT}/components/libraries/fds"
    "${SDK_ROOT}/components/libraries/fstorage"
    "${SDK_ROOT}/components/libraries/gfx"
    "${SDK_ROOT}/components/libraries/gpiote"
    "${SDK_ROOT}/components/libraries/hardfault"
    "${SDK_ROOT}/components/libraries/hci"
    "${SDK_ROOT}/components/libraries/led_softblink"
    "${SDK_ROOT}/components/libraries/log"
    "${SDK_ROOT}/components/libraries/log/src"
    "${SDK_ROOT}/components/libraries/low_power_pwm"
    "${SDK_ROOT}/components/libraries/mem_manager"
    "${SDK_ROOT}/components/libraries/memobj"
    "${SDK_ROOT}/components/libraries/mpu"
    "${SDK_ROOT}/components/libraries/mutex"
    "${SDK_ROOT}/components/libraries/pwm"
    "${SDK_ROOT}/components/libraries/pwr_mgmt"
    "${SDK_ROOT}/components/libraries/queue"
    "${SDK_ROOT}/components/libraries/ringbuf"
    "${SDK_ROOT}/components/libraries/scheduler"
    "${SDK_ROOT}/components/libraries/sdcard"
    "${SDK_ROOT}/components/libraries/slip"
    "${SDK_ROOT}/components/libraries/sortlist"
    "${SDK_ROOT}/components/libraries/spi_mngr"
    "${SDK_ROOT}/components/libraries/stack_guard"
    "${SDK_ROOT}/components/libraries/strerror"
    "${SDK_ROOT}/components/libraries/timer"
    "${SDK_ROOT}/components/libraries/twi_mngr"
    "${SDK_ROOT}/components/libraries/twi_sensor"
    "${SDK_ROOT}/components/libraries/usbd"
    "${SDK_ROOT}/components/libraries/usbd/class/audio"
    "${SDK_ROOT}/components/libraries/usbd/class/cdc"
    "${SDK_ROOT}/components/libraries/usbd/class/cdc/acm"
    "${SDK_ROOT}/components/libraries/usbd/class/hid"
    "${SDK_ROOT}/components/libraries/usbd/class/hid/generic"
    "${SDK_ROOT}/components/libraries/usbd/class/hid/kbd"
    "${SDK_ROOT}/components/libraries/usbd/class/hid/mouse"
    "${SDK_ROOT}/components/libraries/usbd/class/msc"
    "${SDK_ROOT}/components/libraries/util"
    "${SDK_ROOT}/components/nfc/ndef/conn_hand_parser"
    "${SDK_ROOT}/components/nfc/ndef/conn_hand_parser/ac_rec_parser"
    "${SDK_ROOT}/components/nfc/ndef/conn_hand_parser/ble_oob_advdata_parser"
    "${SDK_ROOT}/components/nfc/ndef/conn_hand_parser/le_oob_rec_parser"
    "${SDK_ROOT}/components/nfc/ndef/connection_handover/ac_rec"
    "${SDK_ROOT}/components/nfc/ndef/connection_handover/ble_oob_advdata"
    "${SDK_ROOT}/components/nfc/ndef/connection_handover/ble_pair_lib"
    "${SDK_ROOT}/components/nfc/ndef/connection_handover/ble_pair_msg"
    "${SDK_ROOT}/components/nfc/ndef/connection_handover/common"
    "${SDK_ROOT}/components/nfc/ndef/connection_handover/ep_oob_rec"
    "${SDK_ROOT}/components/nfc/ndef/connection_handover/hs_rec"
    "${SDK_ROOT}/components/nfc/ndef/connection_handover/le_oob_rec"
    "${SDK_ROOT}/components/nfc/ndef/generic/message"
    "${SDK_ROOT}/components/nfc/ndef/generic/record"
    "${SDK_ROOT}/components/nfc/ndef/launchapp"
    "${SDK_ROOT}/components/nfc/ndef/parser/message"
    "${SDK_ROOT}/components/nfc/ndef/parser/record"
    "${SDK_ROOT}/components/nfc/ndef/text"
    "${SDK_ROOT}/components/nfc/ndef/uri"
    "${SDK_ROOT}/components/nfc/t2t_lib"
    "${SDK_ROOT}/components/nfc/t2t_parser"
    "${SDK_ROOT}/components/nfc/t4t_lib"
    "${SDK_ROOT}/components/nfc/t4t_parser/apdu"
    "${SDK_ROOT}/components/nfc/t4t_parser/cc_file"
    "${SDK_ROOT}/components/nfc/t4t_parser/hl_detection_procedure"
    "${SDK_ROOT}/components/nfc/t4t_parser/tlv"
    "${SDK_ROOT}/components/softdevice/common"
    "${SDK_ROOT}/components/softdevice/s140/headers"
    "${SDK_ROOT}/components/softdevice/s140/headers/nrf52"
    "${SDK_ROOT}/components/toolchain/cmsis/include"
    "${SDK_ROOT}/external/fnmatch"
    "${SDK_ROOT}/external/fprintf"
    "${SDK_ROOT}/integration/nrfx"
    "${SDK_ROOT}/integration/nrfx/legacy"
    "${SDK_ROOT}/modules/nrfx"
    "${SDK_ROOT}/modules/nrfx/drivers/include"
    "${SDK_ROOT}/modules/nrfx/hal"
    "${SDK_ROOT}/modules/nrfx/mdk"
)

add_definitions(
    "-DBOARD_TESTER_DEF"
    "-DBOARD_PCA10059"
    "-DCONFIG_GPIO_AS_PINRESET"
    "-DCONFIG_NFCT_PINS_AS_GPIOS"
    "-DFLOAT_ABI_HARD"
    "-DNRF52840_XXAA"
    "-DNRF_SD_BLE_API_VERSION=6"
    "-DS140"
    "-DSOFTDEVICE_PRESENT"
    "-DSWI_DISABLE0"
    "-DGIT_VERSION=\"${GIT_VERSION}\""
    "-DDEBUG_COLOR"
    "-DNRF_DFU_SETTINGS_VERSION=1"
    "-DAPP_FIRMWARE_VERSION=${VERSION_NUMBER}"
    "-BUFFER_MAX_POOL_BUFFERS=128"
    "-BLE_MAX_BUFFERS=128"
    "-D__HEAP_SIZE=4096"
    "-D__STACK_SIZE=65536"
)

if (${CMAKE_BUILD_TYPE} MATCHES "Release")
    set(target_name horizon_app_v${VERSION_STRING})
elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(target_name horizon_app_v${VERSION_STRING}_debug)
else()
    message(FATAL_ERROR "Invalid build type. Use -DCMAKE_BUILD_TYPE=Release or -DCMAKE_BUILD_TYPE=Debug")
endif()

set(CMAKE_C_FLAGS
    # Warnings
    "-Wall"
    "-Wextra"
    "-Werror"
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
    "-ffunction-sections -fdata-sections -fno-strict-aliasing"
    "-fno-builtin -fshort-enums"
    "-mlong-calls" # THIS IS POTENTIALLY SLOW!

    # Always output coloured text from GCC
    "-fdiagnostics-color=always"
)

if(NOT CLANG_STATIC_ANALYSIS)
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-mfloat-abi=hard" "-mfpu=fpv4-sp-d16")
endif()

if(CLANG_STATIC_ANALYSIS)
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-DCLANG_STATIC_ANALYSER -D__SOFTFP__")
    SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS) # Remove -rdynamic from linker options (which CMake adds for some reason)
    
    # Find and get the root directory of arm-none-eabi-gcc. This is needed as we need to explicitly add its include directory
    find_program(ARM_GCC_BIN_PATH NAMES arm-none-eabi-gcc)

    if(NOT ARM_GCC_BIN_PATH)
        message(FATAL_ERROR "Can not find an installation of arm-none-eabi-gcc")
    endif()

    get_filename_component(ARM_GCC_BIN_ROOT ${ARM_GCC_BIN_PATH} DIRECTORY)
    include_directories("${ARM_GCC_BIN_ROOT}/../arm-none-eabi/include")
endif()

string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}") # CMake makes list semicolon seperated so these must be replaced with spaces

set(CMAKE_C_FLAGS_DEBUG   "-Og -g3 -DDEBUG -DDONT_RESTART_ASSERT")
set(CMAKE_C_FLAGS_RELEASE "-O3 -g3 -DDEBUG_DISABLED")

set(CMAKE_ASM_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_ASM_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
set(CMAKE_ASM_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

SET(CMAKE_EXE_LINKER_FLAGS
    "-T${CMAKE_SOURCE_DIR}/gcc_nrf52840.ld"
    "-mthumb -mabi=aapcs -L${CMAKE_SOURCE_DIR}/${SDK_ROOT}/modules/nrfx/mdk"
    "-Wl,--gc-sections"
    "--specs=nano.specs"
    "-u _printf_float"
    "-u _scanf_float"
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

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

if(NOT CLANG_STATIC_ANALYSIS)
    create_bin(${target_name})
    create_hex(${target_name})
    create_dfu(${target_name})

    SET(FLASH_DEVICE CACHE STRING "The target device for flash command. E.g. COM7 or ttyACM0")
    add_custom_target(flash
                      COMMENT "Flashing the firmware to device ${FLASH_DEVICE}"
                      COMMAND nrfutil dfu usb-serial -pkg ${CMAKE_CURRENT_BINARY_DIR}/${target_name}_dfu.zip -p ${FLASH_DEVICE}
    )
    add_dependencies(flash ${target_name})

    # Setup a make cppcheck option
    get_property(ALL_INCLUDES DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    foreach(INCLUDE_EACH ${ALL_INCLUDES}) # Create a list of all the include directories appended with -I for use with CppCheck
        set(CPPCHECK_INCLUDES ${CPPCHECK_INCLUDES}\ -I${PROJECT_SOURCE_DIR}/${INCLUDE_EACH})
    endforeach()
    add_custom_target(cppcheck
                      COMMENT "Running CppCheck over the project files"
                      COMMAND cppcheck -j 4 --enable=all --inconclusive --force --suppress=invalidPrintfArgType_uint --suppress=invalidPrintfArgType_sint --suppress=invalidPrintfArgType_float --xml --xml-version=2 ${PROJECT_SOURCE_DIR}/${CORE_DIR} ${PROJECT_SOURCE_DIR}/${DEVICE_DIR} ${PROJECT_SOURCE_DIR}/${SYSHAL_DIR} ${CPPCHECK_INCLUDES} 2> cppcheck.xml
    )

    # Add a make cpd option for code duplication analysis
    add_custom_target(cpd
                      COMMENT "Running Code Duplication Analysier (CPD) over the project files"
                      COMMAND pmd cpd --minimum-tokens 100 --language c --format xml --failOnViolation false --files ${PROJECT_SOURCE_DIR}/${CORE_DIR} --files ${PROJECT_SOURCE_DIR}/${DEVICE_DIR} --files ${PROJECT_SOURCE_DIR}/${SYSHAL_DIR} > cpd.xml
    )
endif()
