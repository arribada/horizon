/* syshal_sat.h - Abstraction layer for satallite comms
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
 *;

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _SYSHAL_SAT_H_
#define _SYSHAL_SAT_H_

#include <stdint.h>
#include "prepas.h"
#include "iot.h"

#define SYSHAL_SAT_NO_ERROR                 (  0)
#define SYSHAL_SAT_ERROR_BOOT               ( -1)
#define SYSHAL_SAT_ERROR_SPI_TRANSFER       ( -2)
#define SYSHAL_SAT_ERROR_INCORRECT_MEM_SEL  ( -3)
#define SYSHAL_SAT_ERROR_INT_TIMEOUT        ( -4)
#define SYSHAL_SAT_ERROR_CRC                ( -5)
#define SYSHAL_SAT_ERROR_FILE               ( -6)
#define SYSHAL_SAT_ERROR_GPIO               ( -7)
#define SYSHAL_SAT_ERROR_TIME               ( -8)
#define SYSHAL_SAT_ERROR_INTERRUPT          ( -9)
#define SYSHAL_SAT_ERROR_SPI                (-10)
#define SYSHAL_SAT_ERROR_DEBUG              (-11)
#define SYSHAL_SAT_ERROR_BUFFER_OVERFLOW    (-12)
#define SYSHAL_SAT_ERROR_STATUS             (-13)
#define SYSHAL_SAT_ERROR_TX                 (-14)
#define SYSHAL_SAT_ERROR_NOT_INIT           (-15)
#define SYSHAL_SAT_ERROR_NOT_PROGRAMMED     (-16)
#define SYSHAL_SAT_ERROR_NOT_POWERED_ON     (-17)
#define SYSHAL_SAT_ERROR_MAX_SIZE           (-18)
#define SYSHAL_SAT_ERROR_INVALID_PARAM      (-19)

#define MAX_LENGHT_READ         (256)

#define SYSHAL_SAT_MAX_NUM_SATS 	(8)
#define SYSHAL_SAT_MAX_RETRIES  	(3)
#define SYSHAL_SAT_MAX_PACKET_SIZE  (31)


typedef struct
{
    sys_config_iot_sat_artic_settings_t *artic;
} syshal_sat_config_t;

int syshal_sat_init(syshal_sat_config_t sat_config);
int syshal_sat_power_on(void);
int syshal_sat_power_off(void);
int syshal_sat_program_firmware(fs_t fs, uint32_t local_file_id);
int syshal_sat_send_message(const uint8_t *buffer, size_t buffer_size);
int syshal_sat_calc_prepass(const iot_last_gps_location_t *gps, iot_prepass_result_t *result);
int syshal_sat_get_status_register(uint32_t *status);
#ifdef GTEST
void reset_state(void);
void force_programmed(void);
#endif
#endif /* _SYSHAL_SAT_H_ */