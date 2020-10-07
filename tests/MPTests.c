#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

/*! @file MPTests.c
 *
 * @brief msgpack tests
 *
 * @addtogroup testing Test programs for use with CTest
 */
int main(int argc, char *argv[]) {

	msg_t *msn = msg_new_string(SLSOURCE_TEST1, SLCHAN_NAME, 22, "Software Test Source 1");
	assert(msn);

	// Generate a channel map for this "source"
	strarray *SA = sa_new(6);

	assert(SA);
	assert(sa_create_entry(SA, 0,  4, "Name"));
	assert(sa_create_entry(SA, 1,  8, "Channels"));
	assert(sa_create_entry(SA, 2,  9, "Timestamp"));
	assert(sa_create_entry(SA, 3, 16, "SA Entry 1 - One"));
	assert(sa_create_entry(SA, 4, 16, "SA Entry 2 - Two"));
	assert(sa_create_entry(SA, 5, 18, "SA Entry 3 - Three"));

	msg_t *msa = msg_new_string_array(SLSOURCE_TEST1, SLCHAN_MAP, SA);
	assert(msa);

	// Generate a timestamp message
	// Use a constant so that output can be compared
	uint32_t tstamp = 3503357677;
	msg_t *mts = msg_new_timestamp(SLSOURCE_TEST1, SLCHAN_TSTAMP, tstamp);
	assert(mts);

	msg_t *mdf = msg_new_float(SLSOURCE_TEST1, 3, 3.14159);
	assert(mdf);

	uint8_t bytes[20] = {0x01, 0x05, 0xFD, 0xFC, 0x03,
		0x02, 0x06, 0xFE, 0xFD, 0x04,
		0x03, 0x06, 0xFF, 0xFE, 0x05, 
		0x00, 0x04, 0xFC, 0xFB, 0x02};
	msg_t *mdb = msg_new_bytes(SLSOURCE_TEST1, 4, 20, bytes);
	assert(mdb);

	assert(mp_writeMessage(fileno(stdout), msn));
	assert(mp_writeMessage(fileno(stdout), msa));
	assert(mp_writeMessage(fileno(stdout), mts));
	assert(mp_writeMessage(fileno(stdout), mdf));
	assert(mp_writeMessage(fileno(stdout), mdb));


	msg_destroy(msn);
	free(msn);
	msg_destroy(msa);
	free(msa);
	msg_destroy(mts);
	free(mts);
	msg_destroy(mdf);
	free(mdf);
	msg_destroy(mdb);
	free(mdb);

	sa_destroy(SA);
	free(SA);

	return EXIT_SUCCESS;
}
