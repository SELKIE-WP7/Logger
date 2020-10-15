#include "I2CConnection.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


/*!
 * Opens the specified bus in read/write mode
 *
 * @param[in] bus Path to I2C bus device
 * @return Handle to opened bus, or -1 on failure
 */
int i2c_openConnection(const char *bus) {
	errno = 0;
	int handle = open(bus, O_RDWR);
	if (handle < 0) {
		return -1;
	}
	return handle;
}

/*!
 * Close connection previously opened with i2c_openConnection()
 *
 * @param[in] handle File handle returned by i2c_openConnection()
 */
void i2c_closeConnection(int handle) {
	close(handle);
}

/*!
 * Swap the bytes read or written using the SMBus commands
 *
 * Not sure if the need to do this is a misunderstanding or a documentation issue.
 *
 * @param[in] in Word to be swapped
 * @return Input, with byte order reversed
 */
int16_t i2c_swapbytes(const int16_t in) {
	return ((int16_t) ((in >> 8) + ((in & 0xFF) << 8)));
}
