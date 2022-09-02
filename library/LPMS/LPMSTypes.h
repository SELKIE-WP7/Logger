#ifndef SELKIELoggerLPMS_Types
#define SELKIELoggerLPMS_Types

/*!
 * @file LPMSTypes.h Data types and definitions for communication with LPMS devices
 *
 * @ingroup SELKIELoggerLPMS
 */

#include "SELKIELoggerBase.h"
#include <stdbool.h>
#include <stdint.h>

#define LPMS_START 0x3A
#define LPMS_END1  0x0D
#define LPMS_END2  0x0A

typedef struct {
	uint16_t id;
	uint16_t command;
	uint16_t length;
	uint16_t checksum;
	uint8_t *data;
} lpms_message;

bool lpms_from_bytes(const uint8_t *in, const size_t len, lpms_message *msg, size_t *pos);

//! Calculate checksum for LPMS message packet
bool lpms_checksum(const lpms_message *msg, uint16_t *csum);
//! @}
#endif
