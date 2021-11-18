#include "I2C-SN3218.h"

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>


/*!
 * Sends a reset command to connected SN3218 device on this I2C bus
 * @param busHandle Handle for I2C bus opened with i2c_openConnection()
 * @return true on successful write, false otherwise
 */
bool i2c_sn3218_reset(const int busHandle) {
	if (ioctl(busHandle, I2C_SLAVE, SN3218_ADDR_DEFAULT) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_RESET, 0xFF) < 0) {
		return false;
	}

	return true;
}

/*!
 * Sends configuration commands followed by an update command to a connected
 * SN3218 device.
 *
 * As we cannot read state from the chip, all LEDs must be configured on each request.
 *
 * @param busHandle Handle for I2C bus opened with i2c_openConnection()
 * @param state Pointer to i2c_sn3218_state object
 * @return true on successful write of all commands, false otherwise
 */
bool i2c_sn3218_update(const int busHandle, const i2c_sn3218_state *state) {
	if (ioctl(busHandle, I2C_SLAVE, SN3218_ADDR_DEFAULT) < 0) {
		return false;
	}

	if (!state->global_enable) {
		if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_ENABLE, 0x00) < 0) {
			return false;
		}
		return true;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_ENABLE, 0xFF) < 0) {
		return false;
	}

	uint8_t ledcontrol1 = 0;
	uint8_t ledcontrol2 = 0;
	uint8_t ledcontrol3 = 0;

	for (int i = 0; i < 6; ++i) {
		if (state->led[0 + i] > 0) {
			ledcontrol1 |= 1 << i;
		}
	}

	for (int i = 0; i < 6; ++i) {
		if (state->led[6 + i] > 0) {
			ledcontrol2 |= 1 << i;
		}
	}

	for (int i = 0; i < 6; ++i) {
		if (state->led[12 + i] > 0) {
			ledcontrol3 |= 1 << i;
		}
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_LED_01, ledcontrol1) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_LED_02, ledcontrol2) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_LED_03, ledcontrol3) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_01, state->led[0]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_02, state->led[1]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_03, state->led[2]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_04, state->led[3]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_05, state->led[4]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_06, state->led[5]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_07, state->led[6]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_08, state->led[7]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_09, state->led[8]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_10, state->led[9]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_11, state->led[10]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_12, state->led[11]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_13, state->led[12]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_14, state->led[13]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_15, state->led[14]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_16, state->led[15]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_17, state->led[16]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_PWM_18, state->led[17]) < 0) {
		return false;
	}

	if (i2c_smbus_write_byte_data(busHandle, SN3218_REG_UPDATE, 0xFF) < 0) {
		return false;
	}

	return true;
}

