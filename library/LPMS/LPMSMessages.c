#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "LPMSMessages.h"
#include "LPMSTypes.h"

#include <assert.h>

/*!
 *
 * Populate lpms_message from array of bytes, searching for valid start byte if
 * required.
 *
 * @param[in] in Array of bytes
 * @param[in] len Number of bytes available in array
 * @param[out] msg Pointer to lpms_message
 * @param[out] pos Number of bytes consumed
 * @returns True on success, false on error
 */
bool lpms_from_bytes(const uint8_t *in, const size_t len, lpms_message *msg, size_t *pos) {
	if (in == NULL || msg == NULL || len < 10 || pos == NULL) { return NULL; }

	ssize_t start = -1;
	while (((*pos) + 10) < len) {
		if ((in[(*pos)] == LPMS_START)) {
			start = (*pos);
			break;
		}
		(*pos)++;
	}

	if (start < 0) {
		// Nothing found, so bail early
		// (*pos) marks the end of the searched data
		return false;
	}

	msg->id = in[start + 1] + ((uint16_t)in[start + 2] << 8);
	msg->command = in[start + 3] + ((uint16_t)in[start + 4] << 8);
	msg->length = in[start + 5] + ((uint16_t)in[start + 6] << 8);

	if (msg->length == 0) {
		msg->data = NULL;
	} else {
		// Check there's enough bytes in buffer to cover message header
		// (6 bytes), footer (4 bytes), and embedded data
		if ((len - start - 10) < msg->length) {
			msg->id = 0xFF;
			return false;
		}
		msg->data = calloc(msg->length, sizeof(uint8_t));
		if (msg->data == NULL) {
			msg->id = 0xAA;
			return false;
		}

		(*pos) += 7; // Move (*pos) up now we know we can read data in
		for (int i = 0; i < msg->length; i++) {
			msg->data[i] = in[(*pos)++];
		}
	}
	msg->checksum = in[(*pos)] + ((uint16_t)in[(*pos) + 1] << 8);
	(*pos) += 2;
	if ((in[(*pos)] != LPMS_END1) || (in[(*pos) + 1] != LPMS_END2)) {
		msg->id = 0xEE;
		return false;
	}
	(*pos) += 2;
	return true;
}

/*!
 *
 * Convert lpms_message structure to array of bytes. The array will be
 * allocated here and must be freed by the caller.
 *
 * @param[in] msg Pointer to lpms_message
 * @param[out] out Pointer to array of bytes
 * @param[out] len Pointer to size_t - will be set to array length
 * @returns True on success, false on error
 */
bool lpms_to_bytes(const lpms_message *msg, uint8_t **out, size_t *len) {
	if ((msg == NULL) || (out == NULL)) { return false; }
	(*len) = msg->length + 11;
	(*out) = calloc((*len), sizeof(uint8_t));
	if ((*out) == NULL) {
		perror("lpms_to_bytes");
		return false;
	}
	(*out)[0] = LPMS_START;
	(*out)[1] = (msg->id & 0x00FF);
	(*out)[2] = (msg->id & 0xFF00) >> 8;
	(*out)[3] = (msg->command & 0x00FF);
	(*out)[4] = (msg->command & 0xFF00) >> 8;
	(*out)[5] = (msg->length & 0x00FF);
	(*out)[6] = (msg->length & 0xFF00) >> 8;
	size_t p = 7;
	for (int j = 0; j < msg->length; j++) {
		(*out)[p++] = msg->data[j];
	}
	(*out)[p++] = (msg->checksum & 0x00FF);
	(*out)[p++] = (msg->checksum & 0xFF00) >> 8;
	(*out)[p++] = LPMS_END1;
	(*out)[p++] = LPMS_END2;
	return (p == (*len));
}

/*!
 * @param[in] msg Pointer to message structure
 * @param[out] csum Calculated checksum value
 * @returns True if calculated successfully, false on error
 */
bool lpms_checksum(const lpms_message *msg, uint16_t *csum) {
	if (msg == NULL || csum == NULL) { return false; }
	uint16_t cs = 0;
	cs += (msg->id & 0x00FF);
	cs += (msg->id & 0xFF00) >> 8;
	cs += (msg->command & 0x00FF);
	cs += (msg->command & 0xFF00) >> 8;
	cs += (msg->length & 0x00FF);
	cs += (msg->length & 0xFF00) >> 8;
	if (msg->length > 0) {
		if (msg->data == NULL) { return false; }
		for (int i = 0; i < msg->length; ++i) {
			cs += msg->data[i];
		}
	}
	(*csum) = cs;
	return true;
}

/*!
 * Extract timestamp from input message into data struct.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_timestamp(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (msg->length < 4) { return false; }

	d->timestamp = msg->data[0] + ((uint32_t)msg->data[1] << 8) + ((uint32_t)msg->data[2] << 16) +
	               ((uint32_t)msg->data[3] << 24);
	return true;
}

/*!
 * Extract raw accelerometer data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_accel_raw(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { return false; }
	size_t ix = 4;
	memcpy(d->accel_raw, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract calibrated accelerometer data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_accel_cal(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { return false; }

	// Skip timestamp (data[0..3]), then skip any other data present
	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (msg->length < (ix + 12)) { return false; }
	memcpy(d->accel_cal, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract raw gyroscope data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_gyro_raw(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { return false; }

	// Skip timestamp (data[0..3]), then skip any other data present
	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (msg->length < (ix + 12)) { return false; }
	memcpy(d->gyro_raw, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract calibrated gyroscope data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_gyro_cal(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { return false; }

	// Skip timestamp (data[0..3]), then skip any other data present
	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (msg->length < (ix + 12)) { return false; }

	memcpy(d->gyro_cal, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract aligned gyroscope data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_gyro_aligned(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (msg->length < (ix + 12)) { return false; }

	memcpy(d->gyro_aligned, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract raw magnetometer data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_mag_raw(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_MAG_RAW)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { ix += 12; }
	if (msg->length < (ix + 12)) { return false; }

	memcpy(d->mag_raw, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract calibrated magnetometer data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_mag_cal(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_MAG_CAL)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_RAW)) { ix += 12; }
	if (msg->length < (ix + 12)) { return false; }

	memcpy(d->mag_cal, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract angular momentum from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_omega(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_OMEGA)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_CAL)) { ix += 12; }
	if (msg->length < (ix + 12)) { return false; }

	memcpy(d->omega, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract orientation quaternion from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_quaternion(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_QUATERNION)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_OMEGA)) { ix += 12; }
	if (msg->length < (ix + 16)) { return false; }

	memcpy(d->quaternion, &(msg->data[ix]), 16);
	return true;
}

/*!
 * Extract Euler orientation angles from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_euler_angles(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_EULER)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_OMEGA)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_QUATERNION)) { ix += 16; }
	if (msg->length < (ix + 12)) { return false; }

	memcpy(d->euler_angles, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract linear acceleration data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_accel_linear(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_ACCEL_LINEAR)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_OMEGA)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_QUATERNION)) { ix += 16; }
	if (LPMS_HAS(d->present, LPMS_IMU_EULER)) { ix += 12; }
	if (msg->length < (ix + 12)) { return false; }

	memcpy(d->accel_linear, &(msg->data[ix]), 12);
	return true;
}

/*!
 * Extract atmospheric pressure data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_pressure(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_PRESSURE)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_OMEGA)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_QUATERNION)) { ix += 16; }
	if (LPMS_HAS(d->present, LPMS_IMU_EULER)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_LINEAR)) { ix += 12; }
	if (msg->length < (ix + 4)) { return false; }

	memcpy(&(d->pressure), &(msg->data[ix]), 4);
	return true;
}

/*!
 * Extract altitude data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_altitude(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_ALTITUDE)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_OMEGA)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_QUATERNION)) { ix += 16; }
	if (LPMS_HAS(d->present, LPMS_IMU_EULER)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_LINEAR)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_PRESSURE)) { ix += 4; }
	if (msg->length < (ix + 4)) { return false; }

	memcpy(&(d->altitude), &(msg->data[ix]), 4);
	return true;
}

/*!
 * Extract temperature data from input message into data struct.
 * Checks if data is expected to be present before attempting to extract.
 *
 * @param[in] msg Pointer to message structure containing IMU data
 * @param[in,out] d Pointer to lpms_data structure to populate
 * @returns True if data extracted from message, false on error
 */
bool lpms_imu_set_temperature(const lpms_message *msg, lpms_data *d) {
	if (!msg || !d || msg->command != LPMS_MSG_GET_IMUDATA) { return false; }
	if (!LPMS_HAS(d->present, LPMS_IMU_TEMPERATURE)) { return false; }

	size_t ix = 4;
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_GYRO_ALIGN)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_RAW)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_MAG_CAL)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_OMEGA)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_QUATERNION)) { ix += 16; }
	if (LPMS_HAS(d->present, LPMS_IMU_EULER)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_ACCEL_LINEAR)) { ix += 12; }
	if (LPMS_HAS(d->present, LPMS_IMU_PRESSURE)) { ix += 4; }
	if (LPMS_HAS(d->present, LPMS_IMU_ALTITUDE)) { ix += 4; }
	if (msg->length < (ix + 4)) { return false; }

	memcpy(&(d->temperature), &(msg->data[ix]), 4);
	return true;
}
