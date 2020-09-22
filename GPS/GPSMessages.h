#ifndef SELKIELoggerGPSMessages
#define SELKIELoggerGPSMessages

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "GPSTypes.h"

void ubx_calc_checksum(const ubx_message *msg, uint8_t *csA, uint8_t *csB);
void ubx_set_checksum(ubx_message *msg);
bool ubx_check_checksum(const ubx_message *msg);

size_t ubx_flat_array(const ubx_message *msg, uint8_t **out);

char * ubx_string_hex(const ubx_message *msg);
void ubx_print_hex(const ubx_message *msg);
#endif
