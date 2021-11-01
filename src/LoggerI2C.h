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

/*!
 * @addtogroup loggerI2C Logger: I2C Support
 * @ingroup logger
 *
 * Adds support for reading messages from an I2C bus directly connected to the logging machine.
 *
 * General support functions are detailed in SELKIELoggerI2C.h. As the I2C bus
 * must be polled rather than streaming data to the logger, each sensor and
 * value that needs to be logged must be registered into the channel map in
 * i2c_params before i2c_logging is called.
 *
 * @{
 */

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
	char *sourceName; //!< Reported source name
	int handle; //!< Handle for currently opened device
	int frequency; //!< Aim to sample this many times per second
	int en_count; //!< Number of messages in chanmap
	i2c_msg_map *chanmap; //!< Map of device functions to poll
} i2c_params;

//! I2C Connection setup
void *i2c_setup(void *ptargs);

//! I2C main logging loop
void *i2c_logging(void *ptargs);

//! I2C shutdown
void *i2c_shutdown(void *ptargs);

//! Generate I2C channel map
void *i2c_channels(void *ptargs);

//! Fill out device callback functions for logging
device_callbacks i2c_getCallbacks();

//! Fill out default I2C parameters
i2c_params i2c_getParams();

//! Check channel mapping is valid
bool i2c_validate_chanmap(i2c_params *ip);

//! Add INA219 voltage and current readings to channel map
bool i2c_chanmap_add_ina219(i2c_params *ip, const uint8_t devAddr, const uint8_t baseID);

//! Add ADS1015 single ended voltage measurements to channel map
bool i2c_chanmap_add_ads1015(i2c_params *ip, const uint8_t devAddr, const uint8_t baseID);

//! Take a configuration section, parse parameters and register devices
bool i2c_parseConfig(log_thread_args_t *lta, config_section *s);
//!@}
#endif
