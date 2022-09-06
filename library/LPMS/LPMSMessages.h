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

#define LPMS_START                 0x3A
#define LPMS_END1                  0x0D
#define LPMS_END2                  0x0A

// Not a complete list, but contains all commands likely to be used
#define LPMS_MSG_REPLY_ACK         0x00
#define LPMS_MSG_REPLY_NAK         0x01
#define LPMS_MSG_SAVE_REG          0x04
#define LPMS_MSG_FACTORY_RESET     0x05
#define LPMS_MSG_MODE_CMD          0x06
#define LPMS_MSG_MODE_STREAM       0x07
#define LPMS_MSG_GET_SENSORSTATE   0x08
#define LPMS_MSG_GET_IMUDATA       0x09
#define LPMS_MSG_GET_SENSORMODEL   0x14
#define LPMS_MSG_GET_FIRMWAREVER   0x15
#define LPMS_MSG_GET_SERIALNUM     0x16
#define LPMS_MSG_GET_FILTERVER     0x17
#define LPMS_MSG_SET_OUTPUTS       0x1E
#define LPMS_MSG_GET_OUTPUTS       0x1F
#define LPMS_MSG_SET_ID            0x20
#define LPMS_MSG_GET_ID            0x21
#define LPMS_MSG_SET_FREQ          0x22
#define LPMS_MSG_GET_FREQ          0x23
#define LPMS_MSG_SET_RADIANS       0x24
#define LPMS_MSG_GET_RADIANS       0x25
#define LPMS_MSG_SET_ACCRANGE      0x32
#define LPMS_MSG_GET_ACCRANGE      0x33
#define LPMS_MSG_SET_GYRRANGE      0x3C
#define LPMS_MSG_GET_GYRRANGE      0x3D
#define LPMS_MSG_SET_MAGRANGE      0x46
#define LPMS_MSG_GET_MAGRANGE      0x47
#define LPMS_MSG_SET_FILTER        0x5A
#define LPMS_MSG_GET_FILTER        0x5B
#define LPMS_MSG_SET_UARTBAUD      0x82
#define LPMS_MSG_GET_UARTBAUD      0x83
#define LPMS_MSG_SET_UARTFORMAT    0x84
#define LPMS_MSG_GET_UARTFORMAT    0x85
#define LPMS_MSG_SET_UARTPRECISION 0x88
#define LPMS_MSG_GET_UARTPRECISION 0x89

//! Read bytes and populate message structure
bool lpms_from_bytes(const uint8_t *in, const size_t len, lpms_message *msg, size_t *pos);

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
//! @}
#endif
