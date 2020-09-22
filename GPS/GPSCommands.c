#include <stdint.h>

#include "GPSCommands.h"
#include "GPSSerial.h"
#include "GPSTypes.h"
#include "GPSMessages.h"

bool setBaudRate(int handle, uint32_t baud) {
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
	return writeMessage(handle, &setBaud);
}
