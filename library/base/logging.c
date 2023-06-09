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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "logging.h"

/*!
 * \addtogroup logging  Logging support functions
 * @{
 */

/*!
 * Outputs a provided error message, prepending the current program state and
 * the marker "Error: ".
 *
 * The error message and any additional arguments are passed to vfprintf in
 * order to support formatted output.
 *
 * Always outputs to stderr in addition to any configured log file.
 *
 * Cannot be silenced by runtime configuration.
 *
 * @param[in] s program_state, used for state and configuration details
 * @param[in] format printf compatible format string
 */
void log_error(const program_state *s, const char *format, ...) {
	va_list args;
	char *label;
	if (s->shutdown) {
		label = "[Shutdown]";
	} else if (!s->started) {
		label = "[Startup]";
	} else {
		label = "[Running]";
	}
	fprintf(stderr, "%-10s Error: ", label);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");

	if (s->log) {
		fprintf(s->log, "%s Error: ", label);
		va_start(args, format);
		vfprintf(s->log, format, args);
		va_end(args);
		fprintf(s->log, "\n");
	}
}

/*!
 * Outputs a provided warning message, prepending the current program state and
 * the marker "Warning: ".
 *
 * The error message and any additional arguments are passed to vfprintf in
 * order to support formatted output.
 *
 * Always outputs to stderr in addition to any configured log file.
 *
 * Cannot be silenced by runtime configuration.
 *
 * @param[in] s program_state, used for state and configuration details
 * @param[in] format printf compatible format string
 */
void log_warning(const program_state *s, const char *format, ...) {
	va_list args;
	char *label;
	if (s->shutdown) {
		label = "[Shutdown]";
	} else if (!s->started) {
		label = "[Startup]";
	} else {
		label = "[Running]";
	}
	fprintf(stderr, "%-10s Warning: ", label);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");

	if (s->log) {
		fprintf(s->log, "%s Error: ", label);
		va_start(args, format);
		vfprintf(s->log, format, args);
		va_end(args);
		fprintf(s->log, "\n");
	}
}

/*!
 * Outputs a provided message, prepending the current program state and
 * the marker "Info:%d: ", with the level value substituted.
 *
 * The error message and any additional arguments are passed to vfprintf in
 * order to support formatted output.
 *
 * The level is compared to the current verbosity to determine if messages are
 * output to stdout, file, or both.
 *
 * @param[in] s program_state, used for state and configuration details
 * @param[in] level Detail/Verbosity level for this message
 * @param[in] format printf compatible format string
 */
void log_info(const program_state *s, const int level, const char *format, ...) {
	va_list args;
	char *label;
	if (s->shutdown) {
		label = "[Shutdown]";
	} else if (!s->started) {
		label = "[Startup]";
	} else {
		label = "[Running]";
	}

	if (s->verbose >= level) {
		fprintf(stdout, "%-10s Info:%d: ", label, level);
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
		fprintf(stdout, "\n");
	}

	if (s->log && (s->logverbose >= level)) {
		fprintf(s->log, "%s Info:%d: ", label, level);
		va_start(args, format);
		vfprintf(s->log, format, args);
		va_end(args);
		fprintf(s->log, "\n");
	}
}

/*!
 * Generates a file name with the form [prefix]YYYYMMDDXX.[extension], where XX
 * is a two digit hexadecimal serial number. Starting from 0, the proposed file
 * name is opened in exclusive create mode ("w+x"). If this fails because the
 * file exists, the serial number is incremented and another attempt to create
 * the file is made until either a) an unused serial number is found or b) the
 * serial number reaches 0xFF.
 *
 * If no valid file name can be generated, or if any other error is
 * encountered, the function returns NULL.
 *
 * @param[in] prefix    File name prefix (can include a path)
 * @param[in] extension File extension
 * @param[out] name     File name (without extension) used, if successful
 * @returns Opened file handle, or null on failure
 */
FILE *openSerialNumberedFile(const char *prefix, const char *extension, char **name) {
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	char *fileName = NULL;
	char date[9];
	strftime(date, 9, "%Y%m%d", tm);

	int i = 0;
	FILE *file = NULL;
	while (i <= 0xFF) {
		errno = 0;
		if (asprintf(&fileName, "%s%s%02x.%s", prefix, date, i, extension) == -1) { return NULL; }
		errno = 0;
		file = fopen(fileName, "w+x"); // w+x = rw + create. Fail if exists
		if (file) {
			if (name) {
				char *fStem = strndup(fileName, strlen(fileName) - strlen(extension) - 1);
				(*name) = fStem;
			}
			free(fileName);
			return file;
		}
		if (errno != EEXIST) {
			free(fileName);
			return NULL;
		}
		i++;
		free(fileName);
		fileName = NULL;
	}

	return NULL;
}

/*!
 * Ensure dynamically allocated program_state members are freed.
 * @param[in] s program_state to be destroyed
 */
void destroy_program_state(program_state *s) {
	if (s->log) { fclose(s->log); }
	s->log = NULL;
}
//!@}
