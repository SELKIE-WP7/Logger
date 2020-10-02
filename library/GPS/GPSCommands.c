#include <stdint.h>

#include "GPSCommands.h"
#include "GPSSerial.h"
#include "GPSTypes.h"
#include "GPSMessages.h"

bool ubx_setBaudRate(const int handle, const uint32_t baud) {
	ubx_message setBaud = {
		0xB5, 0x62, // Header bytes
		0x06, 0x00, // CFG-PRT
		0x0014, // 20 byte message
		{
			0x01, // Configure port 1 (UART)
			0x00, // Reserved
			0x00, 0x00, // No ready pin
			0xd0, 0x08, 0x00, 0x00, // UART Mode
			0x00, 0x00, 0x00, 0x00, // Baud (set below)
			0x07, 0x00, // All protocols allowed as input
			0x01, 0x00, // UBX protocol only output
			0x00, 0x00, // Flags
			0x00, 0x00  // Reserved
		},
		0xFF, 0xFF}; // Checksum (TBC)

	setBaud.data[8]  = (uint8_t) (baud & 0xFF);
	setBaud.data[9]  = (uint8_t) ((baud >> 8) & 0xFF);
	setBaud.data[10] = (uint8_t) ((baud >> 16) & 0xFF);
	setBaud.data[11] = (uint8_t) ((baud >> 24) & 0xFF);

	ubx_set_checksum(&setBaud);
	return ubx_writeMessage(handle, &setBaud);
}

bool ubx_setMessageRate(const int handle, const uint8_t msgClass, const uint8_t msgID, const uint8_t rate) {
	ubx_message setRate = {
		0xB5, 0x62, 0x06, 0x01, // Header, CFG-MSG
		0x0008, // 8 byte message
		{
			msgClass, msgID,
			0x00, // Disable on I2C
			rate, // Every "rate" updates on UART1
			0x00, // Disable UART 2
			0x00, // Disable on USB direct
			0x00, // Disable on SPI
			0x00  // Disable on port 5
		},
		0xFF, 0xFF};
	ubx_set_checksum(&setRate);
	return ubx_writeMessage(handle, &setRate);
}

/*!
 * Some UBX message types can be polled by sending a message with the message
 * class and ID but zero length.
 *
 * Not valid for all types, check U-Blox manual for information
 */
bool ubx_pollMessage(const int handle, const uint8_t msgClass, const uint8_t msgID) {
	ubx_message poll = {0xB5, 0x62, msgClass, msgID, 0x0000, {0x00}, 0xFF, 0xFF};
	ubx_set_checksum(&poll);
	return ubx_writeMessage(handle, &poll);
}

/*!
 * Not making this configurable for now, as the "proper" method would need a bit more faff
 */
bool ubx_enableGalileo(const int handle) {
	ubx_message enableGalileo = {
		0xB5, 0x62, 0x06, 0x3e,
		0x003C,
		{0x00, 0x20, 0x20, 0x07, 0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01, 0x03, 0x08, 0x10, 0x00, 0x00, 0x00, 0x01, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x03, 0x05, 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0x05, 0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01},
		0xFF, 0xFF};
	ubx_set_checksum(&enableGalileo);
	return ubx_writeMessage(handle, &enableGalileo);
}

/*!
 * Configures the GPS navigation calculation and reporting rate.
 *
 * - Interval is measured in milliseconds and determines the rate at which navigation results are calculated.
 * - Output rate determines how many measurements are made before an update message is sent.
 *
 * Can be overridden by power saving settings
 */
bool ubx_setNavigationRate(const int handle, const uint16_t interval, const uint16_t outputRate) {
	ubx_message navRate = {
		0xB5, 0x62, 0x06, 0x08, // Header, CFG-RATE
		0x0006,
		{
			(interval & 0xFF), ((interval >> 8) & 0xFF),
			(outputRate & 0xFF), ((outputRate >> 8) & 0xFF),
			0x00, 0x00 // Align measurements to UTC
		},
		0xFF, 0xFF}; // Dummy checksum values
	ubx_set_checksum(&navRate);
	return ubx_writeMessage(handle, &navRate);
}

bool ubx_enableLogMessages(const int handle) {
	ubx_message enableInf = {
		0xB5, 0x62, 0x06, 0x02, // Header, CFG-INF
		0x000A, // 10 bytes
		{
			0x00, //UBX
			0x00,0x00,0x00, //Reserved
			0x00, 0x07, 0x00, 0x00, 0x00, 0x00, // Error, warning and info on UART 1, disable all others
		},
		0xFF, 0xFF}; // Checksum to be set below
	ubx_set_checksum(&enableInf);
	return ubx_writeMessage(handle, &enableInf);
}

bool ubx_disableLogMessages(const int handle) {
	ubx_message disableInf = {
		0xB5, 0x62, 0x06, 0x02, // Header, CFG-INF
		0x000A, // 10 bytes
		{
			0x00, //UBX
			0x00,0x00,0x00, //Reserved
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Disable all
		},
		0xFF, 0xFF}; // Checksum to be set below
	ubx_set_checksum(&disableInf);
	return ubx_writeMessage(handle, &disableInf);
}
