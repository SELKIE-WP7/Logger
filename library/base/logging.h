/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SELKIELoggerBase_Logging
#define SELKIELoggerBase_Logging
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

/*!
 * @file logging.h Output logging and file handling functions
 * @ingroup SELKIELoggerBase
 */

/*!
 * @addtogroup logging Logging support functions
 * @ingroup SELKIELoggerBase
 *
 * @{
 */

//! Program state and logging information
typedef struct {
	bool started;   //!< Indicates startup completed
	bool shutdown;  //!< Indicates shutdown begun
	int verbose;    //!< Current log verbosity (console output)
	FILE *log;      //!< Log file
	int logverbose; //!< Current log verbosity (file output)
} program_state;

//! Output formatted error message
void log_error(const program_state *s, const char *format, ...) __attribute__((format(__printf__, 2, 3)));

//! Output formatted warning message
void log_warning(const program_state *s, const char *format, ...) __attribute__((format(__printf__, 2, 3)));

//! Output formatted information message at a given level
void log_info(const program_state *s, const int level, const char *format, ...)
	__attribute__((format(__printf__, 3, 4)));

//! Open dated, serial numbered file with given prefix and extension
FILE *openSerialNumberedFile(const char *prefix, const char *extension, char **name);

//! Cleanly destroy program state
void destroy_program_state(program_state *s);
//!@}
#endif
