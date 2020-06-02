/* MS5837_05BA.c - Device driver for the MS5837_14BA pressure sensor
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

#include <math.h>
#include "MS5837_xBA.h"
#include "syshal_i2c.h"

// See datasheet for details
int32_t MS5837_get_pressure(void)
{
    // Retrieve ADC result
    int32_t D1 = MS5837_xBA_get_adc(CMD_ADC_D1, resolution_priv);
    int32_t D2 = MS5837_xBA_get_adc(CMD_ADC_D2, resolution_priv);

    // working variables
    int32_t TEMP, dT;
    int64_t OFF, SENS, T2, OFF2, SENS2;

    /* Calculate first order coefficients */

    // Difference between actual and reference temperature
    // dT = D2 - C5 * 2^8
    dT = D2 - ((int32_t)MS5837_xBA_coefficient[5] << 8);

    // Actual temperature
    // TEMP = 2000 + dT * C6 / 2^23
    TEMP = (((int64_t)dT * MS5837_xBA_coefficient[6]) >> 23) + 2000;

    // Offset at actual temperature
    // OFF = C2 * 2^16 + ( C4 * dT ) / 2^7
    OFF = ((int64_t)MS5837_xBA_coefficient[2] << 16) + (((MS5837_xBA_coefficient[4] * (int64_t)dT)) >> 7);

    // Sensitivity at actual temperature
    // SENS = C1 * 2^15 + ( C3 * dT ) / 2^8
    SENS = ((int64_t)MS5837_xBA_coefficient[1] << 15) + (((MS5837_xBA_coefficient[3] * (int64_t)dT)) >> 8);

    /* Calculate second order coefficients */

    if (TEMP / 100 < 20) // If temp is below 20.0C
    {
        // T2 = 3 * dT^2 / 2^33
        T2 = 3 * (((int64_t)dT * dT) >> 33);

        // OFF2 = 3 * (TEMP - 2000) ^ 2 / 2^1
        OFF2 = 3 * ((TEMP - 2000) * (TEMP - 2000)) >> 1;

        // SENS2 = 5 * (TEMP - 2000) ^ 2 / 2^3
        SENS2 = 5 * ((TEMP - 2000) * (TEMP - 2000)) >> 3;

        if (TEMP < -1500) // If temp is below -15.0C
        {
            // OFF2 = OFF2 + 7 * (TEMP + 1500)^2
            OFF2 = OFF2 + 7 * ((TEMP + 1500) * (TEMP + 1500));

            // SENS2 = SENS2 + 4 * (TEMP + 1500)^2
            SENS2 = SENS2 + 4 * ((TEMP + 1500) * (TEMP + 1500));
        }
    }
    else // If TEMP is above 20.0C
    {
        // T2 = 2 * dT^2 / 2^37
        T2 = 2 * (((int64_t)dT * dT) >> 37);

        // OFF2 = 1 * (TEMP - 2000) ^ 2 / 2^4
        OFF2 = 1 * ((TEMP - 2000) * (TEMP - 2000)) >> 4;

        // SENS2 = 0
        SENS2 = 0;
    }

    /* Merge first and second order coefficients */

    // TEMP = TEMP - T2
    TEMP = TEMP - T2;

    // OFF = OFF - OFF2
    OFF = OFF - OFF2;

    // SENS = SENS - SENS2
    SENS = SENS - SENS2;

    /* Calculate pressure */

    // P = (D1 * SENS / 2^21 - OFF) / 2^13
    int32_t pressure = ((((SENS * D1) >> 21 ) - OFF) >> 13) / 10;

    return pressure;
}