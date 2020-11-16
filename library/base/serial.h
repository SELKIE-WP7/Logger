#ifndef SELKIELoggerBase_Serial
#define SELKIELoggerBase_Serial

/*!
 * @file serial.h Generic serial connection and utility functions
 * @ingroup SELKIELoggerBase
 */

/*!
 * @addtogroup SELKIELoggerBase
 * @{
 */
//! Open a serial connection at a given baud rate
int openSerialConnection(const char *port, const int baudRate);

//! Convert a numerical baud rate to termios flag
int baud_to_flag(const int rate);

//! Convert a termios baud rate flag to a numerical value
int flag_to_baud(const int flag);
//! @}
#endif
