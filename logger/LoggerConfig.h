#ifndef SL_LOGGER_CONFIG_H
#define SL_LOGGER_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inih/ini.h"

/*! @file
 * Configuration parsing functions and types
 */

/*!
 * @brief Represent a key=value pair
 *
 * Each k=v line in the file is represented as a pair of strings for later
 * processing.
 */
typedef struct {
	char *key;   //!< Configuration item key
	char *value; //!< Configuration item value
} config_kv;

/*!
 * @brief Configuration file section
 *
 * The configuration file can be broken into sections, headed by a [tag].
 * The tag is stored as name, with subsequent key=value pairs in a dynamically
 * allocated array.
 */
typedef struct {
	char *name;      //!< Section [tag]
	size_t optsize;  //!< Allocated size of *opts
	int numopts;     //!< Number of *opts in use
	config_kv *opts; //!< Key=value pairs belonging to this section
} config_section;

/*!
 * @brief Representation of a parsed .ini file
 *
 * Represents a configuration file as an array of sections, each containing the
 * individual key=value pairs.
 *
 * Comments are not included.
 *
 * Any options present at the top of a file, outside any section, are present
 * in the first section with the section name set to an empty string.
 */
typedef struct {
	size_t sectsize;       //!< Allocated size of *sects
	int numsects;          //!< Number of sections defined
	config_section *sects; //!< Array of sections
} ini_config;

//! Populate ini_config instance with values from file
int config_handler(void *user, const char *section, const char *name, const char *value);

//! Initialise a new ini_config instance
bool new_config(ini_config *c);

//! Destroy ini_config instance
void destroy_config(ini_config *c);

//! Print ini_config instance to stdout
void print_config(ini_config *c);

//! Find configuration section by name
config_section *config_get_section(const ini_config *in, const char *sn);

//! Find configugration key within specific section, by name
config_kv *config_get_key(const config_section *cs, const char *kn);

//! Find configuration option by section and key names
config_kv *config_get_option(const ini_config *in, const char *sn, const char *kn);

//! Parse string to boolean
int config_parse_bool(const char *b);

//! Duplicate string, stripping optional leading/trailing quote marks
char *config_qstrdup(const char *c);
#endif
