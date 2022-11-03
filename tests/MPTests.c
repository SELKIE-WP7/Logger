#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include "SELKIELoggerBase.h"
#include "SELKIELoggerMP.h"

/*! @file MPTests.c
 *
 * @brief Test message creation and destruction
 *
 * @test Create a variety of msg_t objects with different types and values, dump
 * them to file and then recreates them.
 * @ingroup testing
 */

/*!
 * Tests different creation, reading and writing of msg_t structures.
 *
 * @param[in] argc Argument count
 * @param[in] argv Arguments
 * @returns 0 (Pass), -1 (Fail), -2 (Failed to run / Error)
 */
int main(int argc, char *argv[]) {
	msg_t *msn = msg_new_string(SLSOURCE_TEST1, SLCHAN_NAME, 22, "Software Test Source 1");
	assert(msn);

	// Generate a channel map for this "source"
	strarray *SA = sa_new(6);

	assert(SA);
	assert(sa_create_entry(SA, 0, 4, "Name"));
	assert(sa_create_entry(SA, 1, 8, "Channels"));
	assert(sa_create_entry(SA, 2, 9, "Timestamp"));
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

	uint8_t bytes[20] = {0x01, 0x05, 0xFD, 0xFC, 0x03, 0x02, 0x06, 0xFE, 0xFD, 0x04,
	                     0x03, 0x06, 0xFF, 0xFE, 0x05, 0x00, 0x04, 0xFC, 0xFB, 0x02};
	msg_t *mdb = msg_new_bytes(SLSOURCE_TEST1, 4, 20, bytes);
	assert(mdb);

	float f[3] = {1.1, 2.2, 3.14159};
	msg_t *mfa = msg_new_float_array(SLSOURCE_TEST1, 5, 3, f);

	assert(mp_writeMessage(fileno(stdout), msn));
	assert(mp_writeMessage(fileno(stdout), msa));
	assert(mp_writeMessage(fileno(stdout), mts));
	assert(mp_writeMessage(fileno(stdout), mdf));
	assert(mp_writeMessage(fileno(stdout), mdb));
	assert(mp_writeMessage(fileno(stdout), mfa));

	FILE *tmp = tmpfile();
	int tmpfd = fileno(tmp);
	assert(mp_writeMessage(tmpfd, msn));
	assert(mp_writeMessage(tmpfd, msa));
	assert(mp_writeMessage(tmpfd, mts));
	assert(mp_writeMessage(tmpfd, mdf));
	assert(mp_writeMessage(tmpfd, mdb));
	assert(mp_writeMessage(tmpfd, mfa));

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
	msg_destroy(mfa);
	free(mfa);

	sa_destroy(SA);
	free(SA);

	fflush(tmp);
	rewind(tmp);

	int count = 0;
	int exit = 0;
	while (!(feof(tmp) || exit == 1)) {
		msg_t tMessage = {0};
		if (mp_readMessage(fileno(tmp), &tMessage)) {
			// Successfully read message
			count++;
			char *str = msg_to_string(&tMessage);
			if (str) {
				fprintf(stdout, "%s\n", str);
				free(str);
				str = NULL;
			}
		} else {
			assert(tMessage.dtype == MSG_ERROR);
			switch ((uint8_t)tMessage.data.value) {
				//LCOV_EXCL_START
				case 0xAA:
					msg_destroy(&tMessage);
					fclose(tmp);
					return -2;
				case 0xEE:
					msg_destroy(&tMessage);
					fclose(tmp);
					return -1;
				//LCOV_EXCL_STOP
				case 0xFD:
					exit = 1;
					break;
				case 0xFF:
				default:
					break;
			}
		}
		msg_destroy(&tMessage);
	}
	assert(count == 6);

	fclose(tmp);
	return EXIT_SUCCESS;
}
