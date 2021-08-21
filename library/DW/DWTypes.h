#ifndef SELKIELoggerDW_Types
#define SELKIELoggerDW_Types

/*!
 * @file DWTypes.h Data types and definitions for use with Datawell equipment
 * @ingroup SELKIELoggerDW
 */

#include <stdint.h>
#include <stdbool.h>

/*!
 * @addtogroup SELKIELoggerDW
 * @{
 */

/*!
 * @addtogroup DWChanStatus Channel Status
 *
 * Channel Status indicators
 *
 * @{
 */

//! Channel OK
#define DW_CHAN_OK '-'

//! Channel Repaired
#define DW_CHAN_REPAIRED '='

//! Channel Unrecoverable
#define DW_CHAN_BAD '!'

//! @}


/*!
 * @brief Internal representation of a Datawell HVA message
 *
 * Transmitted as text, terminated by a carriage return (not stored). The
 * sequence number and data arrays are transmitted as hexadecimal characters,
 * with two characters representing each byte stored.
 */
typedef struct dw_hva {
	uint8_t seq; //!< Sequence number, transmitted as two hex characters
	char rtStatus; //!< Real time channel status information. @sa DWChanStatus
	uint8_t rt[9]; //!< Real time data
	char packStatus; //!< Packet data status information. @sa DWChanStatus
	uint8_t pack[3]; //!< Packet data
} dw_hva;

/*!
 * @brief Internal representation of a Datawell BVA message
 *
 * The data is identical to that stored in a dw_hva structure, but without any
 * status or sequencing information.
 */
typedef struct dw_bva {
	uint8_t rt[9]; //!< Real time data
	uint8_t pack[3]; //!< Packet data
} dw_bva;

/*!
 * @brief Internal representation of a Datawell HVX message
 *
 * Transmitted as text, terminated by a carriage return (not stored).
 *
 * All data transmitted as hexadecimal characters, with two characters
 * representing each byte stored.
 */
typedef struct dw_hxv {
	uint8_t status; //!< Error count. 0 or 1 OK, 2+ error
	uint8_t lines; //!< Transmitted line number
	uint8_t data[8]; //!< 8 bytes of data
} dw_hxv;

bool hexpair_to_uint(const char *in, uint8_t *out);
//! @}
#endif
