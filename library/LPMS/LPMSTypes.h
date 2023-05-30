/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SELKIELoggerLPMS_Types
#define SELKIELoggerLPMS_Types

/*!
 * @file LPMSTypes.h Data types and definitions for communication with LPMS devices
 *
 * @ingroup SELKIELoggerLPMS
 */

#include <stdbool.h>
#include <stdint.h>

#include "SELKIELoggerBase.h"

//! @{

/*!
 * @brief Represent LPMS message
 *
 * Each word is represented as two bytes in little-endian format when
 * transmited.  `id`, `command`, and `length` are transmitted at the start of
 * the message, followed by `length` bytes of data and then `command` - which
 * is the bytewise sum of message contents.
 *
 * When transmitted, an initial LPMS_START byte precedes the message and
 * LPMS_END1 and LPMS_END2 are appended.
 */
typedef struct {
	uint16_t id;       //!< Source/Destination Sensor ID
	uint16_t command;  //!< Message type
	uint16_t length;   //!< Length of data, in bytes
	uint16_t checksum; //!< Sum of all preceding message bytes
	uint8_t *data;     //!< Pointer to data array
} lpms_message;

// While it is unlikely that this wouldn't be true on any supported system, the
// entire library is based on this assumption holding.
_Static_assert(sizeof(float) == 4, "LPMS support requires 32bit floats");

/*!
 * @brief LPMS IMU data packet
 *
 * The exact combination of data transmitted by the sensors is configurable at
 * run time, so this represents all *possible* values that could be retrieved.
 * It almost certainly will not correspond to the on-wire format of the data
 * received.
 *
 * The `present` member will contain a bitmask indicating which members are
 * set, as reported by the sensor.  This is not transmitted with every packet,
 * so must be cached and set explicitly.
 */
typedef struct {
	uint32_t timestamp;    //!< Counted in 0.002s increments
	float accel_raw[3];    //!< Raw accelerometer values [g]
	float accel_cal[3];    //!< Calibrated accelerometer values [g]
	float gyro_raw[3];     //!< Raw gyroscope values [X/s]
	float gyro_cal[3];     //!< Calibrated gyroscope values [X/s]
	float gyro_aligned[3]; //!< Calibrated and aligned gyroscope values [X/s]
	float mag_raw[3];      //!< Raw magnetometer values [uT]
	float mag_cal[3];      //!< Calibrated magnetometer values [uT]
	float omega[3];        //!< Angular velocity [X/s]
	float quaternion[4];   //!< Orientation in quaternion form
	float euler_angles[3]; //!< Orientation as Euler roll angles [X]
	float accel_linear[3]; //!< Linear acceleration (only) [g]
	float pressure;        //!< Atmospheric pressure [kPa]
	float altitude;        //!< Altitude [m]
	float temperature;     //!< Temperature [Celsius]
	uint32_t present;      //!< Bitmask indicating set/valid members
} lpms_data;

/*!
 * @defgroup LPMS_IMU LPMS IMU Data test bits
 * @ingroup SELKIELoggerLPMS
 *
 * Use these flags to test for the presence of data in either inbound data
 * (test the reply to a GET_OUTPUTS/SET_OUTPUTS packet) or in lpms_data.present
 *
 */
//! @{
#define LPMS_IMU_ACCEL_RAW    0  //!< accel_raw[] will contain data
#define LPMS_IMU_ACCEL_CAL    1  //!< accel_cal[] will contain data
#define LPMS_IMU_GYRO_RAW     3  //!< gyro_raw[] will contain data
#define LPMS_IMU_GYRO_CAL     5  //!< gyro_cal[] will contain data
#define LPMS_IMU_GYRO_ALIGN   7  //!< gyro_align[] will contain data
#define LPMS_IMU_MAG_RAW      8  //!< mag_raw[] will contain data
#define LPMS_IMU_MAG_CAL      9  //!< mag_cal[] will contain data
#define LPMS_IMU_OMEGA        10 //!< omega[] will contain data
#define LPMS_IMU_QUATERNION   11 //!< quaternion[] will contain data
#define LPMS_IMU_EULER        12 //!< euler[] will contain data
#define LPMS_IMU_ACCEL_LINEAR 13 //!< accel_linear[] will contain data
#define LPMS_IMU_PRESSURE     14 //!< pressure will contain data
#define LPMS_IMU_ALTITUDE     15 //!< altitude will contain data
#define LPMS_IMU_TEMPERATURE  16 //!< temperature will contain data

#define LPMS_HAS(x, y)        (((x) & (1 << y)) == (1 << y)) //!< Check if masked bit is/bits are set
//! @}

//! @}
#endif
