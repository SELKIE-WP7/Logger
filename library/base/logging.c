#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "logging.h"

/*!
 * \addtogroup logging  Logging support functions
 * @{
 */

void log_error(const program_state *s, const char *format, ...) {
	va_list args;
	char *label;
	if (s->shutdown) {
		label="[Shutdown]";
	} else if (!s->started) {
		label="[Startup]";
	} else {
		label="[Running]";
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

void log_warning(const program_state *s, const char *format, ...) {
	va_list args;
	char *label;
	if (s->shutdown) {
		label="[Shutdown]";
	} else if (!s->started) {
		label="[Startup]";
	} else {
		label="[Running]";
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

void log_info(const program_state *s, const int level, const char *format, ...) {
	va_list args;
	char *label;
	if (s->shutdown) {
		label="[Shutdown]";
	} else if (!s->started) {
		label="[Startup]";
	} else {
		label="[Running]";
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

FILE *openSerialNumberedFile(const char *prefix, const char *extension) {
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	char *fileName = NULL;
	char date[9];
	strftime(date, 9, "%Y%m%d", tm);

	int i = 0;
	FILE *file = NULL;
	while (i <= 0xFF) {
		errno = 0;
		if (asprintf(&fileName, "%s%s%02x.%s", prefix, date, i, extension) == -1) {
			return NULL;
		}
		errno = 0;
		file = fopen(fileName, "w+x"); //w+x = rw + create. Fail if exists
		if (file) {
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

//!@}
