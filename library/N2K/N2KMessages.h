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

uint8_t n2k_get_uint8(const n2k_act_message *n, size_t offset);
int8_t n2k_get_int8(const n2k_act_message *n, size_t offset);
int16_t n2k_get_int16(const n2k_act_message *n, size_t offset);
uint16_t n2k_get_uint16(const n2k_act_message *n, size_t offset);
int32_t n2k_get_int32(const n2k_act_message *n, size_t offset);
uint32_t n2k_get_uint32(const n2k_act_message *n, size_t offset);

void n2k_127257_print(const n2k_act_message *n);
void n2k_129025_print(const n2k_act_message *n);
void n2k_129026_print(const n2k_act_message *n);
void n2k_130306_print(const n2k_act_message *n);
//! @}
#endif
