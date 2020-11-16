#ifndef SELKIELoggerNMEA_Messages
#define SELKIELoggerNMEA_Messages

/*!
 * @file NMEAMessages.h Utility functions for processing NMEA messages
 * @ingroup SELKIELoggerNMEA
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "NMEATypes.h"

/*!
 * @addtogroup nmea NMEA Message support functions
 * @ingroup SELKIELoggerNMEA
 *
 * Convert and verify NMEA protocol messages
 * @{
 */

//! Calculate checksum for NMEA message
void nmea_calc_checksum(const nmea_msg_t *msg, uint8_t *cs);

//! Set checksum bytes for NMEA message
void nmea_set_checksum(nmea_msg_t *msg);

//! Verify checksum bytes of NMEA message
bool nmea_check_checksum(const nmea_msg_t *msg);

//! Calculate number of bytes required to represent message
size_t nmea_message_length(const nmea_msg_t *msg);

//! Convert NMEA message to array of bytes for transmission
size_t nmea_flat_array(const nmea_msg_t *msg, char **out);

//! Return NMEA message as string
char * nmea_string_hex(const nmea_msg_t *msg);

//! Print NMEA message
void nmea_print_hex(const nmea_msg_t *msg);

//! Parse raw data into fields
strarray * nmea_parse_fields(const nmea_msg_t *nmsg);

//! Get date/time from NMEA ZDA message
struct tm * nmea_parse_zda(const nmea_msg_t *msg);
//! @}
#endif
