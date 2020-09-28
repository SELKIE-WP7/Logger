#ifndef SELKIEMessages
#define SELKIEMessages
#include <stdint.h>
#include "strarray.h"

typedef union {
	float value;
	uint32_t timestamp;
	uint8_t *bytes;
	string string;
	strarray *names;
} msg_data_t;

typedef enum {
	MSG_UNDEF = 0,
	MSG_FLOAT,
	MSG_TIMESTAMP,
	MSG_BYTES,
	MSG_STRING,
	MSG_STRARRAY
} msg_dtype_t;

typedef struct {
	uint8_t source;
	uint8_t type;
	size_t length;
	msg_dtype_t dtype;
	msg_data_t data;
} msg_t;

msg_t * msg_new_float(const uint8_t source, const uint8_t type, const float val);
msg_t * msg_new_timestamp(const uint8_t source, const uint8_t type, const uint32_t ts);
msg_t * msg_new_string(const uint8_t source, const uint8_t type, const size_t len, const char *str);
msg_t * msg_new_string_array(const uint8_t source, const uint8_t type, const strarray *array);
msg_t * msg_new_bytes(const uint8_t source, const uint8_t type, const size_t len, const uint8_t *bytes);

void msg_destroy(msg_t *msg);
#endif
