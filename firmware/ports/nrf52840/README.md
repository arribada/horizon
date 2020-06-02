# Horizon Application

## How to build
The Horizon Application project uses CMake and the arm-none-eabi-gcc compiler.
The instructions below describe how to build and flash this application.

**Please ensure the Bootloader is flashed to the device first.** This can be found in the `bootloader/` sub-directory

### Debug Version
```
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain_arm_gcc_nrf52.cmake -DCMAKE_BUILD_TYPE=Debug .
make 
```

### Release Version
```
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain_arm_gcc_nrf52.cmake -DCMAKE_BUILD_TYPE=Release .
make
```

## How to flash the firmware
With the device connected via USB run the following command
```
nrfutil dfu usb-serial -pkg PACKAGE_dfu.zip -p DEVICE
```
Where:
* `PACKAGE` is the DFU zip file created by running `make`
* `DEVICE` is the USB device as it appears to the OS (e.g. COM7 or ttyACM0)
