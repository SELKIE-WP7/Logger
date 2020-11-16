#ifndef SELKIELoggerBase_StrArray
#define SELKIELoggerBase_StrArray
#include <stdbool.h>

/*!
 * @file strarray.h String and String Array types and handling functions
 * @ingroup SELKIELoggerBase
 */

/*!
 * @defgroup strarray Strings and String arrays
 * @ingroup SELKIELoggerBase
 * @{
 */

//! Simple string type
typedef struct {
	size_t length; //!< This should include a terminating null byte where possible
	char *data; //!< Character array, should be null terminated.
} string;

typedef struct{
	int entries; //!< Maximum number of strings in array, set when calling sa_new()
	string *strings; //!< Simple array of string structures.
} strarray;

//! Allocate storage for a new array
strarray * sa_new(int entries);

//! Initialise an already allocated array
bool sa_init(strarray *dst, const int entries);

//! Copy an array of strings from src to dst
bool sa_copy(strarray *dst, const strarray *src);

//! Move strings from one array to another
bool sa_move(strarray *dst, strarray *src);

//! Copy a string to a given position in the array
bool sa_set_entry(strarray *array, const int index, string *str);

//! Create an string in a given position from a character array and length
bool sa_create_entry(strarray *array, const int index, const size_t len, const char *src);

//! Clear an array entry
void sa_clear_entry(strarray *array, const int index);

//! Destroy array and contents
void sa_destroy(strarray *sa);

//! Create new string from character array
string * str_new(const size_t len, const char *ca);

//! Update an existing string by copying the contents from another
bool str_copy(string *dst, const string *src);

//! Allocate a new string containing the contents of another
string * str_duplicate(const string *src);

//! Update an existing string from a character array of given length
bool str_update(string *str, const size_t len, const char *src);

//! Destroy a string and free its contents
void str_destroy(string *str);
//! @}
#endif
