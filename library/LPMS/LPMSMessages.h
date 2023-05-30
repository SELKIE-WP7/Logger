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

#ifndef SELKIELoggerLPMS_Messages
#define SELKIELoggerLPMS_Messages

/*!
 * @file LPMSMessages.h Data types and definitions for communication with LPMS devices
 *
 * @ingroup SELKIELoggerLPMS
 */

#include <stdbool.h>
#include <stdint.h>

#include "SELKIELoggerBase.h"

#include "LPMSTypes.h"

#define LPMS_START                 0x3A //!< Message prefix byte
#define LPMS_END1                  0x0D //!< First message suffix byte
#define LPMS_END2                  0x0A //!< Second message suffix byte

/*!
 * @defgroup LPMS_MSG LPMS Message types
 * @ingroup SELKIELoggerLPMS
 *
 * This isn't a complete list, but should contain all commands likely to be used.
 * Replies to GET commands use the same message type, and SET commands should
 * generate LPMS_MSG_REPLY_ACK or LPMS_MSG_REPLY_NAK.
 *
 * @{
 */
#define LPMS_MSG_REPLY_ACK         0x00 //!< Command successful (Doesn't appear everywhere it should)
#define LPMS_MSG_REPLY_NAK         0x01 //!< Command unsuccessful
#define LPMS_MSG_SAVE_REG          0x04 //!< Write data to specific internal registers
#define LPMS_MSG_FACTORY_RESET     0x05 //!< Reset to default configuration
#define LPMS_MSG_MODE_CMD          0x06 //!< Switch to command mode
#define LPMS_MSG_MODE_STREAM       0x07 //!< Switch to continuous readout mode
#define LPMS_MSG_GET_SENSORSTATE   0x08 //!< Get sensor state
#define LPMS_MSG_GET_IMUDATA       0x09 //!< IMU data, as configured by LPMS_MSG_SET_OUTPUTS
#define LPMS_MSG_GET_SENSORMODEL   0x14 //!< Get hardware model as 24 character string
#define LPMS_MSG_GET_FIRMWAREVER   0x15 //!< Get firmware version as 24 character string
#define LPMS_MSG_GET_SERIALNUM     0x16 //!< Get serial number as 24 character string
#define LPMS_MSG_GET_FILTERVER     0x17 //!< Get filter version as 24 character string
#define LPMS_MSG_SET_OUTPUTS       0x1E //!< Configure fields included in IMUDATA messages
#define LPMS_MSG_GET_OUTPUTS       0x1F //!< Get fields configured for IMUDATA messages
#define LPMS_MSG_SET_ID            0x20 //!< Reassign sensor ID
#define LPMS_MSG_GET_ID            0x21 //!< Get current sensor ID
#define LPMS_MSG_SET_FREQ          0x22 //!< Set streaming output rate (Hz)
#define LPMS_MSG_GET_FREQ          0x23 //!< Get current streaming output rate
#define LPMS_MSG_SET_RADIANS       0x24 //!< Use radians for angular quantities (1) instead of degrees (0)
#define LPMS_MSG_GET_RADIANS       0x25 //!< Get unit for angular quantities - radians (1) or degrees (0)
#define LPMS_MSG_SET_ACCRANGE      0x32 //!< Set accelerometer range (g)
#define LPMS_MSG_GET_ACCRANGE      0x33 //!< Get accelerometer range (g)
#define LPMS_MSG_SET_GYRRANGE      0x3C //!< Set gyroscope range (dps)
#define LPMS_MSG_GET_GYRRANGE      0x3D //!< Get gyroscope range (dps)
#define LPMS_MSG_SET_MAGRANGE      0x46 //!< Set magnetometer range (gauss)
#define LPMS_MSG_GET_MAGRANGE      0x47 //!< Get magnetometer range (gauss)
#define LPMS_MSG_SET_FILTER        0x5A //!< Set motion filtering mode
#define LPMS_MSG_GET_FILTER        0x5B //!< Get motion filtering mode
#define LPMS_MSG_SET_UARTBAUD      0x82 //!< Set baud rate
#define LPMS_MSG_GET_UARTBAUD      0x83 //!< Get baud rate
#define LPMS_MSG_SET_UARTFORMAT    0x84 //!< Set output data format - LPMS (0), ASCII (1)
#define LPMS_MSG_GET_UARTFORMAT    0x85 //!< Get output data format - LPMS (0), ASCII (1)
#define LPMS_MSG_SET_UARTPRECISION 0x88 //!< Set output data precision - Fixed point (0) or floats (1)
#define LPMS_MSG_GET_UARTPRECISION 0x89 //!< Get output data precision - Fixed point (0) or floats (1)

//! @}

//! Read bytes and populate message structure
bool lpms_from_bytes(const uint8_t *in, const size_t len, lpms_message *msg, size_t *pos);

//! Convert message structure to flat array
bool lpms_to_bytes(const lpms_message *msg, uint8_t **out, size_t *len);

//! Calculate checksum for LPMS message packet
bool lpms_checksum(const lpms_message *msg, uint16_t *csum);

//! Extract timestamp from lpms_message into lpms_data, if available
bool lpms_imu_set_timestamp(const lpms_message *msg, lpms_data *d);
//! Extract accel_raw from lpms_message into lpms_data, if available
bool lpms_imu_set_accel_raw(const lpms_message *msg, lpms_data *d);
//! Extract accel_cal from lpms_message into lpms_data, if available
bool lpms_imu_set_accel_cal(const lpms_message *msg, lpms_data *d);
//! Extract gyro_raw from lpms_message into lpms_data, if available
bool lpms_imu_set_gyro_raw(const lpms_message *msg, lpms_data *d);
//! Extract gyro_cal from lpms_message into lpms_data, if available
bool lpms_imu_set_gyro_cal(const lpms_message *msg, lpms_data *d);
//! Extract gyro_aligned from lpms_message into lpms_data, if available
bool lpms_imu_set_gyro_aligned(const lpms_message *msg, lpms_data *d);
//! Extract mag_raw from lpms_message into lpms_data, if available
bool lpms_imu_set_mag_raw(const lpms_message *msg, lpms_data *d);
//! Extract mag_cal from lpms_message into lpms_data, if available
bool lpms_imu_set_mag_cal(const lpms_message *msg, lpms_data *d);
//! Extract omega from lpms_message into lpms_data, if available
bool lpms_imu_set_omega(const lpms_message *msg, lpms_data *d);
//! Extract quaternion from lpms_message into lpms_data, if available
bool lpms_imu_set_quaternion(const lpms_message *msg, lpms_data *d);
//! Extract euler_angles from lpms_message into lpms_data, if available
bool lpms_imu_set_euler_angles(const lpms_message *msg, lpms_data *d);
//! Extract accel_linear from lpms_message into lpms_data, if available
bool lpms_imu_set_accel_linear(const lpms_message *msg, lpms_data *d);
//! Extract pressure from lpms_message into lpms_data, if available
bool lpms_imu_set_pressure(const lpms_message *msg, lpms_data *d);
//! Extract altitude from lpms_message into lpms_data, if available
bool lpms_imu_set_altitude(const lpms_message *msg, lpms_data *d);
//! Extract temperature from lpms_message into lpms_data, if available
bool lpms_imu_set_temperature(const lpms_message *msg, lpms_data *d);
#endif
