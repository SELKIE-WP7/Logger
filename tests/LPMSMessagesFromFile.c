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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"

#include "SELKIELoggerLPMS.h"

#include "version.h"
/*!
 * @file LPMSMessagesFromFile.c
 *
 * @brief Test reading LPMS data from a file
 *
 * @test Read supplied LPMS data and output summary information.
 *
 * @ingroup testing
 */

//! Allocated read buffer size
#define BUFSIZE 1024

/*!
 * Read data from file until end of file is reached, outputting total number of
 * entries read.
 *
 * Exact outputs dependent on supplied test data
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns 0 (Pass), -1 (Fail), -2 (Failed to run / Error)
 */
int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	//LCOV_EXCL_START
	// Should be 1 spare arguments: The file to convert
	if (argc - optind != 1) {
		log_error(&state, "Invalid arguments");
		fprintf(stderr, "%s <datafile>\n", argv[0]);
		return -1;
	}

	FILE *nf = fopen(argv[optind], "r");

	if (nf == NULL) {
		log_error(&state, "Unable to open input file \"%s\"", argv[optind]);
		return -1;
	}
	//LCOV_EXCL_STOP

	state.started = true;
	bool processing = true;

	uint8_t buf[BUFSIZE] = {0};
	size_t hw = 0;
	int count = 0;
	size_t end = 0;
	while (processing || hw > 10) {
		//LCOV_EXCL_START
		if (feof(nf) && processing) {
			log_info(&state, 2, "End of file reached");
			processing = false;
		}
		//LCOV_EXCL_STOP
		lpms_message *m = calloc(1, sizeof(lpms_message));
		if (!m) {
			//LCOV_EXCL_START
			perror("lpms_message calloc");
			return EXIT_FAILURE;
			//LCOV_EXCL_STOP
		}
		bool r = lpms_readMessage_buf(fileno(nf), m, buf, &end, &hw);
		if (r) {
			uint16_t cs = 0;
			bool csOK = (lpms_checksum(m, &cs) && cs == m->checksum);
			if (m->command == LPMS_MSG_GET_IMUDATA) {
				lpms_data d = {0};
				// Guess based on observed data until detection implemented
				d.present |= (1 << LPMS_IMU_ACCEL_RAW);
				d.present |= (1 << LPMS_IMU_ACCEL_CAL);
				d.present |= (1 << LPMS_IMU_MAG_RAW);
				d.present |= (1 << LPMS_IMU_MAG_CAL);
				d.present |= (1 << LPMS_IMU_GYRO_RAW);
				d.present |= (1 << LPMS_IMU_GYRO_CAL);
				d.present |= (1 << LPMS_IMU_GYRO_ALIGN);
				d.present |= (1 << LPMS_IMU_OMEGA);
				d.present |= (1 << LPMS_IMU_EULER);
				d.present |= (1 << LPMS_IMU_ALTITUDE);
				d.present |= (1 << LPMS_IMU_TEMPERATURE);
				if (lpms_imu_set_timestamp(m, &d)) {
					log_info(&state, 1, "%02x: Timestamp: %u", m->id,
					         d.timestamp);
				} else {
					//LCOV_EXCL_START
					log_warning(&state, "%02x: Timestamp invalid", m->id);
					return -1;
					//LCOV_EXCL_STOP
				}
				if (lpms_imu_set_accel_raw(m, &d)) {
					log_info(&state, 1,
					         "%02x: Raw Acceleration: (%+.4f, %+.4f, %+4.f)",
					         m->id, d.accel_raw[0], d.accel_raw[1],
					         d.accel_raw[2]);
				}
				if (lpms_imu_set_accel_cal(m, &d)) {
					log_info(
						&state, 1,
						"%02x: Calibrated Acceleration: (%+.4f, %+.4f, %+4.f)",
						m->id, d.accel_cal[0], d.accel_cal[1],
						d.accel_cal[2]);
				}
				if (lpms_imu_set_gyro_raw(m, &d)) {
					log_info(&state, 1,
					         "%02x: Raw Gyro: (%+.4f, %+.4f, %+4.f)", m->id,
					         d.gyro_raw[0], d.gyro_raw[1], d.gyro_raw[2]);
				}
				if (lpms_imu_set_gyro_cal(m, &d)) {
					log_info(&state, 1,
					         "%02x: Calibrated Gyro: (%+.4f, %+.4f, %+4.f)",
					         m->id, d.gyro_cal[0], d.gyro_cal[1],
					         d.gyro_cal[2]);
				}
				if (lpms_imu_set_gyro_aligned(m, &d)) {
					log_info(&state, 1,
					         "%02x: Aligned Gyro: (%+.4f, %+.4f, %+4.f)",
					         m->id, d.gyro_aligned[0], d.gyro_aligned[1],
					         d.gyro_aligned[2]);
				}
				if (lpms_imu_set_mag_raw(m, &d)) {
					log_info(&state, 1,
					         "%02x: Raw Magnetic Field: (%+.4f, %+.4f, %+4.f)",
					         m->id, d.mag_raw[0], d.mag_raw[1], d.mag_raw[2]);
				}
				if (lpms_imu_set_mag_cal(m, &d)) {
					log_info(
						&state, 1,
						"%02x: Calibrated Magnetic Field: (%+.4f, %+.4f, %+4.f)",
						m->id, d.mag_cal[0], d.mag_cal[1], d.mag_cal[2]);
				}
				if (lpms_imu_set_euler_angles(m, &d)) {
					log_info(&state, 1,
					         "%02x: Euler roll angles: (%+.4f, %+.4f, %+.4f)",
					         m->id, d.euler_angles[0], d.euler_angles[1],
					         d.euler_angles[2]);
				}
				if (lpms_imu_set_altitude(m, &d)) {
					log_info(&state, 1, "%02x: Altitude: %.2f", m->id,
					         d.altitude);
				}
				if (lpms_imu_set_temperature(m, &d)) {
					log_info(&state, 1, "%02x: Temperature: %.2f", m->id,
					         d.temperature);
				}
			} else {
				//LCOV_EXCL_START
				log_info(&state, 2, "%02x: Command %02x, %u bytes, checksum %s",
				         m->id, m->command, m->length, csOK ? "OK" : "not OK");
				//LCOV_EXCL_STOP
			}
			count++;
		} else {
			if (m->id == 0xAA || m->id == 0xEE) {
				//LCOV_EXCL_START
				log_error(&state,
				          "Error reading messages from file (Code: 0x%02x)\n",
				          (uint8_t)m->id);
				//LCOV_EXCL_STOP
			}

			if (m->id == 0xFD) {
				processing = false;
				if (end < hw) { end++; }
			}
		}

		if (m) {
			if (m->data) { free(m->data); }
			free(m);
			m = NULL;
		}
	}
	log_info(&state, 0, "%d messages successfully read from file", count);
	fclose(nf);
	return 0;
}
