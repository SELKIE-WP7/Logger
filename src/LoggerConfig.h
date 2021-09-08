#ifndef SL_LOGGER_CONFIG_H
#define SL_LOGGER_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inih/ini.h"

typedef struct {
	char *key;
	char *value;
} config_kv;

typedef struct {
	char *name;
	size_t optsize;
	int numopts;
	config_kv *opts;
} config_section;

typedef struct {
	size_t sectsize;
	int numsects;
	config_section *sects;
} ini_config;

int config_handler(void* user, const char* section, const char* name, const char* value);

bool new_config(ini_config *c);

void destroy_config(ini_config *c);

void print_config(ini_config *c);

config_section *config_get_section(const ini_config *in, const char *sn);

config_kv *config_get_key(const config_section *cs, const char *kn);

config_kv *config_get_option(const ini_config *in, const char *sn, const char *kn);

int config_parse_bool(const char *b);
#endif
