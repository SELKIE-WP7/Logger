#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "strarray.h"

/*!
 * Allocate a new string array and initialise with sa_init()
 *
 * All allocated data is zeroed.
 *
 * Must be destroyed with sa_destroy() before being freed.
 *
 * @param[in] entries Number of entries in array
 * @return Pointer to allocated array. NULL on failure.
 */
strarray * sa_new(int entries) {
	strarray * newarray = calloc(1, sizeof(strarray));
	if (newarray == NULL) {
		return NULL;
	}
	if (!sa_init(newarray, entries)) {
		free(newarray);
		return NULL;
	}
	return newarray;
}

/*!
 * Take an array and initialise internal storage for the stated number of
 * entries.
 *
 * Will destroy existing contents
 *
 * @param[in] dst Pointer to array
 * @param[in] entries Number of strings to allocate
 * @returns True on success, false on error
 */
bool sa_init(strarray *dst, const int entries) {
	if (dst->entries) {
		sa_destroy(dst);
	}
	dst->entries = entries;
	dst->strings = calloc(entries, sizeof(string));
	if (dst->strings == NULL) {
		return false;
	}
	return true;
}


/*!
 * Destroys the existing contents of the destination array, which cannot be
 * restored in the event of an error, and then creates new strings to match the
 * contents of the source array.
 *
 * @param[out] dst Pointer to destination array (all existing data will be lost)
 * @param[in]  src Pointer to source array
 * @return True on success, false on error
 */
bool sa_copy(strarray *dst, const strarray *src) {
	if (dst == NULL || src == NULL) {
		return false;
	}
	sa_destroy(dst);
	dst->entries = src->entries;
	dst->strings = calloc(dst->entries, sizeof(string));
	if (dst->strings == NULL) {
		dst->entries = 0;
		return false;
	}
	bool success = true;
	for (int ix=0; ix < dst->entries; ix++) {
		success &= str_copy(&(dst->strings[ix]), &(src->strings[ix]));
	}
	if (!success) {
		sa_destroy(dst);
		return false;
	}
	return true;
}


/*!
 * Moves strings from one array to anther by copying the internal pointer of
 * the source array to the internal pointer of the destination. No additional
 * memory allocations are made.
 *
 * The source array is invalidated and the internal pointer nulled so that it
 * can be destroyed without affecting the destination array.
 *
 * @param[out] dst Pointer to destination array (all existing data will be lost)
 * @param[in]  src Pointer to source array (will be emptied)
 * @return True on success, false on error.
 */
bool sa_move(strarray *dst, strarray *src) {
	if (dst == NULL || src == NULL) {
		return false;
	}
	sa_destroy(dst);
	dst->entries = src->entries;
	dst->strings = src->strings;
	src->strings = NULL;
	src->entries = 0;
	return true;
}

/*!
 * str_copy() does the heavy lifting, with this function performing bounds
 * checks on the string array itself.
 *
 * @param[in] array Pointer to array being modified
 * @param[in] index Position in array to modify
 * @param[in] str String to be copied into array
 * @return Result of str_copy(), or false if parameters are invalid
 */
bool sa_set_entry(strarray *array, const int index, string *str) {
	if (array == NULL) {
		return false;
	}

	if (index >= array->entries) {
		return false;
	}

	return str_copy(&(array->strings[index]), str);
}

/*!
 * Checks that the array is valid and the index is within bounds, then hands
 * off to str_update() to copy in the new string data.
 *
 * @param[in] array Pointer to array being modified
 * @param[in] index Position in array to modify
 * @param[in] len Length of character array
 * @param[in] src Character array to be copied into array
 * @return Result of str_upate(), or false if parameters are invalid
 */
bool sa_create_entry(strarray *array, const int index, const size_t len, const char *src) {
	if (array == NULL) {
		return false;
	}
	if (index >= array->entries) {
		return false;
	}

	return str_update(&(array->strings[index]), len, src);
}

/*!
 * Hands off to str_destroy() if the specified index is within the array, and
 * noops if the array is invalid or index is out of bounds
 *
 * @param[in] array Pointer to array being modified
 * @param[in] index Position in array to clear
 */
void sa_clear_entry(strarray *array, const int index) {
	if (array == NULL) {
		return;
	}
	if (index >= array->entries) {
		return;
	}

	str_destroy(&(array->strings[index]));
}

/*!
 * Iterates over all array entries and calls str_destroy() for each string
 * before freeing the array's internal storage.
 *
 * Internal pointer is nulled and number of entries set to zero, so there
 * should be no side effects if called repeatedly on an array, if that should
 * happen for some reason. Don't do it deliberately just to check.
 *
 * Caller is responsible for freeing the array itself, if required.
 *
 * @param[in] sa Pointer to array to be destroyed.
 */
void sa_destroy(strarray *sa) {
	if (sa == NULL) {
		return;
	}
	if (sa->entries == 0) {
		// Why did we allocate a zero length array?
		if (sa->strings) {
			free(sa->strings);
			sa->strings = NULL;
		}
		return;
	}
	for (int ix=0; ix < sa->entries; ix++) {
		str_destroy(&(sa->strings[ix]));
	}
	free(sa->strings);
	sa->strings = NULL;
	sa->entries = 0;
}

/*!
 * Allocates a new string structure and copies in the character array.
 *
 * If last character of ca is not null then an extra byte is allocated and zeroed.
 * The declared length represents an upper bound on the string length - the
 * copy will stop at the first null byte found (strncpy() used internally)
 *
 * @param[in] len Maximum length of character array to be copied
 * @param[in] ca  Pointer to character array
 * @return Pointer to newly allocated string, or NULL on failure
 */
string * str_new(const size_t len, const char *ca) {
	string *ns = calloc(1, sizeof(string));
	if (ns == NULL) {
		return NULL;
	}

	ns->length = len;
	if (ca[len -1] != 0) {
		ns->length++;
	}
	ns->data = malloc(ns->length);
	if (ns->data == NULL) {
		free(ns);
		return NULL;
	}

	strncpy(ns->data, ca, len);
	ns->data[ns->length-1] = 0; // Force last byte to zero
	return ns;
}

/*!
 * Allocates memory and copies the string from src.
 *
 * No explicit null termination checks, but the source string declared length
 * is used as an upper bound. Uses strncpy() internally, so should be safe enough.
 *
 * @param[out] dst Pointer to destination string (Existing data will be destroyed)
 * @param[in]  src Pointer to source string.
 * @return True on success, false otherwise
 */
bool str_copy(string *dst, const string *src) {
	str_destroy(dst);
	dst->length = src->length;
	dst->data = malloc(dst->length);
	if (dst->data == NULL) {
		dst->length = 0;
		return false;
	}
	strncpy(dst->data, src->data, dst->length);
	return true;
}

/*!
 * Wrapper around str_new(), passing it the length and internal character array
 * of the source string.
 *
 * @param[in] src Pointer to source string
 * @return Return value of str_new()
 */
string * str_duplicate(const string *src) {
	return str_new(src->length, src->data);
}

/*!
 * Destroys the string, then allocates new memory and copies the array using
 * strncpy() as described for str_new()
 *
 * @param[out] str Pointer to destination string
 * @param[in]  len Maximum length of new character array to copy in
 * @param[in]  src Pointer to source character array
 * @return True on success, false otherwise
 */
bool str_update(string *str, const size_t len, const char *src) {
	str_destroy(str);
	str->length = len;
	if (len == 0) {
		str->data = NULL;
		return true;
	}
	str->data = malloc(len+1);
	if (str->data == NULL) {
		str->length = 0;
		return false;
	}
	strncpy(str->data, src, len);
	str->data[len] = 0;
	return true;
}

/*!
 * Frees the internal storage and sets length to zero
 *
 * Caller is responsible for freeing string itself, if required.
 *
 * @param[in] str Pointer to string to be destroyed
 */
void str_destroy(string *str) {
	if (str == NULL || str->length == 0) {
		return;
	}
	str->length = 0;
	if (str->data) {
		free(str->data);
	}
	str->data = NULL;
}
