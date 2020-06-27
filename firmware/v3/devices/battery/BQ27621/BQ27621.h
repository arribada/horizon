/* BQ27621.h - BQ27621-G1 fuel gauge definitions
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

// Datasheet: http://www.ti.com/product/bq27621-g1

#ifndef _BQ27621_H_
#define _BQ27621_H_

#define BQ27621_ADDR (0x55)

// Register addresses
#define BQ27621_REG_CTRL                             (0x00)
#define BQ27621_REG_TEMP                             (0x02)
#define BQ27621_REG_VOLT                             (0x04)
#define BQ27621_REG_FLAGS                            (0x06)
#define BQ27621_REG_NOMINAL_AVAILABLE_CAPACITY       (0x08)
#define BQ27621_REG_FULL_AVAILABLE_CAPACITY          (0x0A)
#define BQ27621_REG_REMAINING_CAPACITY               (0x0C)
#define BQ27621_REG_FULL_CHARGE_CAPACITY             (0x0E)
#define BQ27621_REG_EFFECTIVE_CURRENT                (0x10)
#define BQ27621_REG_AVERAGE_POWER                    (0x18)
#define BQ27621_REG_STATE_OF_CHARGE                  (0x1C)
#define BQ27621_REG_INTERNAL_TEMP                    (0x1E)
#define BQ27621_REG_REMAINING_CAPACITY_UNFILTERED    (0x28)
#define BQ27621_REG_REMAINING_CAPACITY_FILTERED      (0x2A)
#define BQ27621_REG_FULL_CHARGE_CAPACITY_UNFILTERED  (0x2C)
#define BQ27621_REG_FULL_CHARGE_CAPACITY_FILTERED    (0x2E)
#define BQ27621_REG_STATE_OF_CHARGE_UNFILTERED       (0x30)
#define BQ27621_REG_OPERATION_CONF                   (0x3A)

// Register sizes
#define BQ27621_REG_CTRL_SIZE                             (2)
#define BQ27621_REG_TEMP_SIZE                             (2)
#define BQ27621_REG_VOLT_SIZE                             (2)
#define BQ27621_REG_FLAGS_SIZE                            (2)
#define BQ27621_REG_NOMINAL_AVAILABLE_CAPACITY_SIZE       (2)
#define BQ27621_REG_FULL_AVAILABLE_CAPACITY_SIZE          (2)
#define BQ27621_REG_REMAINING_CAPACITY_SIZE               (2)
#define BQ27621_REG_FULL_CHARGE_CAPACITY_SIZE             (2)
#define BQ27621_REG_EFFECTIVE_CURRENT_SIZE                (2)
#define BQ27621_REG_AVERAGE_POWER_SIZE                    (2)
#define BQ27621_REG_STATE_OF_CHARGE_SIZE                  (2)
#define BQ27621_REG_INTERNAL_TEMP_SIZE                    (2)
#define BQ27621_REG_REMAINING_CAPACITY_UNFILTERED_SIZE    (2)
#define BQ27621_REG_REMAINING_CAPACITY_FILTERED_SIZE      (2)
#define BQ27621_REG_FULL_CHARGE_CAPACITY_UNFILTERED_SIZE  (2)
#define BQ27621_REG_FULL_CHARGE_CAPACITY_FILTERED_SIZE    (2)
#define BQ27621_REG_STATE_OF_CHARGE_UNFILTERED_SIZE       (2)
#define BQ27621_REG_OPERATION_CONF_SIZE                   (2)

// Control register params
#define BQ27621_CONTROL_STATUS   (0x0000) // Reports the status of device
#define BQ27621_DEVICE_TYPE      (0x0001) // Reports the device type (0x0621)
#define BQ27621_FW_VERSION       (0x0002) // Reports the firmware version of the device
#define BQ27621_PREV_MACWRITE    (0x0007) // Returns previous MAC command code
#define BQ27621_CHEM_ID          (0x0008) // Reports the chemical identifier of the battery profile currently used by the fuel gauging algorithm
#define BQ27621_BAT_INSERT       (0x000C) // Forces the Flags( ) [BAT_DET] bit set when the Op Config [BIE] bit is 0.
#define BQ27621_BAT_REMOVE       (0x000D) // Forces the Flags( ) [BAT_DET] bit clear when the Op Config [BIE] bit is 0.
#define BQ27621_TOGGLE_POWERMIN  (0x0010) // Sets CONTROL_STATUS [POWERMIN] to 1
#define BQ27621_SET_HIBERNATE    (0x0011) // Forces CONTROL_STATUS [HIBERNATE] to 1
#define BQ27621_CLEAR_HIBERNATE  (0x0012) // Forces CONTROL_STATUS [HIBERNATE] to 0
#define BQ27621_SET_CFGUPDATE    (0x0013) // Forces Flags( ) [CFGUPD] to 1 and gauge enters CONFIG UPDATE mode.
#define BQ27621_SHUTDOWN_ENABLE  (0x001B) // Enables device SHUTDOWN mode
#define BQ27621_SHUTDOWN         (0x001C) // Commands the device to enter SHUTDOWN mode
#define BQ27621_SEALED           (0x0020) // Places the device in SEALED access mode
#define BQ27621_TOGGLE_GPOUT     (0x0023) // Tests the GPIO pin by sending a pulse signa
#define BQ27621_ALT_CHEM1        (0x0031) // Selects alternate chemistry 1 (0x1210)
#define BQ27621_ALT_CHEM2        (0x0032) // Selects alternate chemistry 2 (0x354)
#define BQ27621_RESET            (0x0041) // Performs a full device reset
#define BQ27621_SOFT_RESET       (0x0042) // Gauge exits CONFIG UPDATE mode
#define BQ27621_EXIT_CFGUPDATE   (0x0043) // Exits CONFIG UPDATE mode without an OCV measurement and without resimulating to update StateOfCharge( )
#define BQ27621_EXIT_RESIM       (0x0044) // Exits CONFIG UPDATE mode without an OCV measurement and resimulates with the updated configuration data to update StateOfCharge( )

// Status bit definitions
#define BQ27621_STATUS_CHEMCHNG   (1 << 0)
#define BQ27621_STATUS_LDMD       (1 << 3)
#define BQ27621_STATUS_SLEEP      (1 << 4)
#define BQ27621_STATUS_POWERMIN   (1 << 5)
#define BQ27621_STATUS_HIBERNATE  (1 << 6)
#define BQ27621_STATUS_INITCOMP   (1 << 7)
#define BQ27621_STATUS_OCVFAIL    (1 << 8)
#define BQ27621_STATUS_OCVCMDCOMP (1 << 9)
#define BQ27621_STATUS_CALMODE    (1 << 12)
#define BQ27621_STATUS_SS         (1 << 13)
#define BQ27621_STATUS_WDRESET    (1 << 14)
#define BQ27621_STATUS_SHUTDOWNEN (1 << 15)

// Flags register bit definitions
#define BQ27621_FLAG_DSG      (1 << 0)  // Discharging detected
#define BQ27621_FLAG_SOCF     (1 << 1)  // State-of-Charge threshold final
#define BQ27621_FLAG_SOC1     (1 << 2)  // State-of-Charge threshold 1
#define BQ27621_FLAG_BAT_DET  (1 << 3)  // Battery insertion detected
#define BQ27621_FLAG_CFGUPD   (1 << 4)  // Fuel gauge is in CONFIG UPDATE mode
#define BQ27621_FLAG_ITPOR    (1 << 5)  // Indicates a power-on reset or RESET has occured
#define BQ27621_FLAG_OCVTAKEN (1 << 7)  // Cleared on entry to relax mode and set to 1 when OCV measurement is performed in relax
#define BQ27621_FLAG_CHG      (1 << 8)  // Fast charging allowed
#define BQ27621_FLAG_FC       (1 << 9)  // Full charge detected
#define BQ27621_FLAG_UT       (1 << 14) // Under temperature condition
#define BQ27621_FLAG_OT       (1 << 15) // Over temperature condition

#endif /* _BQ27621_H_ */