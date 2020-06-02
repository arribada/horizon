/* ARTIC.h - ARTIC satellite module definitions
 *
 * Copyright (C) 2019 Arribada
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

#ifndef _ARTIC_H_
#define _ARTIC_H_

#include <stdint.h>

#ifndef GTEST
#define SYSHAL_SAT_TURN_ON_TIME_S  (16)
#else
#define SYSHAL_SAT_TURN_ON_TIME_S  ( 0) // Note: Unit testing makes more sense if we ignore this parameter
#endif
#define SYSHAL_SAT_DELAY                     (1000)
#define SYSHAL_SAT_ARTIC_DELAY_BOOT          (1000)
#define SYSHAL_SAT_ARTIC_DELAY_INTERRUPT     (10000)
#define SYSHAL_SAT_ARTIC_TIMEOUT_SEND_TX     (20000)
#define SYSHAL_SAT_ARTIC_DELAY_RESET         (5)
#define SYSHAL_SAT_ARTIC_DELAY_BURST         (5)
#define SYSHAL_SAT_ARTIC_DELAY_SET_BURST     (22)
#define SYSHAL_SAT_ARTIC_DELAY_FINISH_BURST  (13)
#define SYSHAL_SAT_ARTIC_DELAY_TRANSFER      (50)
#define SYSHAL_SAT_ARTIC_DELAY_BURST         (5)

#define NUM_FIRMWARE_FILES_ARTIC 3

#define SYSHAL_SAT_GPIO_POWER_ON  (GPIO_EXT2_GPIO1)
#define SYSHAL_SAT_GPIO_RESET     (GPIO_EXT2_GPIO2)
#define SYSHAL_SAT_GPIO_INT_1     (GPIO_EXT2_GPIO3)
#define SYSHAL_SAT_GPIO_INT_2     (GPIO_EXT2_GPIO4)

#define BURST_ADDRESS  (0)
#define ADDRESS_DSP    (1)

#define NUM_BYTES_BEFORE_WAIT   60

#define ARTIC_WRITE_ADDRESS(x) (((x << 1) & 0xF7))
#define ARTIC_READ_ADDRESS(x)  (((x << 1) & 0xF7) | 1) // LSB set to indicate a read

#define BURST_MODE_SHIFT_BITMASK    (0x080000)
#define MEM_SEL_BITMASK             (0x060000)
#define MEM_SEL_SHIFT               (17)
#define BURST_R_NW_MODE_BITMASK     (0x010000)
#define BURST_START_ADDRESS_BITMASK (0x00FFFF)

#define CRC_ADDRESS         (0x0371)
#define INTERRUPT_ADDRESS   (0x8018)
#define TX_PAYLOAD_ADDRESS  (0x0273)

#define FIRMWARE_ADDRESS_LENGTH       (3)
#define SIZE_SPI_REG_XMEM_YMEM_IOMEM  (3)
#define SIZE_SPI_REG_PMEM             (4)

#define SPI_MAX_BYTE_READ  (8192)

#define MAX_BURST  (2048)

#define MAX_TX_SIZE_BYTES  (31)

#define MAX_BUFFER_READ  (256)

#define INTERRUPT_1  (1)
#define INTERRUPT_2  (2)

#define TOTAL_NUMBER_STATUS_FLAG  (25)



#define ID_SIZE_BYTE (4)

#define ARTIC_PACKET_HDR_BYTES (3)

#define ARTIC_MSG_LEN_BITS      ( 4)
#define ARTIC_MSG_ID_SIZE_BITS  (28)
#define ARTIC_MSG_ID_BITMASK    (0x0FFFFFFF)

#define ARTIC_ZTE_MAX_USER_BITS            (0)
#define ARTIC_ZTE_MAX_USER_BYTES           (0)
#define ARTIC_ZTE_MSG_NUM_TAIL_BITS        (8)
#define ARTIC_ZTE_MSG_TOTAL_BITS           (ARTIC_MSG_ID_SIZE_BITS + ARTIC_ZTE_MAX_USER_BITS + ARTIC_ZTE_MSG_NUM_TAIL_BITS)
#define ARTIC_ZTE_BYTES_TO_SEND            (9) // CEIL(ARTIC_ZTE_MSG_TOTAL_BITS + ARTIC_MSG_LEN_BITS) / 8) + ARTIC_PACKET_HDR_BYTES // Must be divisible by 3

#define ARTIC_PTT_A3_24_MAX_USER_BITS      (24)
#define ARTIC_PTT_A3_24_MAX_USER_BYTES     ((unsigned int) ARTIC_PTT_A3_24_MAX_USER_BITS / 8)
#define ARTIC_PTT_A3_24_MSG_LEN_FIELD      (0x00)
#define ARTIC_PTT_A3_24_MSG_NUM_TAIL_BITS  (7)
#define ARTIC_PTT_A3_24_MSG_TOTAL_BITS     (ARTIC_MSG_LEN_BITS + ARTIC_MSG_ID_SIZE_BITS + ARTIC_PTT_A3_24_MAX_USER_BITS + ARTIC_PTT_A3_24_MSG_NUM_TAIL_BITS)
#define ARTIC_PTT_A3_24_BYTES_TO_SEND      (12)

#define ARTIC_PTT_A3_56_MAX_USER_BITS      (56)
#define ARTIC_PTT_A3_56_MAX_USER_BYTES     ((unsigned int) ARTIC_PTT_A3_56_MAX_USER_BITS / 8)
#define ARTIC_PTT_A3_56_MSG_LEN_FIELD      (0x30)
#define ARTIC_PTT_A3_56_MSG_NUM_TAIL_BITS  (8)
#define ARTIC_PTT_A3_56_MSG_TOTAL_BITS     (ARTIC_MSG_LEN_BITS + ARTIC_MSG_ID_SIZE_BITS + ARTIC_PTT_A3_56_MAX_USER_BITS + ARTIC_PTT_A3_56_MSG_NUM_TAIL_BITS)
#define ARTIC_PTT_A3_56_BYTES_TO_SEND      (15)

#define ARTIC_PTT_A3_88_MAX_USER_BITS      (88)
#define ARTIC_PTT_A3_88_MAX_USER_BYTES     ((unsigned int) ARTIC_PTT_A3_88_MAX_USER_BITS / 8)
#define ARTIC_PTT_A3_88_MSG_LEN_FIELD      (0x50)
#define ARTIC_PTT_A3_88_MSG_NUM_TAIL_BITS  (9)
#define ARTIC_PTT_A3_88_MSG_TOTAL_BITS     (ARTIC_MSG_LEN_BITS + ARTIC_MSG_ID_SIZE_BITS + ARTIC_PTT_A3_88_MAX_USER_BITS + ARTIC_PTT_A3_88_MSG_NUM_TAIL_BITS)
#define ARTIC_PTT_A3_88_BYTES_TO_SEND      (21)

#define ARTIC_PTT_A3_120_MAX_USER_BITS     (120)
#define ARTIC_PTT_A3_120_MAX_USER_BYTES    ((unsigned int) ARTIC_PTT_A3_120_MAX_USER_BITS / 8)
#define ARTIC_PTT_A3_120_MSG_LEN_FIELD     (0x60)
#define ARTIC_PTT_A3_120_MSG_NUM_TAIL_BITS (7)
#define ARTIC_PTT_A3_120_MSG_TOTAL_BITS    (ARTIC_MSG_LEN_BITS + ARTIC_MSG_ID_SIZE_BITS + ARTIC_PTT_A3_120_MAX_USER_BITS + ARTIC_PTT_A3_120_MSG_NUM_TAIL_BITS)
#define ARTIC_PTT_A3_120_BYTES_TO_SEND     (24)

#define ARTIC_PTT_A3_152_MAX_USER_BITS     (152)
#define ARTIC_PTT_A3_152_MAX_USER_BYTES    ((unsigned int) ARTIC_PTT_A3_152_MAX_USER_BITS / 8)
#define ARTIC_PTT_A3_152_MSG_LEN_FIELD     (0x90)
#define ARTIC_PTT_A3_152_MSG_NUM_TAIL_BITS (8)
#define ARTIC_PTT_A3_152_MSG_TOTAL_BITS    (ARTIC_MSG_LEN_BITS + ARTIC_MSG_ID_SIZE_BITS + ARTIC_PTT_A3_152_MAX_USER_BITS + ARTIC_PTT_A3_152_MSG_NUM_TAIL_BITS)
#define ARTIC_PTT_A3_152_BYTES_TO_SEND     (27)

#define ARTIC_PTT_A3_184_MAX_USER_BITS     (184)
#define ARTIC_PTT_A3_184_MAX_USER_BYTES    ((unsigned int) ARTIC_PTT_A3_184_MAX_USER_BITS / 8)
#define ARTIC_PTT_A3_184_MSG_LEN_FIELD     (0xA0)
#define ARTIC_PTT_A3_184_MSG_NUM_TAIL_BITS (9)
#define ARTIC_PTT_A3_184_MSG_TOTAL_BITS    (ARTIC_MSG_LEN_BITS + ARTIC_MSG_ID_SIZE_BITS + ARTIC_PTT_A3_184_MAX_USER_BITS + ARTIC_PTT_A3_184_MSG_NUM_TAIL_BITS)
#define ARTIC_PTT_A3_184_BYTES_TO_SEND     (33)

#define ARTIC_PTT_A3_216_MAX_USER_BITS     (216)
#define ARTIC_PTT_A3_216_MAX_USER_BYTES    ((unsigned int) ARTIC_PTT_A3_216_MAX_USER_BITS / 8)
#define ARTIC_PTT_A3_216_MSG_LEN_FIELD     (0xC0)
#define ARTIC_PTT_A3_216_MSG_NUM_TAIL_BITS (7)
#define ARTIC_PTT_A3_216_MSG_TOTAL_BITS    (ARTIC_MSG_LEN_BITS + ARTIC_MSG_ID_SIZE_BITS + ARTIC_PTT_A3_216_MAX_USER_BITS + ARTIC_PTT_A3_216_MSG_NUM_TAIL_BITS)
#define ARTIC_PTT_A3_216_BYTES_TO_SEND     (36)

#define ARTIC_PTT_A3_248_MAX_USER_BITS     (248)
#define ARTIC_PTT_A3_248_MAX_USER_BYTES    ((unsigned int) ARTIC_PTT_A3_248_MAX_USER_BITS / 8)
#define ARTIC_PTT_A3_248_MSG_LEN_FIELD     (0xF0)
#define ARTIC_PTT_A3_248_MSG_NUM_TAIL_BITS (8)
#define ARTIC_PTT_A3_248_MSG_TOTAL_BITS    (ARTIC_MSG_LEN_BITS + ARTIC_MSG_ID_SIZE_BITS + ARTIC_PTT_A3_248_MAX_USER_BITS + ARTIC_PTT_A3_248_MSG_NUM_TAIL_BITS)
#define ARTIC_PTT_A3_248_BYTES_TO_SEND     (39)

#define ARTIC_MSG_MAX_SIZE (ARTIC_PTT_A3_248_BYTES_TO_SEND)

#define TAIL_BITS_VALUE  0

#define MAX_WORD_A3 7
#define TOTAL_LEN_MAX_A3 288

#define MAXIMUM_READ_FIRMWARE_OPERATION (FIRMWARE_ADDRESS_LENGTH + SIZE_SPI_REG_PMEM)

// COMMANDS //

// Artic setting commands
#define ARTIC_CMD_SET_ARGOS_4_RX_MODE          (0x01)  // 00000001b
#define ARTIC_CMD_SET_ARGOS_3_RX_MODE          (0x02)  // 00000010b
#define ARTIC_CMD_SET_ARGOS_3_RX_BACKUP_MODE   (0x03)  // 00000011b
#define ARTIC_CMD_SET_PTT_A2_TX_MODE           (0x04)  // 00000100b
#define ARTIC_CMD_SET_PTT_A3_TX_MODE           (0x05)  // 00000101b
#define ARTIC_CMD_SET_PTT_ZE_TX_MODE           (0x06)  // 00000110b
#define ARTIC_CMD_SET_ARGOS_3_PTT_HD_TX_MODE   (0x07)  // 00000111b
#define ARTIC_CMD_SET_ARGOS_4_PTT_HD_TX_MODE   (0x08)  // 00001000b
#define ARTIC_CMD_SET_ARGOS_4_PTT_MD_TX_MODE   (0x09)  // 00001001b
#define ARTIC_CMD_SET_PTT_VLD_TX_MODE          (0x0A)  // 00001010b

// Artic instruction commands
#define ARTIC_CMD_START_RX_CONT                (0x41)  // 01000001b
#define ARTIC_CMD_START_RX_1M                  (0x42)  // 01000010b
#define ARTIC_CMD_START_RX_2M                  (0x43)  // 01000011b
#define ARTIC_CMD_START_RX_TIMED               (0x46)  // 01000110b
#define ARTIC_CMD_START_TX_1M_SLEEP            (0x48)  // 01001000b
#define ARTIC_CMD_START_TX_1M_RX_TIMED         (0x49)  // 01001001b
#define ARTIC_CMD_SLEEP                        (0x50)  // 01010000b
#define ARTIC_CMD_SATELLITE_DETECTION          (0x55)  // 01010101b

// Artic commands to clear interrupt flags
#define ARTIC_CMD_CLEAR_INT1  (0x80)   // 10XXXXXXb
#define ARTIC_CMD_CLEAR_INT2  (0xC0)   // 11XXXXXXb    

// Because of an issue with the MCP2210, not more than 8192 bytes can be transmitted at once.

typedef enum
{
    // Current Firmware state //
    IDLE,                                   // The firmware is idle and ready to accept commands.
    RX_IN_PROGRESS,                         // The firmware is receiving.
    TX_IN_PROGRESS,                         // The firmware is transmitting.
    BUSY,                                   // The firmware is changing state.

    // Interrupt 1 flags //
    RX_VALID_MESSAGE,                       // A message has been received.
    RX_SATELLITE_DETECTED,                  // A satellite has been detected.
    TX_FINISHED,                            // The transmission was completed.
    MCU_COMMAND_ACCEPTED,                   // The configuration command has been accepted.
    CRC_CALCULATED,                         // CRC calculation has finished.
    IDLE_STATE,                             // Firmware returned to the idle state.
    RX_CALIBRATION_FINISHED,                // RX offset calibration has completed.
    RESERVED_11,
    RESERVED_12,

    // Interrupt 2 flags //
    RX_TIMEOUT,                             // The specified reception time has been exceeded.
    SATELLITE_TIMEOUT,                      // No satellite was detected within the specified time.
    RX_BUFFER_OVERFLOW,                     // A received message is lost. No buffer space left.
    TX_INVALID_MESSAGE,                     // Incorrect TX payload length specified.
    MCU_COMMAND_REJECTED,                   // Incorrect command send or Firmware is not in idle.
    MCU_COMMAND_OVERFLOW,                   // Previous command was not yet processed.
    RESERVED_19,
    RESERVER_20,

    // Misc //
    INTERNAL_ERROR,                         // An internal error has occurred.
    dsp2mcu_int1,                           // Interrupt 1 pin status
    dsp2mcu_int2,                           // Interrupt 2 pin status
} status_flag_t;

typedef enum
{
    DSP_STATUS,
    DSP_CONFIG,
} artic_cmd_t;

typedef enum
{
    XMEM,
    YMEM,
    PMEM,
    IOMEM,
    INVALID_MEM = 0xFF
} mem_id_t;

typedef struct
{
    uint32_t XMEM_length;
    uint32_t XMEM_CRC;
    uint32_t YMEM_length;
    uint32_t YMEM_CRC;
    uint32_t PMEM_length;
    uint32_t PMEM_CRC;
} firmware_header_t;

typedef struct
{
    uint32_t total_len;
    uint32_t len_id;
    uint8_t data[31];
    uint8_t tail_bits;
} tx_message_t;

#endif // _ARTIC_H_