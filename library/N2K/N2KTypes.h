#ifndef SELKIELoggerN2K_Types
#define SELKIELoggerN2K_Types

/*!
 * @file N2KTypes.h Data types and definitions for communication with N2K devices
 * @ingroup SELKIELoggerN2K
 */

#include "SELKIELoggerBase.h"
#include <stdbool.h>
#include <stdint.h>

/*!
 * @addtogroup SELKIELoggerN2K
 * @{
 */

/*
 * Logic from CANHandler.cs

 * N2kFrame build process
 *
 * Extract Header
 * If fast packet (multipart message):
 * 	Check if part of existing sequence in pending list
 * 		If end of sequence, assemble data and return
 * 		Else, add to pending list and return nothing
 * 	Else if valid start of fast packet sequence, add to pending list
 * 	Else, discard and return nothing
 * Else
 * 	Return Frame
 *
 */
// typedef struct {
//  Header
//
//} n2kmessage;

/*
 * From CANReader
 * Actisense devices:
 *
 */

#define ACT_ESC 0x10 // Escape character
#define ACT_SOT 0x02 // Start of text
#define ACT_EOT 0x03 // End of text
#define ACT_N2K 0x93 // N2k Message
#define ACT_BEM 0xA0 // BEM CMD ??

/*
 * N2K Frame received from ACT device starts _ESC, _SOT, _N2K
 * uint8_t msglen = Number of bytes from priority to csum
 * uint8_t priority
 * uint8_t PGNa
 * uint8_t PGNb
 * uint8_t PGNc
 *
 * Where PGN = PGNa + (PGNb << 8) + (PGNc << 16)
 *
 * uint8_t dest
 * uint8_t source
 *
 * uint32_t timestamp (LSB first?)
 * uint8_t datalen (Up to but not including CRCByte)
 * uint8_t *data (If any byte in the raw message is ACT_ESC, will become ACT_ESC, ACT_ESC
 * uint8_t csum = 0 if cs == 0 else 256 - cs where cs=sum(_N2K+priority+....+data)
 * uint8_t ACT_ESC
 * uint8_t ACT_EOT
 */
typedef struct {
	// Header not stored
	uint8_t length;
	uint8_t priority;
	uint32_t PGN; // 24 bits
	uint8_t dst;
	uint8_t src;
	uint32_t timestamp;
	uint8_t datalen;
	uint8_t *data;
	uint8_t csum;
	// Footer not stored
} n2k_act_message;

bool n2k_act_to_bytes(const n2k_act_message *act, uint8_t **out, size_t *len);
bool n2k_act_from_bytes(const uint8_t *in, const size_t len, n2k_act_message **msg, size_t *pos, bool debug);
uint8_t n2k_act_checksum(const n2k_act_message *msg);

//! @}
#endif
