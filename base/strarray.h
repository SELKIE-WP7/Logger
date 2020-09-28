#ifndef SELKIEstrarray
#define SELKIEstrarray
#include <stdbool.h>

typedef struct {
	size_t length;
	char *data;
} string;

typedef struct{
	int entries;
	string *strings;
} strarray;

strarray * sa_new(int entries);
bool sa_copy(strarray *dst, const strarray *src);
bool sa_move(strarray *dst, strarray *src);
bool sa_set_entry(strarray *array, const int index, string *str);
bool sa_create_entry(strarray *array, const int index, const size_t len, const char *src);
void sa_clear_entry(strarray *array, const int index);
void sa_destroy(strarray *sa);

string * str_new(const size_t len, const char *ca);
bool str_copy(string *dst, const string *src);
string * str_duplicate(const string *src);
bool str_update(string *str, const size_t len, const char *src);
void str_destroy(string *str);
#endif
