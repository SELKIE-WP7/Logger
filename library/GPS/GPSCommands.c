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

#include <stdint.h>

#include "GPSCommands.h"
#include "GPSMessages.h"
#include "GPSSerial.h"
#include "GPSTypes.h"

/***
 * Some common features of all messages:
 * - Header:
 *   - 2 byte header
 *   - 2 byte message ID (category + type)
 *   - 2 byte / 1 word length
 * - Footer:
 *   - 2 byte checksum
 * An artefact of the internal representation of the messages is an additional
 * trailing zero in the values used here.  This ensures that the pointer to any
 * extra data is set to zero
 */
/*!
 * Sends a UBX protocol CFG-PRT message, configuring UART 1 for the specified
 * baud rate with all protocols permitted as input and only UBX messages
 * permitted as output.
 *
 * No configuration for UBX message types is performed here, so the GPS may
 * just sit silently until we configure the messages we want as output
 * (depending on default configuration).
 *
 * @param[in] handle File descriptor to write command to
 * @param[in] baud Desired baud rate - will be converted with baud_to_flag()
 * @return Status of ubx_writeMessage()
 */
bool ubx_setBaudRate(const int handle, const uint32_t baud) {
	ubx_message setBaud = {0xB5,
	                       0x62, // Header bytes
	                       0x06,
	                       0x00,   // CFG-PRT
	                       0x0014, // 20 byte message
	                       {
				       0x01,                   // Configure port 1 (UART)
				       0x00,                   // Reserved
				       0x00, 0x00,             // No ready pin
				       0xd0, 0x08, 0x00, 0x00, // UART Mode
				       0x00, 0x00, 0x00, 0x00, // Baud (set below)
				       0x07, 0x00,             // All protocols allowed as input
				       0x01, 0x00,             // UBX protocol only output
				       0x00, 0x00,             // Flags
				       0x00, 0x00              // Reserved
			       },
	                       0xFF,
	                       0xFF,  // Checksum value calculated later
	                       0x00}; // No extra data pointer

	setBaud.data[8] = (uint8_t)(baud & 0xFF);
	setBaud.data[9] = (uint8_t)((baud >> 8) & 0xFF);
	setBaud.data[10] = (uint8_t)((baud >> 16) & 0xFF);
	setBaud.data[11] = (uint8_t)((baud >> 24) & 0xFF);

	ubx_set_checksum(&setBaud);
	return ubx_writeMessage(handle, &setBaud);
}

/*!
 * Sends a UBX protcol CFG-MSG message with the provided message class, type and rate.
 *
 * The message is output very "rate" updates/calculations on UART1 and disabled
 * on all other outputs.
 *
 * @param[in] handle File descriptor to write command to
 * @param[in] msgClass UBX Message Class
 * @param[in] msgID UBX Message ID/Type
 * @param[in] rate Requested message rate (0 to disable)
 * @return Status of ubx_writeMessage()
 */
bool ubx_setMessageRate(const int handle, const uint8_t msgClass, const uint8_t msgID, const uint8_t rate) {
	ubx_message setRate = {0xB5,
	                       0x62,
	                       0x06,
	                       0x01,   // Header, CFG-MSG
	                       0x0008, // 8 byte message
	                       {
				       msgClass, msgID,
				       0x00, // Disable on I2C
				       rate, // Every "rate" updates on UART1
				       0x00, // Disable UART 2
				       rate, // Also enable on USB direct
				       0x00, // Disable on SPI
				       0x00  // Disable on port 5
			       },
	                       0xFF,
	                       0xFF,
	                       0x00};
	ubx_set_checksum(&setRate);
	return ubx_writeMessage(handle, &setRate);
}

/*!
 * Some UBX message types can be polled by sending a message with the message
 * class and ID but zero length.
 *
 * Not valid for all types, check U-Blox manual for information
 *
 * @param[in] handle File descriptor to write command to
 * @param[in] msgClass UBX Message Class
 * @param[in] msgID UBX Message ID/Type
 * @return Status of ubx_writeMessage()
 */
bool ubx_pollMessage(const int handle, const uint8_t msgClass, const uint8_t msgID) {
	ubx_message poll = {0xB5, 0x62, msgClass, msgID, 0x0000, {0x00}, 0xFF, 0xFF, 0x00};
	ubx_set_checksum(&poll);
	return ubx_writeMessage(handle, &poll);
}

/*!
 * Not making this configurable for now, as the "proper" method would need a bit more faff
 *
 * @param[in] handle File descriptor to write command to
 * @return Status of ubx_writeMessage()
 */
bool ubx_enableGalileo(const int handle) {
	ubx_message enableGalileo = {0xB5,
	                             0x62, // Header
	                             0x06, // CFG
	                             0x3e, // ?
	                             0x003C,
	                             {0x00, 0x20, 0x20, 0x07, 0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
	                              0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x00,
	                              0x01, 0x00, 0x01, 0x01, 0x03, 0x08, 0x10, 0x00, 0x00, 0x00, 0x01, 0x01,
	                              0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x03, 0x05, 0x00, 0x03, 0x00,
	                              0x01, 0x00, 0x01, 0x05, 0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01},
	                             0xFF,
	                             0xFF,
	                             0x00};
	ubx_set_checksum(&enableGalileo);
	return ubx_writeMessage(handle, &enableGalileo);
}

/*!
 * Configures the GPS navigation calculation and reporting rate.
 *
 * - Interval is measured in milliseconds and sets navigation results calculation rate
 * - Output rate sets how many measurements are made before an update message is sent.
 *
 * Can be overridden by power saving settings
 *
 * @param[in] handle File descriptor to write command to
 * @param[in] interval Calculation interval in milliseconds
 * @param[in] outputRate Output solution every 'outputRate' calculations
 * @return Status of ubx_writeMessage()
 */
bool ubx_setNavigationRate(const int handle, const uint16_t interval, const uint16_t outputRate) {
	ubx_message navRate = {0xB5,
	                       0x62, // Header
	                       0x06, // CFG
	                       0x08, // RATE
	                       0x0006,
	                       {
				       (interval & 0xFF), ((interval >> 8) & 0xFF), (outputRate & 0xFF),
				       ((outputRate >> 8) & 0xFF), 0x00,
				       0x00 // Align measurements to UTC
			       },
	                       0xFF,
	                       0xFF,
	                       0x00};
	ubx_set_checksum(&navRate);
	return ubx_writeMessage(handle, &navRate);
}

/*!
 * Allows us to log warnings and information from the GPS module, largely
 * during the startup and configuration process.
 *
 * Enables error, warning and information messages on UART1 only and disables
 * message output on all other ports.
 *
 * @param[in] handle File descriptor to write command to
 * @return Status of ubx_writeMessage()
 */
bool ubx_enableLogMessages(const int handle) {
	ubx_message enableInf = {0xB5,
	                         0x62,   // Header
	                         0x06,   // CFG
	                         0x02,   // INF
	                         0x000A, // 10 bytes
	                         {
					 0x00,             // UBX
					 0x00, 0x00, 0x00, // Reserved
					 0x00, 0x07, 0x00, 0x00, 0x00,
					 0x00, // Error, warning and info on UART 1, disable all others
				 },
	                         0xFF,
	                         0xFF,
	                         0x00};
	ubx_set_checksum(&enableInf);
	return ubx_writeMessage(handle, &enableInf);
}

/*!
 * Disables options set by ubx_enableLogMessages()
 *
 * @param[in] handle File descriptor to write command to
 * @return Status of ubx_writeMessage()
 */
bool ubx_disableLogMessages(const int handle) {
	ubx_message disableInf = {0xB5,
	                          0x62,
	                          0x06,
	                          0x02,   // Header, CFG-INF
	                          0x000A, // 10 bytes
	                          {
					  0x00,             // UBX
					  0x00, 0x00, 0x00, // Reserved
					  0x00, 0x00, 0x00, 0x00, 0x00,
					  0x00, // Disable all
				  },
	                          0xFF,
	                          0xFF,
	                          0x00};
	ubx_set_checksum(&disableInf);
	return ubx_writeMessage(handle, &disableInf);
}

/*!
 * Set I2C address for this GPS module
 *
 * @param[in] handle File descriptor to write command to
 * @param[in] addr New I2C address
 * @return Status of ubx_writeMessage()
 */
bool ubx_setI2CAddress(const int handle, const uint8_t addr) {
	ubx_message setI2C = {0xB5,
	                      0x62,
	                      0x06,
	                      0x00,   // Header, CFG-INF
	                      0x0014, // 20 bytes
	                      {
				      0x00,                          // I2C/DDC
				      0x00,                          // Reserved
				      0x00, 0x00,                    // TXReady (disabled)
				      (addr << 1), 0x00, 0x00, 0x00, // Mode
				      0x00, 0x00,                    // Reserved
				      0x01,                          // UBX Only input
				      0x01,                          // UBX Only output
				      0x00, 0x00, 0x00, 0x00,        // Flags, reserved
			      },
	                      0xFF,
	                      0xFF,
	                      0x00};
	ubx_set_checksum(&setI2C);
	return ubx_writeMessage(handle, &setI2C);
}
