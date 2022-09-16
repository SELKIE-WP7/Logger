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
 * @file
 * @brief Read and print N2K messages
 * @ingroup Executables
 */

//! Allocated read buffer size
#define BUFSIZE 1024

/*!
 * Read messages from file and print details to screen.
 *
 * Detail printed depends on whether structure of message is known, although
 * brief data can be printed for all messages at higher verbosities.
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns -1 on error, otherwise 0
 */
int main(int argc, char *argv[]) {
	program_state state = {0};
	state.verbose = 1;

	char *usage = "Usage: %1$s [-v] [-q] port\n"
		      "\t-v\tIncrease verbosity\n"
		      "\t-q\tDecrease verbosity\n"
		      "\nVersion: " GIT_VERSION_STRING "\n";

	opterr = 0; // Handle errors ourselves
	int go = 0;
	bool doUsage = false;
	while ((go = getopt(argc, argv, "vq")) != -1) {
		switch (go) {
			case 'v':
				state.verbose++;
				break;
			case 'q':
				state.verbose--;
				break;
			case '?':
				log_error(&state, "Unknown option `-%c'", optopt);
				doUsage = true;
		}
	}

	// Should be 1 spare arguments: The file to convert
	if (argc - optind != 1) {
		log_error(&state, "Invalid arguments");
		doUsage = true;
	}

	if (doUsage) {
		fprintf(stderr, usage, argv[0]);
		return -1;
	}

	int hdl = lpms_openConnection(argv[optind], 921600);

	state.started = true;
	bool processing = true;

	uint8_t buf[BUFSIZE] = {0};
	size_t hw = 0;
	int count = 0;
	size_t end = 0;
	lpms_send_command_mode(hdl);
	usleep(10000);
	lpms_message getModel = {.id = 0x01, .command = LPMS_MSG_GET_SENSORMODEL};
	lpms_send_command(hdl, &getModel);
	lpms_message getFW = {.id = 0x01, .command = LPMS_MSG_GET_FIRMWAREVER};
	lpms_send_command(hdl, &getFW);
	lpms_message getSerial = {.id = 0x01, .command = LPMS_MSG_GET_SERIALNUM};
	lpms_send_command(hdl, &getSerial);
	lpms_message getTransmitted = {.id = 0x01, .command = LPMS_MSG_GET_OUTPUTS};
	lpms_send_command(hdl, &getTransmitted);
	lpms_send_stream_mode(hdl);
	lpms_data d = {0};
	while (processing || hw > 10) {
		lpms_message *m = calloc(1, sizeof(lpms_message));
		if (!m) {
			perror("lpms_message calloc");
			return EXIT_FAILURE;
		}
		bool r = lpms_readMessage_buf(hdl, m, buf, &end, &hw);
		if (r) {
			uint16_t cs = 0;
			bool csOK = (lpms_checksum(m, &cs) && cs == m->checksum);
			if (m->command == LPMS_MSG_GET_OUTPUTS) {
				d.present = (uint32_t)m->data[0] + ((uint32_t)m->data[1] << 8) +
				            ((uint32_t)m->data[2] << 16) +
				            ((uint32_t)m->data[3] << 24);
				log_info(&state, 2, "%02x: Output configuration received", m->id);
			} else if (m->command == LPMS_MSG_GET_SENSORMODEL) {
				log_info(&state, 2, "%02x: Sensor model: %-24s", m->id,
				         (char *)m->data);
			} else if (m->command == LPMS_MSG_GET_SERIALNUM) {
				log_info(&state, 2, "%02x: Serial number: %-24s", m->id,
				         (char *)m->data);
			} else if (m->command == LPMS_MSG_GET_FIRMWAREVER) {
				log_info(&state, 2, "%02x: Firmware version: %-24s", m->id,
				         (char *)m->data);
			} else if (m->command == LPMS_MSG_GET_IMUDATA) {
				if (lpms_imu_set_timestamp(m, &d)) {
					log_info(&state, 1, "%02x: Timestamp: %u", m->id,
					         d.timestamp);
				} else {
					log_warning(&state, "%02x: Timestamp invalid", m->id);
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
				if (lpms_imu_set_pressure(m, &d)) {
					log_info(&state, 1, "%02x: Pressure: %.2f", m->id,
					         d.pressure);
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
				log_info(&state, 1, "%02x: Command %02x, %u bytes, checksum %s",
				         m->id, m->command, m->length, csOK ? "OK" : "not OK");
			}
			count++;
		} else {
			if (m->command > 0 || m->id > 0 || m->length > 0) {
				log_info(&state, 1, "%02x: [Error] - Command %02x, %u bytes",
				         m->id, m->command, m->length);
			}
			if (m->id == 0xAA || m->id == 0xEE) {
				log_error(&state,
				          "Error reading messages from file (Code: 0x%02x)\n",
				          (uint8_t)m->id);
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
	return 0;
}
