# Horizon Bootloader

## How to build
The Horizon Bootloader project uses CMake and the arm-none-eabi-gcc compiler.
The instructions below describe how to build and flash this application.

```
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain_arm_gcc_nrf52.cmake .
make
```

## How to flash the firmware
With the device connected via a JTAG debugger run:
```
make flash
```
**Please note this will erase all firmware currently on the device**