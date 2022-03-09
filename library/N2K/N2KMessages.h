#ifndef SELKIELoggerN2K_Messages
#define SELKIELoggerN2K_Messages

/*!
 * @file N2KMessages.h Message specific formats and decoders
 * @ingroup SELKIELoggerN2K
 */

#include "SELKIELoggerBase.h"
#include <stdbool.h>
#include <stdint.h>

#include "N2KTypes.h"

/*!
 * @addtogroup SELKIELoggerN2K
 * @{
 */

#define N2K_TO_DEGREES 0.0057295779513082332

uint8_t n2k_get_uint8(const n2k_act_message *n, size_t offset);
int8_t n2k_get_int8(const n2k_act_message *n, size_t offset);
int16_t n2k_get_int16(const n2k_act_message *n, size_t offset);
uint16_t n2k_get_uint16(const n2k_act_message *n, size_t offset);
int32_t n2k_get_int32(const n2k_act_message *n, size_t offset);
uint32_t n2k_get_uint32(const n2k_act_message *n, size_t offset);

bool n2k_127257_values(const n2k_act_message *n, uint8_t *seq, double *yaw, double *pitch, double *roll);
bool n2k_129025_values(const n2k_act_message *n, double *lat, double *lon);
bool n2k_129026_values(const n2k_act_message *n, uint8_t *seq, uint8_t *mag, double *course, double *speed);
bool n2k_130306_values(const n2k_act_message *n, uint8_t *seq, uint8_t *ref, double *speed, double *angle);

void n2k_127257_print(const n2k_act_message *n);
void n2k_129025_print(const n2k_act_message *n);
void n2k_129026_print(const n2k_act_message *n);
void n2k_130306_print(const n2k_act_message *n);
//! @}
#endif
