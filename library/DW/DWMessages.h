#ifndef SELKIELoggerDW_Messages
#define SELKIELoggerDW_Messages

/*!
 * @file DWMessages.h Utility functions for use with Datawell equipment messages
 * @ingroup SELKIELoggerDW
 */

#include <stdint.h>

#include "DWTypes.h"

/*!
 * @addtogroup SELKIELoggerDW
 * @{
 */

uint16_t dw_hxv_cycdat(const dw_hxv *in);
int16_t dw_hxv_vertical(const dw_hxv *in);
int16_t dw_hxv_north(const dw_hxv *in);
int16_t dw_hxv_west(const dw_hxv *in);
uint16_t dw_hxv_parity(const dw_hxv *in);

//! @}
#endif
