#include "LoggerConfig.h"

int config_handler(void* user, const char* section, const char* name, const char* value) {
	ini_config *c = (ini_config *)user;

	// For each new key/value:
	// - Find section
	// 	- If not found, create section (realloc as required)
	// - Append new kv item (realloc as required)
	int sectFound = -1;
	for (int i = 0; i < c->numsects; ++i) {
		if (c->sects[i].name && (strcmp(c->sects[i].name, section) == 0)) {
			sectFound = i;
			break;
		}
	}

	config_section *cs = NULL;
	if (sectFound < 0) {
		if (c->numsects < c->sectsize) {
			cs = &(c->sects[c->numsects++]);
			cs->name = strdup(section);
		} else {
			c->sectsize += 5;
			config_section *a = realloc(c->sects,c->sectsize * sizeof(config_section));
			if (a == NULL) {
				perror("config_handler");
				return -99;
			}
			c->sects = a;
			memset(&(c->sects[c->numsects]), 0, sizeof(config_section) * 5);
			cs = &(c->sects[c->numsects++]);
			cs->name = strdup(section);
		}
	} else {
		cs = &(c->sects[sectFound]);
	}

	config_kv *k = NULL;
	if (cs->numopts < cs->optsize) {
		k = &(cs->opts[cs->numopts++]);
	} else {
		cs->optsize += 5;
		config_kv *a = NULL;
		if (cs->opts) {
			a = realloc(cs->opts,cs->optsize * sizeof(config_kv));
		} else {
			a = calloc(cs->optsize, sizeof(config_kv));
		}

		if (a == NULL) {
			perror("config_handler");
			return -99;
		}
		cs->opts = a;
		memset(&(cs->opts[cs->numopts]), 0, sizeof(config_kv) * 5);
		k = &(cs->opts[cs->numopts++]);
	}
	k->key = strdup(name);
	k->value = strdup(value);

	return 0;
}

bool new_config(ini_config *c) {
	if (c == NULL) {
		return false;
	}
	c->sectsize = 10;
	c->sects = calloc(c->sectsize, sizeof(config_section));
	if (c->sects == NULL) {
		c->sectsize = 0;
		c->numsects = 0;
		perror("new_config");
		return false;
	}

	c->numsects = 1;
	c->sects[0].name = strdup("");
	c->sects[0].numopts = 0;
	c->sects[0].optsize = 10;
	c->sects[0].opts = calloc(c->sects[0].optsize, sizeof(config_kv));
	if (c->sects[0].opts == NULL) {
		c->sects[0].optsize = 0;
		c->sects[0].numopts = 0;
		c->numsects = 0;
		c->sectsize = 0;
		free(c->sects);
		perror("new_config");
		return false;
	}

	return true;
}

void destroy_config(ini_config *c) {
	if (c == NULL) {
		return;
	}

	for (int i = 0; i < c->numsects; ++i) {
		config_section *cs = &(c->sects[i]);
		for (int j = 0; j < cs->numopts; ++j) {
			free(cs->opts[j].key);
			free(cs->opts[j].value);
		}
		free(cs->opts);
		free(cs->name);
	}
	free(c->sects);
}

void print_config(ini_config *c) {
	if (c == NULL) {
		return;
	}

	for (int i = 0; i < c->numsects; ++i) {
		config_section *cs = &(c->sects[i]);
		if (strcmp("", cs->name)) {
			fprintf(stdout, "[%s]\n", cs->name);
		}
		for (int j = 0; j < cs->numopts; ++j) {
			fprintf(stdout, "%s = %s\n", cs->opts[j].key, cs->opts[j].value);
		}
		fprintf(stdout, "\n");
	}
}
