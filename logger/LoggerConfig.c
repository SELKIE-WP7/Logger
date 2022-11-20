#include "LoggerConfig.h"

/*!
 *
 * Not called directly. This function is called by ini_parse() and used to
 * populate an ini_config structure that is passed in as the first parameter.
 * The ini_config structure must have been initialised before passing to the
 * initial ini_parse() call.
 *
 * @param[in] user Pointer to ini_config structure passed to ini_parse()
 * @param[in] section Section name
 * @param[in] name Key name
 * @param[in] value Key value, as string
 * @return 0 on error (abort parsing), 1 on success
 */
int config_handler(void *user, const char *section, const char *name, const char *value) {
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
		if (c->numsects < (ssize_t) c->sectsize) {
			cs = &(c->sects[c->numsects++]);
			cs->name = strdup(section);
		} else {
			c->sectsize += 5;
			config_section *a =
				realloc(c->sects, c->sectsize * sizeof(config_section));
			if (a == NULL) {
				perror("config_handler");
				return 0;
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
	if (cs->numopts < (ssize_t) cs->optsize) {
		k = &(cs->opts[cs->numopts++]);
	} else {
		cs->optsize += 5;
		config_kv *a = NULL;
		if (cs->opts) {
			a = realloc(cs->opts, cs->optsize * sizeof(config_kv));
		} else {
			a = calloc(cs->optsize, sizeof(config_kv));
		}

		if (a == NULL) {
			perror("config_handler");
			return 0;
		}
		cs->opts = a;
		memset(&(cs->opts[cs->numopts]), 0, sizeof(config_kv) * 5);
		k = &(cs->opts[cs->numopts++]);
	}
	k->key = config_qstrdup(name);
	k->value = config_qstrdup(value);

	return 1;
}

/*!
 * Allocates memory and initialises the structure with sensible defaults.
 *
 * Allocates space for 10 sections and initialises section 0 as a blank/unnamed
 * section with space for up to 10 key=value pairs.
 *
 * @param c Pointer to ini_config structure to be initialised.
 * @returns True on success, false on failure.
 */
bool new_config(ini_config *c) {
	if (c == NULL) { return false; }
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

/*!
 * Recursively frees memory within an ini_config structure.
 *
 * @param c Pointer to ini_config structure to be destroyed
 */
void destroy_config(ini_config *c) {
	if (c == NULL) { return; }

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

/*!
 * Will print formatted configuration to stdout, suitable for reuse in a
 * configuration file.
 *
 * @param[in] c Pointer to ini_config structure
 */
void print_config(ini_config *c) {
	if (c == NULL) { return; }

	for (int i = 0; i < c->numsects; ++i) {
		config_section *cs = &(c->sects[i]);
		if (strcmp("", cs->name) != 0) { fprintf(stdout, "[%s]\n", cs->name); }
		for (int j = 0; j < cs->numopts; ++j) {
			fprintf(stdout, "%s = %s\n", cs->opts[j].key, cs->opts[j].value);
		}
		fprintf(stdout, "\n");
	}
}

/*!
 * Iterates over all sections in the supplied ini_config structure and checks
 * for a matching section name.
 *
 * Comparison between search key and section names is case insensitive.
 *
 * @param[in] in Pointer to ini_config structure to search
 * @param[in] sn Pointer to null-terminated search string
 * @returns Pointer to first config_section found, or NULL if no matches
 */
config_section *config_get_section(const ini_config *in, const char *sn) {
	for (int i = 0; i < in->numsects; i++) {
		config_section *cs = &(in->sects[i]);
		if (strcasecmp(sn, cs->name) == 0) { return cs; }
	}
	return NULL;
}

/*!
 * Iterates over all entries in the supplied config_section structure and
 * checks for a matching key name.
 *
 * Comparison between search key and key names is case insensitive.
 *
 * @param[in] cs Pointer to config_section structure to search
 * @param[in] kn Pointer to null-terminated search string
 * @returns Pointer to first config_kv found, or NULL if no matches
 */
config_kv *config_get_key(const config_section *cs, const char *kn) {
	for (int i = 0; i < cs->numopts; i++) {
		config_kv *k = &(cs->opts[i]);
		if (strcasecmp(kn, k->key) == 0) { return k; }
	}
	return NULL;
}

/*!
 * Convenience wrapper around config_get_section and config_get_key.
 *
 * Will search for section, then search that section for a matching key.
 *
 * @param[in] in Pointer to ini_config structure to search
 * @param[in] sn Pointer to null-terminated section name
 * @param[in] kn Pointer to null-terminated key name
 * @returns Pointer to first config_kv found, or NULL if no matches
 */
config_kv *config_get_option(const ini_config *in, const char *sn, const char *kn) {
	config_section *cs = config_get_section(in, sn);
	if (cs == NULL) { return NULL; }
	return config_get_key(cs, kn);
}

/*!
 * Provides a consistent approach for parsing strings to boolean values.
 *
 * This is a very simple test - any string starting with 1, Y, y, t, or T is
 * considered to be True and anything starting with 0, N, n, F, or f will be
 * considered False.
 *
 * Any other initial character is treated as unknown.
 *
 * @param[in] b Pointer to null-terminated string
 * @returns 1 for true, 0 for false and -1 for unknown
 */
int config_parse_bool(const char *b) {
	if (b == NULL || (strcmp(b, "") == 0)) { return -1; }

	switch (b[0]) {
		case 'Y':
		case 'y':
		case 'T':
		case 't':
		case '1':
			return 1;
		case 'N':
		case 'n':
		case 'F':
		case 'f':
		case '0':
			return 0;
	}
	return -1;
}

/*!
 * Checks for matching single or double quotes around a string and duplicates the enclosed value.
 *
 * If no quotes are present (or if the leading and trailing quotes don't match)
 * then the string is duplicated in its entirety.
 *
 * Input string is not modified. Returned string must be freed by caller.
 *
 * @param[in] c String to duplicate
 * @returns Pointer to new string or NULL on failure.
 */
char *config_qstrdup(const char *c) {
	size_t sl = strlen(c);
	if (((c[0] == '"') && (c[sl - 1] == '"')) || ((c[0] == '\'') && (c[sl - 1] == '\''))) {
		// Length minus the two quote chars
		return strndup(&(c[1]), sl - 2);
	}

	// No matched quotes? Dup what we were given
	return strdup(c);
}
