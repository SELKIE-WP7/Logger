#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

/*!
 * Sources:
 * https://www.kernel.org/doc/html/latest/i2c/smbus-protocol.html
 * https://www.kernel.org/doc/html/latest/i2c/dev-interface.html
 * https://www.waveshare.com/w/upload/1/10/Ina219.pdf
 */

int main(int argc, char *argv[]) {
	int handle;

	handle = open("/dev/i2c-1", O_RDWR);
	if (handle < 0) {
		fprintf(stderr, "Unable to open I2C bus: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	for (int i2cc=0x40; i2cc < 0x44; i2cc++) {
		if (ioctl(handle, I2C_SLAVE, i2cc) < 0) {
			fprintf(stderr, "Unable to set slave address: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}

		fprintf(stdout, "Input %02x\n", i2cc-0x40+1);

		/*!
		 * INA219 registers (from datasheet):
		 * - 0x00 Configuration
		 * - 0x01 Shunt Voltage
		 * - 0x02 Bus Voltage
		 * - 0x03 Power
		 * - 0x04 Current
		 * - 0x05 Calibration
		 *
		 * All registers are 16bits.
		 *
		 * Calibration value (0x05) needs to be set before power and current can be read
		 * 4us delay minimum required between write and subsequent reads
		 */
		//! Note that the bytes are in the wrong order for smbus_read/write, so need to swap them

		i2c_smbus_write_word_data(handle, 0x00, 0xDFBD); // 0xBDDF: Reset, 32V range, 320mV step, 12 bit resolution, 8 sample averages, continuous sampling
		usleep(500);
		i2c_smbus_write_word_data(handle, 0x05, 0x0010); // Current_LSB 1uA
		usleep(10);

		{

			int32_t res = i2c_smbus_read_word_data(handle, 0x01);
			if (res < 0) {
			  fprintf(stderr, "Unable to read from register 0x01: %s\n", strerror(errno));
			  return EXIT_FAILURE;
			}

			res = (res >> 8) + ((res & 0xFF) << 8);
			float shuntV = 0;
			if ((res & 0x8000)) {
				uint16_t t = (res & 0x7FFF) + 1;
				shuntV = ~t * 1E-2;
			} else {
				shuntV = (res & 0x7FFF) * 1E-2;
			}

			if (shuntV > 320.0 || shuntV < -320.0) {
				fprintf(stdout, "Shunt Voltage out of range - input disconnected?\n");
				continue;
			}
			fprintf(stdout, "\tShunt Voltage: %+.3fmV\n", shuntV);

			res = i2c_smbus_read_word_data(handle, 0x02);
			if (res < 0) {
			  fprintf(stderr, "Unable to read from register 0x02: %s\n", strerror(errno));
			  return EXIT_FAILURE;
			}
			res = (res >> 8) + ((res & 0xFF) << 8);
			float busV = (res >> 3) * 4E-3;
			//uint8_t flags = (res & 0x03);
			//bool overflow = (flags & 0x01);
			//bool valid = (flags & 0x02);
			fprintf(stdout, "\tBus Voltage: %+.6fV\n", busV);

			res = i2c_smbus_read_word_data(handle, 0x04);
			if (res < 0) {
			  fprintf(stderr, "Unable to read from register 0x04: %s\n", strerror(errno));
			  return EXIT_FAILURE;
			}
			res = (res >> 8) + ((res & 0xFF) << 8);
			float current = 0;
			if ((res & 0x8000)) {
				uint16_t t = ~(res & 0x7FFF) - 1;
				current = t * 1E-3;
			} else {
				current = (res & 0x7FFF) * 1E-3;
			}
			fprintf(stdout, "\tCurrent: %+.6fA\n", current);

			res = i2c_smbus_read_word_data(handle, 0x03);
			if (res < 0) {
			  fprintf(stderr, "Unable to read from register 0x03: %s\n", strerror(errno));
			  return EXIT_FAILURE;
			}
			res = (res >> 8) + ((res & 0xFF) << 8);
			float power = ((uint16_t) res) * 2E-3;
			fprintf(stdout, "\tPower: %+.6fW\n", power);

		}
	}
	close(handle);
	return 0;
}
