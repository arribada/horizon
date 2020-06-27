######################################
# target
######################################
TARGET = ArribadaSTM32

######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -O2

BOARD = arribada_v2

#######################################
# paths
#######################################
# source path
STMHAL_DIR = drivers/STM32F0xx_HAL_Driver
CMSIS_DIR = drivers/CMSIS
USB_DIR = drivers/STM32_USB_Device_Library
CORE_DIR = ../../core
DEVICE_DIR = ../../devices
SYSHAL_DIR = ../../syshal

# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES =  \
$(CORE_DIR)/buffer/buffer.c \
$(CORE_DIR)/cexception/cexception.c \
$(CORE_DIR)/config_if/config_if.c \
$(CORE_DIR)/crc32/crc32.c \
$(CORE_DIR)/debug/debug.c \
$(CORE_DIR)/fs/fs.c \
$(CORE_DIR)/sm/sm.c \
$(CORE_DIR)/sm_main/sm_main.c \
$(CORE_DIR)/sys_config/sys_config.c \
$(DEVICE_DIR)/axl/LSM9DS1/LSM9DS1.c \
$(DEVICE_DIR)/battery/BQ27621/BQ27621.c \
$(DEVICE_DIR)/ble/nRF52x/syshal_ble.c \
$(DEVICE_DIR)/gps/M8N/M8N.c \
$(DEVICE_DIR)/flash/S25FL128/S25FL128.c \
$(DEVICE_DIR)/pressure/MS5837_xBA/MS5837_xBA.c \
$(DEVICE_DIR)/pressure/MS5837_xBA/MS5837_30BA/MS5837_30BA.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_cortex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_dma.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_flash.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_flash_ex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_gpio.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_i2c.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_i2c_ex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_pcd.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_pcd_ex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_pwr.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_pwr_ex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_rcc.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_rcc_ex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_rtc.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_rtc_ex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_spi.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_spi_ex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_tim.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_tim_ex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_uart.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_uart_ex.c \
$(STMHAL_DIR)/Src/stm32f0xx_hal_iwdg.c \
$(USB_DIR)/Core/Src/usbd_core.c \
$(USB_DIR)/Core/Src/usbd_ctlreq.c \
$(USB_DIR)/Core/Src/usbd_ioreq.c \
usb/usbd_conf.c \
usb/usbd_desc.c \
usb/usbd_vendor.c \
$(SYSHAL_DIR)/src/syshal_switch.c \
$(SYSHAL_DIR)/src/syshal_timer.c \
syshal/syshal_firmware.c \
syshal/syshal_gpio.c \
syshal/syshal_i2c.c \
syshal/syshal_pmu.c \
syshal/syshal_rtc.c \
syshal/syshal_spi.c \
syshal/syshal_time.c \
syshal/syshal_uart.c \
syshal/syshal_usb.c \
boards/$(BOARD)/bsp.c \
stm32f0xx_hal_msp.c \
stm32f0xx_it.c \
system_stm32f0xx.c \
system_clock.c \
main.c

# ASM sources
ASM_SOURCES =  \
startup_stm32f072xb.s

#######################################
# binaries
#######################################
OPENOCD = "../../../Toolchain/OpenOCD-20170821/bin/openocd"
OPENOCDSCRIPTS = "/../../../Toolchain/OpenOCD-20170821/share/openocd/scripts"

# Allow BINPATH to be passed in to make if not set
BINPATH ?= "../../../Toolchain/gcc-arm-none-eabi-7-2018-q2-update-win32/bin/"

JLINK = "JLink"
PREFIX = arm-none-eabi-
CC = $(BINPATH)$(PREFIX)gcc -pipe
AS = $(BINPATH)$(PREFIX)gcc -x assembler-with-cpp
CP = $(BINPATH)$(PREFIX)objcopy
AR = $(BINPATH)$(PREFIX)ar
SZ = $(BINPATH)$(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
# Get GIT version
G_VERSION := $(shell git describe --dirty --always --tags)

#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m0

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F072xB \
-DDEBUG_COLOR \
-DGIT_VERSION=\"$(G_VERSION)\" \
-DDEBUG_DISABLED

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES =  \
-I$(CMSIS_DIR)/Device/ST/STM32F0xx/Include \
-I$(CMSIS_DIR)/Include \
-I$(CORE_DIR)/buffer \
-I$(CORE_DIR)/cexception \
-I$(CORE_DIR)/config \
-I$(CORE_DIR)/config_if \
-I$(CORE_DIR)/crc32 \
-I$(CORE_DIR)/debug \
-I$(CORE_DIR)/fs \
-I$(CORE_DIR)/logging \
-I$(CORE_DIR)/ring_buffer \
-I$(CORE_DIR)/sm \
-I$(CORE_DIR)/sm_main/ \
-I$(CORE_DIR)/sys_config \
-I$(DEVICE_DIR)/axl/LSM9DS1/ \
-I$(DEVICE_DIR)/battery/BQ27621/ \
-I$(DEVICE_DIR)/ble/nRF52x/ \
-I$(DEVICE_DIR)/gps/M8N/ \
-I$(DEVICE_DIR)/flash/S25FL128/ \
-I$(DEVICE_DIR)/pressure/MS5837_xBA/ \
-I$(STMHAL_DIR)/Inc \
-I$(STMHAL_DIR)/Inc/Legacy \
-I$(USB_DIR)/Core/Inc \
-I$(SYSHAL_DIR)/inc \
-Iboards/$(BOARD) \
-Iusb \
-I.

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -Werror -fdata-sections -ffunction-sections
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -Werror -fdata-sections -ffunction-sections -mlong-calls

ifeq ($(DEBUG), 1)
CFLAGS += -g3
endif


# Generate dependency information
CFLAGS += -MMD -MP

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32F072RBTx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT) Makefile 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@

#######################################
# flash the application
#######################################

flash:
	$(MAKE) all
	$(JLINK) -device STM32F072VB -if SWD -speed 4000 -autoconnect 1 -CommanderScript JLinkCommandFile.jlink

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)
  
#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d) 
