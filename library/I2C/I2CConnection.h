#ifndef SELKIELoggerI2C_Connection
#define SELKIELoggerI2C_Connection

#include <stdint.h>

/*!
 * @file I2CConnection.h General connection functions for I2C bus connected sensors
 * @ingroup SELKIELoggerI2C
 */

/*!
 * @addtogroup SELKIELoggerI2C
 * @{
 */

//! Device specific callback functions
typedef float (*i2c_dev_read_fn)(const int, const int);

//! Set up a connection to the specified bus
int i2c_openConnection(const char *bus);

//! Close existing connection
void i2c_closeConnection(int handle);

//! Swap word byte order
int16_t i2c_swapbytes(const int16_t in);
//! @}
#endif
