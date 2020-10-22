#ifndef SELKIELoggerGPS_Messages
#define SELKIELoggerGPS_Messages

//! @file GPSMessages.h Utility functions for processing UBX messages

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "GPSTypes.h"

/*!
 * \addtogroup ubx UBX Message support functions
 *
 * Convert and verify UBX protocol messages
 * @{
 */

//! Calculate checksum for UBX message
void ubx_calc_checksum(const ubx_message *msg, uint8_t *csA, uint8_t *csB);

//! Set checksum bytes for UBX message
void ubx_set_checksum(ubx_message *msg);

//! Verify checksum bytes of UBX message
bool ubx_check_checksum(const ubx_message *msg);

//! Convert UBX message to flat array of bytes
size_t ubx_flat_array(const ubx_message *msg, uint8_t **out);

//! Return UBX message as string of hexadecimal pairs
char * ubx_string_hex(const ubx_message *msg);

//! Print UBX message in hexadecimal form
void ubx_print_hex(const ubx_message *msg);

//! Decode UBX NAV-PVT message
bool ubx_decode_nav_pvt(const ubx_message *msg, ubx_nav_pvt *out);

//! @}
#endif
