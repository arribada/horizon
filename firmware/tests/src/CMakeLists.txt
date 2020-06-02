cmake_minimum_required(VERSION 3.10)
include(GoogleTest)

# Set up path varaibles
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(CORE_DIR ${ROOT_DIR}/core)
set(SYSHAL_DIR ${ROOT_DIR}/syshal)
set(DEVICES_DIR ${ROOT_DIR}/devices)
set(MOCKS_DIR ${ROOT_DIR}/tests/src/mocks)

# Setup CMOCK includes and source files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mocks)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cmock/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cmock/vendor/unity/src)
set(CMOCK_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmock_stubs.c ${CMAKE_CURRENT_SOURCE_DIR}/cmock/src/cmock.c ${CMAKE_CURRENT_SOURCE_DIR}/cmock/vendor/unity/src/unity.c) # Add cmock source files to CMOCK_SOURCES
add_definitions(-DUNITY_USE_FLUSH_STDOUT) # Use stdout flush for Unity
add_definitions(-DCMOCK_MEM_DYNAMIC) # Use dynamic memory allocation

# Setup GTEST includes and library linkage
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/googletest-release-1.8.1/googletest/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/googletest-release-1.8.1/build/googlemock/gtest)
link_libraries(libgtest.a libgtest_main.a pthread gcov)

# Build flags for allowing gcov to collect test coverage statistics
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1) # Generate *.gcno not *.cpp.gcno extensions
set(CMAKE_CXX_FLAGS "-g -O0 --coverage")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "-g -O0 --coverage")
set(CMAKE_EXE_LINKER_FLAGS "--coverage")

# Application specific flags
add_definitions(-DBOARD_TESTER_DEF -DBOARD_PCA10059 -DCONFIG_GPIO_AS_PINRESET -DCONFIG_NFCT_PINS_AS_GPIOS -DFLOAT_ABI_HARD -DNRF52840_XXAA -DNRF_SD_BLE_API_VERSION=6 -DS140 -DSOFTDEVICE_PRESENT -DSWI_DISABLE0 -DDEBUG_DISABLED -DGTEST)

# Common includes between all tests
include_directories(${SYSHAL_DIR}/inc)
include_directories(${CORE_DIR}/aws)
include_directories(${CORE_DIR}/buffer)
include_directories(${CORE_DIR}/cexception)
include_directories(${CORE_DIR}/cmd)
include_directories(${CORE_DIR}/config)
include_directories(${CORE_DIR}/config_if)
include_directories(${CORE_DIR}/config_if/backends)
include_directories(${CORE_DIR}/crc32)
include_directories(${CORE_DIR}/fs)
include_directories(${CORE_DIR}/gps_scheduler)
include_directories(${CORE_DIR}/iot)
include_directories(${CORE_DIR}/json)
include_directories(${CORE_DIR}/logging)
include_directories(${CORE_DIR}/sm)
include_directories(${CORE_DIR}/sm_iot)
include_directories(${CORE_DIR}/sm_main)
include_directories(${CORE_DIR}/sys_config)
include_directories(${CORE_DIR}/at)
include_directories(${DEVICES_DIR}/satellite/artic)

# Enable CTest
enable_testing()

# test_aws
add_executable(test_aws test_aws.cpp)
target_sources(test_aws PRIVATE ${CMOCK_SOURCES})
target_sources(test_aws PRIVATE ${CORE_DIR}/aws/aws.c)
target_sources(test_aws PRIVATE ${CORE_DIR}/json/json.c)
gtest_discover_tests(test_aws)

# test_buffer
add_executable(test_buffer test_buffer.cpp)
target_sources(test_buffer PRIVATE ${CMOCK_SOURCES})
target_sources(test_buffer PRIVATE ${CORE_DIR}/buffer/buffer.c)
gtest_discover_tests(test_buffer)

# test_flash
add_executable(test_flash test_flash.cpp)
target_sources(test_flash PRIVATE ${CMOCK_SOURCES})
target_sources(test_flash PRIVATE ${MOCKS_DIR}/Mocksyshal_gpio.c)
target_sources(test_flash PRIVATE ${MOCKS_DIR}/Mocksyshal_spi.c)
target_sources(test_flash PRIVATE ${DEVICES_DIR}/flash/S25FL128/S25FL128.c)
target_include_directories(test_flash PRIVATE ${DEVICES_DIR}/flash/S25FL128)
gtest_discover_tests(test_flash)

# test_fs
add_executable(test_fs test_fs.cpp)
target_sources(test_fs PRIVATE ${CMOCK_SOURCES})
target_sources(test_fs PRIVATE ${MOCKS_DIR}/Mocksyshal_flash.c)
target_sources(test_fs PRIVATE ${CORE_DIR}/fs/fs.c)
gtest_discover_tests(test_fs)

# test_fs_script
add_executable(test_fs_script test_fs_script.cpp)
target_sources(test_fs_script PRIVATE ${CMOCK_SOURCES})
target_sources(test_fs_script PRIVATE ${MOCKS_DIR}/Mocksyshal_flash.c)
target_sources(test_fs_script PRIVATE ${CORE_DIR}/cmd/cmd.c)
target_sources(test_fs_script PRIVATE ${CORE_DIR}/fs/fs.c)
target_sources(test_fs_script PRIVATE ${CORE_DIR}/config_if/backends/fs_script.c)
gtest_discover_tests(test_fs_script)

# test_iot
add_executable(test_iot test_iot.cpp)
target_sources(test_iot PRIVATE ${CMOCK_SOURCES})
target_sources(test_iot PRIVATE ${MOCKS_DIR}/Mockaws.c)
target_sources(test_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_cellular.c)
target_sources(test_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_sat.c)
target_sources(test_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_flash.c)
target_sources(test_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_time.c)
target_sources(test_iot PRIVATE ${CORE_DIR}/logging/logging.c)
target_sources(test_iot PRIVATE ${CORE_DIR}/fs/fs.c)
target_sources(test_iot PRIVATE ${CORE_DIR}/iot/iot.c)
gtest_discover_tests(test_iot)

# test_json
add_executable(test_json test_json.cpp)
target_sources(test_json PRIVATE ${CMOCK_SOURCES})
target_sources(test_json PRIVATE ${CORE_DIR}/json/json.c)
gtest_discover_tests(test_json)

# test_prepas
add_executable(test_prepas test_prepas.cpp)
target_sources(test_prepas PRIVATE ${DEVICES_DIR}/satellite/artic/prepas.c)
target_include_directories(test_prepas PRIVATE ${DEVICES_DIR}/satellite/artic)
gtest_discover_tests(test_prepas)

# test_ring_buffer
add_executable(test_ring_buffer test_ring_buffer.cpp)
target_sources(test_ring_buffer PRIVATE ${CMOCK_SOURCES})
target_include_directories(test_ring_buffer PRIVATE ${CORE_DIR}/ring_buffer)
gtest_discover_tests(test_ring_buffer)

# test_rtc
add_executable(test_rtc test_rtc.cpp)
target_sources(test_rtc PRIVATE ${CMOCK_SOURCES})
target_sources(test_rtc PRIVATE ${MOCKS_DIR}/nrfx_rtc.c)
target_sources(test_rtc PRIVATE ${ROOT_DIR}/ports/nrf52840/syshal/syshal_rtc.c)
target_sources(test_rtc PRIVATE ${SYSHAL_DIR}/src/syshal_timer.c)
gtest_discover_tests(test_rtc)

# test_syshal_timer
add_executable(test_syshal_timer test_syshal_timer.cpp)
target_sources(test_syshal_timer PRIVATE ${CMOCK_SOURCES})
target_sources(test_syshal_timer PRIVATE ${MOCKS_DIR}/Mocksyshal_rtc.c)
target_sources(test_syshal_timer PRIVATE ${SYSHAL_DIR}/src/syshal_timer.c)
gtest_discover_tests(test_syshal_timer)

# test_syshal_timer_nrf52840
add_executable(test_syshal_timer_nrf52840 test_syshal_timer_nrf52840.cpp)
target_sources(test_syshal_timer_nrf52840 PRIVATE ${CMOCK_SOURCES})
target_sources(test_syshal_timer_nrf52840 PRIVATE ${MOCKS_DIR}/nrfx_rtc.c)
target_sources(test_syshal_timer_nrf52840 PRIVATE ${ROOT_DIR}/ports/nrf52840/syshal/syshal_rtc.c)
target_sources(test_syshal_timer_nrf52840 PRIVATE ${SYSHAL_DIR}/src/syshal_timer.c)
gtest_discover_tests(test_syshal_timer_nrf52840)

# test_sm_iot_linux
add_executable(test_sm_iot_linux test_sm_iot_linux.cpp)
target_sources(test_sm_iot_linux PRIVATE ${CMOCK_SOURCES})
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/at/at.c)
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/aws/aws.c)
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/iot/iot.c)
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/json/json.c)
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/sm_iot/sm_iot.c)
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/cexception/cexception.c)
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/sys_config/sys_config.c)
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/fs/fs.c)
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/logging/logging.c)
target_sources(test_sm_iot_linux PRIVATE ${SYSHAL_DIR}/src/syshal_timer.c)
target_sources(test_sm_iot_linux PRIVATE ${DEVICES_DIR}/cellular/SARAU270/syshal_cellular.c)
target_sources(test_sm_iot_linux PRIVATE ${MOCKS_DIR}/Mocksyshal_batt.c)
target_sources(test_sm_iot_linux PRIVATE ${MOCKS_DIR}/Mocksyshal_gpio.c)
target_sources(test_sm_iot_linux PRIVATE ${MOCKS_DIR}/Mocksyshal_flash.c)
target_sources(test_sm_iot_linux PRIVATE ${MOCKS_DIR}/Mocksyshal_firmware.c)
target_sources(test_sm_iot_linux PRIVATE ${MOCKS_DIR}/Mocksyshal_pmu.c)
target_sources(test_sm_iot_linux PRIVATE ${MOCKS_DIR}/Mocksyshal_time.c)
target_sources(test_sm_iot_linux PRIVATE ${MOCKS_DIR}/Mocksyshal_device.c)
target_sources(test_sm_iot_linux PRIVATE ${MOCKS_DIR}/Mocksyshal_sat.c)
target_sources(test_sm_iot_linux PRIVATE ${MOCKS_DIR}/Mockprepas.c)
target_sources(test_sm_iot_linux PRIVATE ${ROOT_DIR}/ports/linux/syshal/syshal_uart.c)
target_sources(test_sm_iot_linux PRIVATE ${ROOT_DIR}/ports/linux/syshal/syshal_rtc.c)
target_sources(test_sm_iot_linux PRIVATE ${CORE_DIR}/crc32/crc_32.c)
#gtest_discover_tests(test_sm_iot_linux) # CTest execution disabled

# test_sm_iot
add_executable(test_sm_iot test_sm_iot.cpp)
target_sources(test_sm_iot PRIVATE ${CMOCK_SOURCES})
target_sources(test_sm_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_batt.c)
target_sources(test_sm_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_pmu.c)
target_sources(test_sm_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_rtc.c)
target_sources(test_sm_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_firmware.c)
target_sources(test_sm_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_device.c)
target_sources(test_sm_iot PRIVATE ${MOCKS_DIR}/Mocksyshal_sat.c)
target_sources(test_sm_iot PRIVATE ${MOCKS_DIR}/Mockiot.c)
target_sources(test_sm_iot PRIVATE ${SYSHAL_DIR}/src/syshal_timer.c)
target_sources(test_sm_iot PRIVATE ${CORE_DIR}/cexception/cexception.c)
target_sources(test_sm_iot PRIVATE ${CORE_DIR}/crc32/crc_32.c)
target_sources(test_sm_iot PRIVATE ${CORE_DIR}/sm_iot/sm_iot.c)
gtest_discover_tests(test_sm_iot)

# test_gps_scheduler
add_executable(test_gps_scheduler test_gps_scheduler.cpp)
target_sources(test_gps_scheduler PRIVATE ${CMOCK_SOURCES})
target_sources(test_gps_scheduler PRIVATE ${CORE_DIR}/gps_scheduler/gps_scheduler.c)
target_sources(test_gps_scheduler PRIVATE ${SYSHAL_DIR}/src/syshal_timer.c)
target_sources(test_gps_scheduler PRIVATE ${MOCKS_DIR}/Mocksyshal_gps.c)
target_sources(test_gps_scheduler PRIVATE ${MOCKS_DIR}/Mocksyshal_rtc.c)
target_sources(test_gps_scheduler PRIVATE ${MOCKS_DIR}/Mocksyshal_time.c)
gtest_discover_tests(test_gps_scheduler)

# test_sm_main
add_executable(test_sm_main test_sm_main.cpp)
target_sources(test_sm_main PRIVATE ${CMOCK_SOURCES})
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_axl.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_batt.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_ble.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_firmware.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_gpio.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_gps.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_flash.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_uart.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_sat.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_spi.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_switch.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_i2c.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_rtc.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_time.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_pressure.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_pmu.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_cellular.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_usb.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_device.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksyshal_led.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mocksm_iot.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mockiot.c)
target_sources(test_sm_main PRIVATE ${MOCKS_DIR}/Mockgps_scheduler.c)
target_sources(test_sm_main PRIVATE ${SYSHAL_DIR}/src/syshal_timer.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/buffer/buffer.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/cexception/cexception.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/cmd/cmd.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/config_if/config_if.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/config_if/backends/fs_script.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/crc32/crc_32.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/fs/fs.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/sm/sm.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/sm_main/sm_main.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/sys_config/sys_config.c)
target_sources(test_sm_main PRIVATE ${CORE_DIR}/at/at.c)
gtest_discover_tests(test_sm_main)

# test_sys_config
add_executable(test_sys_config test_sys_config.cpp)
target_sources(test_sys_config PRIVATE ${CMOCK_SOURCES})
target_sources(test_sys_config PRIVATE ${MOCKS_DIR}/Mocksyshal_rtc.c)
target_sources(test_sys_config PRIVATE ${MOCKS_DIR}/Mocksyshal_flash.c)
target_sources(test_sys_config PRIVATE ${CORE_DIR}/sys_config/sys_config.c)
target_sources(test_sys_config PRIVATE ${CORE_DIR}/fs/fs.c)
target_sources(test_sys_config PRIVATE ${CORE_DIR}/crc32/crc_32.c)
gtest_discover_tests(test_sys_config)

# test_at
add_executable(test_at test_at.cpp)
target_sources(test_at PRIVATE ${CMOCK_SOURCES})
target_sources(test_at PRIVATE ${CORE_DIR}/at/at.c)
target_sources(test_at PRIVATE ${MOCKS_DIR}/Mockfs.c)
target_sources(test_at PRIVATE ${MOCKS_DIR}/Mocksyshal_uart.c)
gtest_discover_tests(test_at)

# test_syshal_cellular
add_executable(test_syshal_cellular test_syshal_cellular.cpp)
target_sources(test_syshal_cellular PRIVATE ${CMOCK_SOURCES})
target_sources(test_syshal_cellular PRIVATE ${CORE_DIR}/at/at.c)
target_sources(test_syshal_cellular PRIVATE ${DEVICES_DIR}/cellular/SARAU270/syshal_cellular.c)
target_sources(test_syshal_cellular PRIVATE ${MOCKS_DIR}/Mockfs.c)
target_sources(test_syshal_cellular PRIVATE ${MOCKS_DIR}/Mocksyshal_uart.c)
target_sources(test_syshal_cellular PRIVATE ${MOCKS_DIR}/Mocksyshal_gpio.c)
target_sources(test_syshal_cellular PRIVATE ${MOCKS_DIR}/Mocksyshal_time.c)
target_sources(test_syshal_cellular PRIVATE ${MOCKS_DIR}/Mocksyshal_rtc.c)
gtest_discover_tests(test_syshal_cellular)

# test_syshal_sat
add_executable(test_syshal_sat test_syshal_sat.cpp)
target_sources(test_syshal_sat PRIVATE ${CMOCK_SOURCES})
target_sources(test_syshal_sat PRIVATE ${CORE_DIR}/at/at.c)
target_sources(test_syshal_sat PRIVATE ${CORE_DIR}/fs/fs.c)
target_sources(test_syshal_sat PRIVATE ${DEVICES_DIR}/satellite/artic/syshal_sat.c)
target_sources(test_syshal_sat PRIVATE ${DEVICES_DIR}/satellite/artic/prepas.c)
target_include_directories(test_syshal_sat PRIVATE ${DEVICES_DIR}/satellite/artic)
target_sources(test_syshal_sat PRIVATE ${MOCKS_DIR}/Mocksyshal_uart.c)
target_sources(test_syshal_sat PRIVATE ${MOCKS_DIR}/Mocksyshal_gpio.c)
target_sources(test_syshal_sat PRIVATE ${MOCKS_DIR}/Mocksyshal_time.c)
target_sources(test_syshal_sat PRIVATE ${MOCKS_DIR}/Mocksyshal_flash.c)
target_sources(test_syshal_sat PRIVATE ${MOCKS_DIR}/Mocksyshal_rtc.c)
target_sources(test_syshal_sat PRIVATE ${MOCKS_DIR}/Mocksyshal_spi.c)
gtest_discover_tests(test_syshal_sat)