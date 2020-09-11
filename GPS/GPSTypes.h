#ifndef SELKIEGPSTypes
#define SELKIEGPSTypes

#include <stdint.h>

typedef enum ubx_class {
	UBXNAV = 0x01,
	UBXRXM = 0x02,
	UBXINF = 0x04,
	UBXACK = 0x05,
	UBXCFG = 0x06,
	UBXUPD = 0x09,
	UBXMON = 0x0A,
	UBXAID = 0x0B,
	UBXTIM = 0x0D,
	UBXESF = 0x10,
	UBXMGA = 0x13,
	UBXLOG = 0x21,
	UBXSEC = 0x27,
	UBXHNR = 0x28,
} ubx_class;

typedef struct ubx_message {
	uint8_t sync1;
	uint8_t sync2;
	uint8_t msgClass;
	uint8_t msgID;
	uint16_t length;
	uint8_t data[256];
	uint8_t csumA;
	uint8_t csumB;
	uint8_t *extdata;
} ubx_message;

typedef struct ubx_message_name {
	uint8_t msgClass;
	uint8_t msgID;
	char name[30];
} ubx_message_name;

#endif
