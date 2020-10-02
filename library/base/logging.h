#ifndef SELKIELogging
#define SELKIELogging
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

//! @file logging.h Output logging and file handling functions

/*!
 * \addtogroup logging Logging support functions
 *
 * @{
 */

//! Program state and logging information
typedef struct {
	bool started; //!< Indicates startup completed
	bool shutdown; //!< Indicates shutdown begun
	int verbose; //!< Current log verbosity (console output)
	FILE *log; //!< Log file
	int logverbose; //!< Current log verbosity (file output)
} program_state;

//! Output formatted error message
void log_error(const program_state *s, const char *format, ...) __attribute__ ((format (gnu_printf, 2, 3)));

//! Output formatted warning message
void log_warning(const program_state *s, const char *format, ...) __attribute__ ((format (gnu_printf, 2, 3)));

//! Output formatted information message at a given level
void log_info(const program_state *s, const int level, const char *format, ...) __attribute__ ((format (gnu_printf, 3, 4)));

/*! @brief Open dated, serial numbered file with given prefix and extension
 *
 * Generates a file name with the form [prefix]YYYYMMDDXX.[extension], where XX
 * is a two digit hexadecimal serial number. Starting from 0, the proposed file
 * name is opened in exclusive create mode ("w+x"). If this fails because the
 * file exists, the serial number is incremented and another attempt to create
 * the file is made until either a) an unused serial number is found or b) the
 * serial number reaches 0xFF.
 *
 * If no valid file name can be generated, or if any other error is
 * encountered, the function returns NULL.
 */
FILE *openSerialNumberedFile(const char *prefix, const char *extension);

//!@}
#endif
