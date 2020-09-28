#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "strarray.h"

strarray * sa_new(int entries) {
	strarray * newarray = calloc(1, sizeof(strarray));
	if (newarray == NULL) {
		return NULL;
	}
	newarray->entries = entries;
	newarray->strings = calloc(entries, sizeof(string));
	if (newarray->strings == NULL) {
		free(newarray);
		return NULL;
	}
	return newarray;
}

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

bool sa_set_entry(strarray *array, const int index, string *str) {
	if (array == NULL) {
		return false;
	}

	if (index >= array->entries) {
		return false;
	}

	return str_copy(&(array->strings[index]), str);
}

bool sa_create_entry(strarray *array, const int index, const size_t len, const char *src) {
	if (array == NULL) {
		return false;
	}
	if (index >= array->entries) {
		return false;
	}

	return str_update(&(array->strings[index]), len, src);
}

void sa_clear_entry(strarray *array, const int index) {
	if (array == NULL) {
		return;
	}
	if (index >= array->entries) {
		return;
	}

	str_destroy(&(array->strings[index]));
}

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

string * str_new(const size_t len, const char *ca) {
	string *ns = calloc(1, sizeof(string));
	if (ns == NULL) {
		return NULL;
	}

	ns->length = len;
	ns->data = malloc(len);
	if (ns->data == NULL) {
		free(ns);
		return NULL;
	}

	strncpy(ns->data, ca, len);
	return ns;
}

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

string * str_duplicate(const string *src) {
	return str_new(src->length, src->data);
}

bool str_update(string *str, const size_t len, const char *src) {
	str_destroy(str);
	str->length = len;
	str->data = malloc(len);
	if (str->data == NULL) {
		str->length = 0;
		return false;
	}
	strncpy(str->data, src, len);
	return true;
}

void str_destroy(string *str) {
	if (str == NULL) {
		return;
	}
	str->length = 0;
	if (str->data) {
		free(str->data);
	}
	str->data = NULL;
}
