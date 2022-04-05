#ifndef SELKIELoggerN2K_Types
#define SELKIELoggerN2K_Types

/*!
 * @file N2KTypes.h Data types and definitions for communication with N2K devices
 *
 * Encoding and protocol information heavily based on the equivalent functions
 * in the OpenSkipper project, which can be found at
 * https://github.com/OpenSkipper/OpenSkipper
 *
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

/*
 * From CANReader
 * Actisense devices:
 *
 */

#define ACT_ESC 0x10 //!< Escape character
#define ACT_SOT 0x02 //!< Start of text
#define ACT_EOT 0x03 //!< End of text
#define ACT_N2K 0x93 //!< N2k Message
#define ACT_BEM 0xA0 //!< BEM CMD ??

/*!
 * Represent an N2K message received from an ACT gateway device
 *
 * The PGN value is transmitted as three separate bytes, but combined here as a 24 bit value (stored as an unsigned 32
 * bit integer).
 *
 * Transmitted messages are escaped such that any ACT_ESC bytes in the original data are doubled up as ACT_ESC ACT_ESC
 */
typedef struct {
	// Header not stored: ACT_ESC ACT_SOT ACT_N2K
	uint8_t length;     //!< Counted from priority to csum
	uint8_t priority;   //!< N2K Message priority value
	uint32_t PGN;       //!< 24 bit PGN identifier
	uint8_t dst;        //!< Message destination
	uint8_t src;        //!< Message source
	uint32_t timestamp; //!< Message timestamp
	uint8_t datalen;    //!< Length of *data
	uint8_t *data;      //!< Message payload
	/*!
	 * Message checksum:
	 * 0 if cs == 0 else 256 - cs, where
	 * cs=sum(ACT_N2K+priority+....+data)
	 */
	uint8_t csum;
	// Footer not stored (ACT_ESC ACT_EOT)
} n2k_act_message;

//! Convert N2K message to a series of bytes compatible with ACT gateway devices
bool n2k_act_to_bytes(const n2k_act_message *act, uint8_t **out, size_t *len);

//! Convert a series of recieved bytes from ACT gateway devices into a message representation
bool n2k_act_from_bytes(const uint8_t *in, const size_t len, n2k_act_message **msg, size_t *pos, bool debug);

//! Calculate checksum for n2k_act_message
uint8_t n2k_act_checksum(const n2k_act_message *msg);

//! Print representation of an n2k_act_message to standard output
void n2k_act_print(const n2k_act_message *msg);

//! @}
#endif
