#ifndef SL_LOGGER_I2C_H
#define SL_LOGGER_I2C_H

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"
#include "SELKIELoggerI2C.h"

//! Map device functions to message IDs
typedef struct {
	uint8_t messageID; //!< Message ID to report
	string message_name; //!< Message name to report
	uint8_t deviceAddr; //!< I2C Device address
	i2c_dev_read_fn func; //!< Pointer to device read function
} i2c_msg_map;

//! I2C Source device specific parameters
typedef struct {
	char *busName; //!< Target port name
	uint8_t sourceNum; //!< Source ID for messages
	int handle; //!< Handle for currently opened device
	int frequency; //!< Aim to sample this many times per second
	int en_count; //!< Number of messages in chanmap
	i2c_msg_map *chanmap; //!< Map of device functions to poll
} i2c_params;

void *i2c_setup(void *ptargs);
void *i2c_logging(void *ptargs);
void *i2c_shutdown(void *ptargs);
void *i2c_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks i2c_getCallbacks();

//! Fill out default GPS parameters
i2c_params i2c_getParams();

//! Check channel mapping is valid
bool i2c_validate_chanmap(i2c_params *ip);

//! Add INA219 voltage and current readings to channel map
bool i2c_chanmap_add_ina219(i2c_params *ip, const uint8_t devAddr, const uint8_t baseID);
#endif
