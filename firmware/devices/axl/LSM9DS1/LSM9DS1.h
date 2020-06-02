/* LSM9DS1.h - HAL for accelerometer device
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

#ifndef _LSM9DS1_REGISTERS_H_
#define _LSM9DS1_REGISTERS_H_

#define LSM9D1_AG_ADDR   (0x6A)
#define LSM9D1_MAG_ADDR  (0x1C)

// LSM9DS1 Accel/Gyro (XL/G) Registers
#define LSM9D1_ACT_THS             (0x04)
#define LSM9D1_ACT_DUR             (0x05)
#define LSM9D1_INT_GEN_CFG_XL      (0x06)
#define LSM9D1_INT_GEN_THS_X_XL    (0x07)
#define LSM9D1_INT_GEN_THS_Y_XL    (0x08)
#define LSM9D1_INT_GEN_THS_Z_XL    (0x09)
#define LSM9D1_INT_GEN_DUR_XL      (0x0A)
#define LSM9D1_REFERENCE_G         (0x0B)
#define LSM9D1_INT1_CTRL           (0x0C)
#define LSM9D1_INT2_CTRL           (0x0D)
#define LSM9D1_WHO_AM_I_XG         (0x0F)
#define LSM9D1_CTRL_REG1_G         (0x10)
#define LSM9D1_CTRL_REG2_G         (0x11)
#define LSM9D1_CTRL_REG3_G         (0x12)
#define LSM9D1_ORIENT_CFG_G        (0x13)
#define LSM9D1_INT_GEN_SRC_G       (0x14)
#define LSM9D1_OUT_TEMP_L          (0x15)
#define LSM9D1_OUT_TEMP_H          (0x16)
#define LSM9D1_STATUS_REG_0        (0x17)
#define LSM9D1_OUT_X_L_G           (0x18)
#define LSM9D1_OUT_X_H_G           (0x19)
#define LSM9D1_OUT_Y_L_G           (0x1A)
#define LSM9D1_OUT_Y_H_G           (0x1B)
#define LSM9D1_OUT_Z_L_G           (0x1C)
#define LSM9D1_OUT_Z_H_G           (0x1D)
#define LSM9D1_CTRL_REG4           (0x1E)
#define LSM9D1_CTRL_REG5_XL        (0x1F)
#define LSM9D1_CTRL_REG6_XL        (0x20)
#define LSM9D1_CTRL_REG7_XL        (0x21)
#define LSM9D1_CTRL_REG8           (0x22)
#define LSM9D1_CTRL_REG9           (0x23)
#define LSM9D1_CTRL_REG10          (0x24)
#define LSM9D1_INT_GEN_SRC_XL      (0x26)
#define LSM9D1_STATUS_REG_1        (0x27)
#define LSM9D1_OUT_X_L_XL          (0x28)
#define LSM9D1_OUT_X_H_XL          (0x29)
#define LSM9D1_OUT_Y_L_XL          (0x2A)
#define LSM9D1_OUT_Y_H_XL          (0x2B)
#define LSM9D1_OUT_Z_L_XL          (0x2C)
#define LSM9D1_OUT_Z_H_XL          (0x2D)
#define LSM9D1_FIFO_CTRL           (0x2E)
#define LSM9D1_FIFO_SRC            (0x2F)
#define LSM9D1_INT_GEN_CFG_G       (0x30)
#define LSM9D1_INT_GEN_THS_XH_G    (0x31)
#define LSM9D1_INT_GEN_THS_XL_G    (0x32)
#define LSM9D1_INT_GEN_THS_YH_G    (0x33)
#define LSM9D1_INT_GEN_THS_YL_G    (0x34)
#define LSM9D1_INT_GEN_THS_ZH_G    (0x35)
#define LSM9D1_INT_GEN_THS_ZL_G    (0x36)
#define LSM9D1_INT_GEN_DUR_G       (0x37)

// LSM9DS1 Magnetometer Registers
#define LSM9D1_OFFSET_X_REG_L_M    (0x05)
#define LSM9D1_OFFSET_X_REG_H_M    (0x06)
#define LSM9D1_OFFSET_Y_REG_L_M    (0x07)
#define LSM9D1_OFFSET_Y_REG_H_M    (0x08)
#define LSM9D1_OFFSET_Z_REG_L_M    (0x09)
#define LSM9D1_OFFSET_Z_REG_H_M    (0x0A)
#define LSM9D1_WHO_AM_I_M          (0x0F)
#define LSM9D1_CTRL_REG1_M         (0x20)
#define LSM9D1_CTRL_REG2_M         (0x21)
#define LSM9D1_CTRL_REG3_M         (0x22)
#define LSM9D1_CTRL_REG4_M         (0x23)
#define LSM9D1_CTRL_REG5_M         (0x24)
#define LSM9D1_STATUS_REG_M        (0x27)
#define LSM9D1_OUT_X_L_M           (0x28)
#define LSM9D1_OUT_X_H_M           (0x29)
#define LSM9D1_OUT_Y_L_M           (0x2A)
#define LSM9D1_OUT_Y_H_M           (0x2B)
#define LSM9D1_OUT_Z_L_M           (0x2C)
#define LSM9D1_OUT_Z_H_M           (0x2D)
#define LSM9D1_INT_CFG_M           (0x30)
#define LSM9D1_INT_SRC_M           (0x31)
#define LSM9D1_INT_THS_L_M         (0x32)
#define LSM9D1_INT_THS_H_M         (0x33)

// LSM9DS1 WHO_AM_I Responses
#define LSM9D1_WHO_AM_I_AG_RSP     (0x68)
#define LSM9D1_WHO_AM_I_M_RSP      (0x3D)

// INT1_CTRL
#define LSM9D1_INT1_CTRL_INT_DRDY_XL (1 << 0) // Accelerometer data ready on INT 1_A/G pin

// CTRL_REG5_XL
#define LSM9D1_CTRL_REG5_XL_ZEN_XL (1 << 5)
#define LSM9D1_CTRL_REG5_XL_YEN_XL (1 << 4)
#define LSM9D1_CTRL_REG5_XL_XEN_XL (1 << 3)

// CTRL_REG6_XL
#define LSM9D1_CTRL_REG6_XL_FS_XL_POS   (3)
#define LSM9D1_CTRL_REG6_XL_FS_XL_2G    (0x00 << LSM9D1_CTRL_REG6_XL_FS_XL_POS)
#define LSM9D1_CTRL_REG6_XL_FS_XL_4G    (0x02 << LSM9D1_CTRL_REG6_XL_FS_XL_POS)
#define LSM9D1_CTRL_REG6_XL_FS_XL_8G    (0x03 << LSM9D1_CTRL_REG6_XL_FS_XL_POS)
#define LSM9D1_CTRL_REG6_XL_FS_XL_16G   (0x01 << LSM9D1_CTRL_REG6_XL_FS_XL_POS)

#define LSM9D1_CTRL_REG6_XL_ODR_XL_POS  (5)
#define LSM9D1_CTRL_REG6_XL_ODR_XL_MASK (0x07 << LSM9D1_CTRL_REG6_XL_ODR_XL_POS)

#define LSM9D1_CTRL_REG6_XL_ODR_XL_POWER_DOWN   (0x00 << LSM9D1_CTRL_REG6_XL_ODR_XL_POS)
#define LSM9D1_CTRL_REG6_XL_ODR_XL_10HZ         (0x01 << LSM9D1_CTRL_REG6_XL_ODR_XL_POS)
#define LSM9D1_CTRL_REG6_XL_ODR_XL_50HZ         (0x02 << LSM9D1_CTRL_REG6_XL_ODR_XL_POS)
#define LSM9D1_CTRL_REG6_XL_ODR_XL_119HZ        (0x03 << LSM9D1_CTRL_REG6_XL_ODR_XL_POS)
#define LSM9D1_CTRL_REG6_XL_ODR_XL_238HZ        (0x04 << LSM9D1_CTRL_REG6_XL_ODR_XL_POS)
#define LSM9D1_CTRL_REG6_XL_ODR_XL_476HZ        (0x05 << LSM9D1_CTRL_REG6_XL_ODR_XL_POS)
#define LSM9D1_CTRL_REG6_XL_ODR_XL_952HZ        (0x06 << LSM9D1_CTRL_REG6_XL_ODR_XL_POS)

#define LSM9D1_CTRL_REG1_G_FS_G_POS  (3)
#define LSM9D1_CTRL_REG1_G_ODR_G_POS (5)

#endif /* _LSM9DS1_REGISTERS_H_ */