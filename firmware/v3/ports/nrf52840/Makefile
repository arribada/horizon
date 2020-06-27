PROJECT_NAME     := plastic_tracker
APP_VERSION      := 9
BOARD            := plastic_tracker_v1
TARGETS          := horizon_app_v$(APP_VERSION)
OUTPUT_DIRECTORY := build

SDK_ROOT     := drivers/nRF5_SDK_15.3.0
OLD_SDK_ROOT := drivers/nRF5_SDK_15.2.0
PROJ_DIR   := .
BOARD_DIR  := $(PROJ_DIR)/boards/$(BOARD)
CORE_DIR = ../../core
DEVICE_DIR = ../../devices
SYSHAL_DIR = ../../syshal

G_VERSION := $(shell git describe --dirty --always --tags)

ifndef RELEASE
  TARGETS := $(TARGETS)_debug
  G_VERSION := $(G_VERSION)_debug
endif

$(OUTPUT_DIRECTORY)/$(TARGETS).out: \
  LINKER_SCRIPT  := gcc_nrf52840.ld

# Source files common to all targets
SRC_FILES += \
  $(PROJ_DIR)/main.c \
  $(PROJ_DIR)/drivers/ble/amts.c \
  $(PROJ_DIR)/syshal/syshal_adc.c \
  $(PROJ_DIR)/syshal/syshal_ble.c \
  $(PROJ_DIR)/syshal/syshal_button.c \
  $(PROJ_DIR)/syshal/syshal_firmware.c \
  $(PROJ_DIR)/syshal/syshal_gpio.c \
  $(PROJ_DIR)/syshal/syshal_i2c.c \
  $(PROJ_DIR)/syshal/syshal_pmu.c \
  $(PROJ_DIR)/syshal/syshal_qspi.c \
  $(PROJ_DIR)/syshal/syshal_rtc.c \
  $(PROJ_DIR)/syshal/syshal_spi.c \
  $(PROJ_DIR)/syshal/syshal_temp.c \
  $(PROJ_DIR)/syshal/syshal_time.c \
  $(PROJ_DIR)/syshal/syshal_uart.c \
  $(PROJ_DIR)/syshal/syshal_usb.c \
  $(PROJ_DIR)/syshal/syshal_device.c \
  $(PROJ_DIR)/syshal/syshal_led.c \
  $(SYSHAL_DIR)/src/syshal_switch.c \
  $(SYSHAL_DIR)/src/syshal_timer.c \
  $(BOARD_DIR)/bsp.c \
  $(CORE_DIR)/aws/aws.c \
  $(CORE_DIR)/buffer/buffer.c \
  $(CORE_DIR)/cexception/cexception.c \
  $(CORE_DIR)/cmd/cmd.c \
  $(CORE_DIR)/config_if/backends/fs_script.c \
  $(CORE_DIR)/config_if/config_if.c \
  $(CORE_DIR)/crc32/crc_32.c \
  $(CORE_DIR)/debug/debug.c \
  $(CORE_DIR)/fs/fs.c \
  $(CORE_DIR)/logging/logging.c \
  $(CORE_DIR)/json/json.c \
  $(CORE_DIR)/sm/sm.c \
  $(CORE_DIR)/sm_iot/sm_iot.c \
  $(CORE_DIR)/sm_main/sm_main.c \
  $(CORE_DIR)/sys_config/sys_config.c \
  $(CORE_DIR)/at/at.c \
  $(CORE_DIR)/iot/iot.c \
  $(DEVICE_DIR)/cellular/SARAU270/syshal_cellular.c \
  $(DEVICE_DIR)/axl/LSM9DS1/LSM9DS1.c \
  $(DEVICE_DIR)/battery/BQ27621/BQ27621.c \
  $(DEVICE_DIR)/gps/M8N/M8N.c \
  $(DEVICE_DIR)/satellite/artic/syshal_sat.c \
  $(DEVICE_DIR)/satellite/artic/prepas.c \
  $(DEVICE_DIR)/flash/IS25LP128F/IS25LP128F_SPI.c \
  $(DEVICE_DIR)/pressure/MS5837_xBA/MS5837_xBA.c \
  $(DEVICE_DIR)/pressure/MS5837_xBA/MS5837_30BA/MS5837_30BA.c \
  $(SDK_ROOT)/components/ble/ble_db_discovery/ble_db_discovery.c \
  $(SDK_ROOT)/components/ble/common/ble_advdata.c \
  $(SDK_ROOT)/components/ble/common/ble_conn_state.c \
  $(SDK_ROOT)/components/ble/common/ble_srv_common.c \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt/nrf_ble_gatt.c \
  $(SDK_ROOT)/components/ble/nrf_ble_qwr/nrf_ble_qwr.c \
  $(SDK_ROOT)/components/ble/peer_manager/gatt_cache_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/gatts_cache_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/id_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_data_storage.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_database.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_id.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/pm_buffer.c \
  $(SDK_ROOT)/components/ble/peer_manager/security_dispatcher.c \
  $(SDK_ROOT)/components/ble/peer_manager/security_manager.c \
  $(SDK_ROOT)/components/boards/boards.c \
  $(SDK_ROOT)/components/libraries/atomic/nrf_atomic.c \
  $(SDK_ROOT)/components/libraries/atomic_fifo/nrf_atfifo.c \
  $(SDK_ROOT)/components/libraries/atomic_flags/nrf_atflags.c \
  $(SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
  $(OLD_SDK_ROOT)/components/libraries/bootloader/dfu/nrf_dfu_flash.c \
  $(OLD_SDK_ROOT)/components/libraries/bootloader/dfu/nrf_dfu_settings.c \
  $(SDK_ROOT)/components/libraries/button/app_button.c \
  $(SDK_ROOT)/components/libraries/crc32/crc32.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_rtt.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_serial.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_uart.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_default_backends.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_frontend.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_str_formatter.c \
  $(SDK_ROOT)/components/libraries/memobj/nrf_memobj.c \
  $(SDK_ROOT)/components/libraries/ringbuf/nrf_ringbuf.c \
  $(SDK_ROOT)/components/libraries/experimental_section_vars/nrf_section_iter.c \
  $(SDK_ROOT)/components/libraries/fds/fds.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage_nvmc.c \
  $(SDK_ROOT)/components/libraries/hardfault/hardfault_implementation.c \
  $(SDK_ROOT)/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c \
  $(SDK_ROOT)/components/libraries/queue/nrf_queue.c \
  $(SDK_ROOT)/components/libraries/scheduler/app_scheduler.c \
  $(SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
  $(SDK_ROOT)/components/libraries/timer/app_timer.c \
  $(SDK_ROOT)/components/libraries/util/app_error.c \
  $(SDK_ROOT)/components/libraries/util/app_error_handler_gcc.c \
  $(SDK_ROOT)/components/libraries/util/app_error_weak.c \
  $(SDK_ROOT)/components/libraries/util/app_util_platform.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_ble.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_soc.c \
  $(SDK_ROOT)/external/fnmatch/fnmatch.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf_format.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_clock.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_spis.c \
  $(SDK_ROOT)/modules/nrfx/soc/nrfx_atomic.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_clock.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_power.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_qspi.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_rtc.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_ppi.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_pwm.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_saadc.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_spim.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_systick.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_twim.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_timer.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_usbd.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_wdt.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/prs/nrfx_prs.c \
  $(SDK_ROOT)/modules/nrfx/hal/nrf_nvmc.c \
  $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52840.S \
  $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52840.c \
#  $(SDK_ROOT)/components/libraries/bsp/bsp.c \

# Include folders common to all targets
INC_FOLDERS += \
  . \
  $(PROJ_DIR)/syshal \
  $(PROJ_DIR)/drivers/ble \
  $(BOARD_DIR) \
  $(CORE_DIR)/at \
  $(CORE_DIR)/aws \
  $(CORE_DIR)/buffer \
  $(CORE_DIR)/cexception \
  $(CORE_DIR)/cmd \
  $(CORE_DIR)/config \
  $(CORE_DIR)/config_if/backends \
  $(CORE_DIR)/config_if \
  $(CORE_DIR)/crc32 \
  $(CORE_DIR)/debug \
  $(CORE_DIR)/fs \
  $(CORE_DIR)/iot \
  $(CORE_DIR)/json \
  $(CORE_DIR)/logging \
  $(CORE_DIR)/ring_buffer \
  $(CORE_DIR)/sm \
  $(CORE_DIR)/sm_iot \
  $(CORE_DIR)/sm_main/ \
  $(CORE_DIR)/sys_config \
  $(CORE_DIR)/at \
  $(DEVICE_DIR)/axl/LSM9DS1/ \
  $(DEVICE_DIR)/battery/BQ27621/ \
  $(DEVICE_DIR)/gps/M8N/ \
  $(DEVICE_DIR)/flash/IS25LP128F/ \
  $(DEVICE_DIR)/cellular/SARAU270/ \
  $(DEVICE_DIR)/satellite/artic/ \
  $(DEVICE_DIR)/pressure/MS5837_xBA/ \
  $(SYSHAL_DIR)/inc \
  $(SDK_ROOT)/components \
  $(SDK_ROOT)/components/ble/ble_advertising \
  $(SDK_ROOT)/components/ble/ble_db_discovery \
  $(SDK_ROOT)/components/ble/ble_dtm \
  $(SDK_ROOT)/components/ble/ble_racp \
  $(SDK_ROOT)/components/ble/ble_services/ble_ancs_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_ans_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_bas \
  $(SDK_ROOT)/components/ble/ble_services/ble_bas_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_cscs \
  $(SDK_ROOT)/components/ble/ble_services/ble_cts_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_dfu \
  $(SDK_ROOT)/components/ble/ble_services/ble_dis \
  $(SDK_ROOT)/components/ble/ble_services/ble_gls \
  $(SDK_ROOT)/components/ble/ble_services/ble_hids \
  $(SDK_ROOT)/components/ble/ble_services/ble_hrs \
  $(SDK_ROOT)/components/ble/ble_services/ble_hrs_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_hts \
  $(SDK_ROOT)/components/ble/ble_services/ble_ias \
  $(SDK_ROOT)/components/ble/ble_services/ble_ias_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_lbs \
  $(SDK_ROOT)/components/ble/ble_services/ble_lbs_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_lls \
  $(SDK_ROOT)/components/ble/ble_services/ble_nus \
  $(SDK_ROOT)/components/ble/ble_services/ble_nus_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_rscs \
  $(SDK_ROOT)/components/ble/ble_services/ble_rscs_c \
  $(SDK_ROOT)/components/ble/ble_services/ble_tps \
  $(SDK_ROOT)/components/ble/common \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt \
  $(SDK_ROOT)/components/ble/nrf_ble_qwr \
  $(SDK_ROOT)/components/ble/peer_manager \
  $(SDK_ROOT)/components/boards \
  $(SDK_ROOT)/components/libraries/atomic \
  $(SDK_ROOT)/components/libraries/atomic_fifo \
  $(SDK_ROOT)/components/libraries/atomic_flags \
  $(SDK_ROOT)/components/libraries/balloc \
  $(OLD_SDK_ROOT)/components/libraries/bootloader/ble_dfu \
  $(OLD_SDK_ROOT)/components/libraries/bootloader/dfu \
  $(SDK_ROOT)/components/libraries/bsp \
  $(SDK_ROOT)/components/libraries/button \
  $(SDK_ROOT)/components/libraries/cli \
  $(SDK_ROOT)/components/libraries/cli/rtt \
  $(SDK_ROOT)/components/libraries/cli/uart \
  $(SDK_ROOT)/components/libraries/crc16 \
  $(SDK_ROOT)/components/libraries/crc32 \
  $(SDK_ROOT)/components/libraries/crypto \
  $(SDK_ROOT)/components/libraries/csense \
  $(SDK_ROOT)/components/libraries/csense_drv \
  $(SDK_ROOT)/components/libraries/delay \
  $(SDK_ROOT)/components/libraries/ecc \
  $(SDK_ROOT)/components/libraries/log \
  $(SDK_ROOT)/components/libraries/log/src \
  $(SDK_ROOT)/components/libraries/memobj \
  $(SDK_ROOT)/components/libraries/mpu \
  $(SDK_ROOT)/components/libraries/ringbuf \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(SDK_ROOT)/components/libraries/stack_guard \
  $(SDK_ROOT)/components/libraries/experimental_task_manager \
  $(SDK_ROOT)/components/libraries/fds \
  $(SDK_ROOT)/components/libraries/fstorage \
  $(SDK_ROOT)/components/libraries/gfx \
  $(SDK_ROOT)/components/libraries/gpiote \
  $(SDK_ROOT)/components/libraries/hardfault \
  $(SDK_ROOT)/components/libraries/hci \
  $(SDK_ROOT)/components/libraries/led_softblink \
  $(SDK_ROOT)/components/libraries/low_power_pwm \
  $(SDK_ROOT)/components/libraries/mem_manager \
  $(SDK_ROOT)/components/libraries/mutex \
  $(SDK_ROOT)/components/libraries/pwm \
  $(SDK_ROOT)/components/libraries/pwr_mgmt \
  $(SDK_ROOT)/components/libraries/queue \
  $(SDK_ROOT)/components/libraries/scheduler \
  $(SDK_ROOT)/components/libraries/sdcard \
  $(SDK_ROOT)/components/libraries/slip \
  $(SDK_ROOT)/components/libraries/sortlist \
  $(SDK_ROOT)/components/libraries/spi_mngr \
  $(SDK_ROOT)/components/libraries/strerror \
  $(SDK_ROOT)/components/libraries/timer \
  $(SDK_ROOT)/components/libraries/twi_mngr \
  $(SDK_ROOT)/components/libraries/twi_sensor \
  $(SDK_ROOT)/components/libraries/usbd \
  $(SDK_ROOT)/components/libraries/usbd/class/audio \
  $(SDK_ROOT)/components/libraries/usbd/class/cdc \
  $(SDK_ROOT)/components/libraries/usbd/class/cdc/acm \
  $(SDK_ROOT)/components/libraries/usbd/class/hid \
  $(SDK_ROOT)/components/libraries/usbd/class/hid/generic \
  $(SDK_ROOT)/components/libraries/usbd/class/hid/kbd \
  $(SDK_ROOT)/components/libraries/usbd/class/hid/mouse \
  $(SDK_ROOT)/components/libraries/usbd/class/msc \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/components/nfc/ndef/conn_hand_parser \
  $(SDK_ROOT)/components/nfc/ndef/conn_hand_parser/ac_rec_parser \
  $(SDK_ROOT)/components/nfc/ndef/conn_hand_parser/ble_oob_advdata_parser \
  $(SDK_ROOT)/components/nfc/ndef/conn_hand_parser/le_oob_rec_parser \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ac_rec \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ble_oob_advdata \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ble_pair_lib \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ble_pair_msg \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/common \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ep_oob_rec \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/hs_rec \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/le_oob_rec \
  $(SDK_ROOT)/components/nfc/ndef/generic/message \
  $(SDK_ROOT)/components/nfc/ndef/generic/record \
  $(SDK_ROOT)/components/nfc/ndef/launchapp \
  $(SDK_ROOT)/components/nfc/ndef/parser/message \
  $(SDK_ROOT)/components/nfc/ndef/parser/record \
  $(SDK_ROOT)/components/nfc/ndef/text \
  $(SDK_ROOT)/components/nfc/ndef/uri \
  $(SDK_ROOT)/components/nfc/t2t_lib \
  $(SDK_ROOT)/components/nfc/t2t_parser \
  $(SDK_ROOT)/components/nfc/t4t_lib \
  $(SDK_ROOT)/components/nfc/t4t_parser/apdu \
  $(SDK_ROOT)/components/nfc/t4t_parser/cc_file \
  $(SDK_ROOT)/components/nfc/t4t_parser/hl_detection_procedure \
  $(SDK_ROOT)/components/nfc/t4t_parser/tlv \
  $(SDK_ROOT)/components/softdevice/common \
  $(SDK_ROOT)/components/softdevice/s140/headers \
  $(SDK_ROOT)/components/softdevice/s140/headers/nrf52 \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/external/fnmatch \
  $(SDK_ROOT)/external/fprintf \
  $(SDK_ROOT)/integration/nrfx \
  $(SDK_ROOT)/integration/nrfx/legacy \
  $(SDK_ROOT)/modules/nrfx \
  $(SDK_ROOT)/modules/nrfx/drivers/include \
  $(SDK_ROOT)/modules/nrfx/hal \
  $(SDK_ROOT)/modules/nrfx/mdk \

# adds a -I prefix to every include. This is used for CPPCHECK
INCLUDE_WITH_I = $(addprefix -I ,$(INC_FOLDERS))

# Libraries common to all targets
LIB_FILES += \

# Optimization flags
ifdef RELEASE
OPT = -O3 -g3
else
OPT = -Og -g3
endif
# Uncomment the line below to enable link time optimization
#OPT += -flto

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -DBOARD_TESTER_DEF
CFLAGS += -DBOARD_PCA10059
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DCONFIG_NFCT_PINS_AS_GPIOS
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DNRF52840_XXAA
CFLAGS += -DNRF_SD_BLE_API_VERSION=6
CFLAGS += -DS140
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DSWI_DISABLE0
CFLAGS += -DGIT_VERSION=\"$(G_VERSION)\"
CFLAGS += -DDEBUG_COLOR
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -mlong-calls # THIS IS POTENTIALLY SLOW!
CFLAGS += -DAPP_FIRMWARE_VERSION=$(APP_VERSION)
ifndef RELEASE
CFLAGS += -DDEBUG
CFLAGS += -DDONT_RESTART_ASSERT
else
CFLAGS += -DDEBUG_DISABLED
endif

# Warnings
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror 
CFLAGS += -Wduplicated-cond
CFLAGS += -Wduplicated-branches
CFLAGS += -Wlogical-op
CFLAGS += -Wnull-dereference
CFLAGS += -Wjump-misses-init
CFLAGS += -Wshadow
CFLAGS += -Wpointer-arith
CFLAGS += -Wdangling-else
CFLAGS += -Wrestrict
CFLAGS += -Wdouble-promotion
CFLAGS += -Wvla
# Warnings disabled
CFLAGS += -Wno-expansion-to-defined
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-clobbered

CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums
ifndef RELEASE
#CFLAGS += -fstack-protector-all # Enable stack guards in debug mode
endif

CFLAGS += -BUFFER_MAX_POOL_BUFFERS=128
CFLAGS += -BLE_MAX_BUFFERS=128
CFLAGS += -DNRF_DFU_SETTINGS_VERSION=1

# C++ flags common to all targets
CXXFLAGS += $(OPT)

# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DBOARD_PCA10059
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DNRF52840_XXAA
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=6
ASMFLAGS += -DS140
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -DNRF_DFU_SETTINGS_VERSION=1
ASMFLAGS += -DAPP_FIRMWARE_VERSION=$(APP_VERSION)

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs
LDFLAGS += -u _printf_float
LDFLAGS += -u _scanf_float

$(TARGETS): CFLAGS += -D__HEAP_SIZE=4096
$(TARGETS): CFLAGS += -D__STACK_SIZE=65536
$(TARGETS): ASMFLAGS += -D__HEAP_SIZE=4096
$(TARGETS): ASMFLAGS += -D__STACK_SIZE=65536

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm


.PHONY: default help

# Default target - first one defined
default: $(TARGETS)

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo		flash_softdevice
	@echo		sdk_config - starting external tool for editing sdk_config.h
	@echo		flash      - flashing binary

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc


include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash cppcheck flash_softdevice erase

all: merge_softdevice
	rm -f $(OUTPUT_DIRECTORY)/$(TARGETS).hex # Remove the application files
	rm -f $(OUTPUT_DIRECTORY)/$(TARGETS).bin

# Flash the program
flash: $(OUTPUT_DIRECTORY)/$(TARGETS).hex merge_softdevice
	@echo Flashing: $(OUTPUT_DIRECTORY)/$(TARGETS)_merged.hex
	nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/$(TARGETS)_merged.hex --sectorerase
	nrfjprog -f nrf52 --reset

cppcheck:
	cppcheck -j 4 --enable=all --inconclusive --force --suppress=invalidPrintfArgType_uint --suppress=invalidPrintfArgType_sint --suppress=invalidPrintfArgType_float --xml --xml-version=2 $(CORE_DIR) $(DEVICE_DIR) $(SYSHAL_DIR) $(INCLUDE_WITH_I) 2> cppcheck.xml

# Flash softdevice
flash_softdevice:
	@echo Flashing: s140_nrf52_6.1.1_softdevice.hex
	nrfjprog -f nrf52 --program $(SDK_ROOT)/components/softdevice/s140/hex/s140_nrf52_6.1.1_softdevice.hex --sectorerase
	nrfjprog -f nrf52 --reset

dfu_package: $(OUTPUT_DIRECTORY)/$(TARGETS).hex
	@echo Generating the dfu package $(OUTPUT_DIRECTORY)/$(TARGETS)_package.zip
	nrfutil pkg generate --hw-version 52 --sd-req 0xAE,0xB6 --application-version $(APP_VERSION) --application $(OUTPUT_DIRECTORY)/$(TARGETS).hex --key-file nrfutil_pkg_key.pem $(OUTPUT_DIRECTORY)/$(TARGETS)_package.zip

dfu_package_with_softdevice: $(OUTPUT_DIRECTORY)/$(TARGETS).hex
	@echo Generating the dfu package $(OUTPUT_DIRECTORY)/$(TARGETS)_package.zip
	nrfutil pkg generate --hw-version 52 --sd-req 0xAE,0xB6 --sd-id 0xB6 --softdevice $(SDK_ROOT)/components/softdevice/s140/hex/s140_nrf52_6.1.1_softdevice.hex --application-version $(APP_VERSION) --application $(OUTPUT_DIRECTORY)/$(TARGETS).hex --key-file nrfutil_pkg_key.pem $(OUTPUT_DIRECTORY)/$(TARGETS)_package.zip

# Flash the program using the built in bootloader and nrfutil
flash_dfu: dfu_package
	@echo Flashing $(OUTPUT_DIRECTORY)/$(TARGETS)_package.zip to device $(DEVICE)
	nrfutil dfu usb-serial -pkg $(OUTPUT_DIRECTORY)/$(TARGETS)_package.zip -p $(DEVICE)

# Flash the program and softdevice using the built in bootloader and nrfutil
flash_dfu_with_softdevice: dfu_package_with_softdevice
	@echo Flashing $(OUTPUT_DIRECTORY)/$(TARGETS)_package.zip to device $(DEVICE)
	nrfutil dfu usb-serial -pkg $(OUTPUT_DIRECTORY)/$(TARGETS)_package.zip -p $(DEVICE)

# Merge the softdevice with the application
merge_softdevice: $(OUTPUT_DIRECTORY)/$(TARGETS).hex
	@echo Merging: s140_nrf52_6.1.1_softdevice.hex and $(TARGETS).hex
	mergehex -m $(SDK_ROOT)/components/softdevice/s140/hex/s140_nrf52_6.1.1_softdevice.hex $(OUTPUT_DIRECTORY)/$(TARGETS).hex -o $(OUTPUT_DIRECTORY)/$(TARGETS)_merged.hex
	$(OBJCOPY) -I ihex --output-target=binary $(OUTPUT_DIRECTORY)/$(TARGETS)_merged.hex $(OUTPUT_DIRECTORY)/$(TARGETS)_merged.bin

erase:
	nrfjprog -f nrf52 --eraseall

SDK_CONFIG_FILE := $(BOARD_DIR)/sdk_config.h
CMSIS_CONFIG_TOOL := $(SDK_ROOT)/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar
sdk_config:
	java -jar $(CMSIS_CONFIG_TOOL) $(SDK_CONFIG_FILE)
