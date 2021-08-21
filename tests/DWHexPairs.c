#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "DWTypes.h"

bool test(const char *in, const uint8_t target) {
	uint8_t val = 0;
	bool res = hexpair_to_uint(in, &val);
	if (!res) {
		fprintf(stderr, "Conversion of '%c%c' failed\n", in[0], in[1]);
		return false;
	}

	if (val == target) {
		fprintf(stdout, "Conversion of '%c%c' successfully yielded %d\n", in[0], in[1], target);
		return true;
	}
	fprintf(stdout, "Conversion of '%c%c' failed (returned %d, not %d)\n", in[0], in[1], val, target);
	return false;
}


int main(int argc, char *argv[]) {
	const char *ts1 = "00";
	const char *ts2 = "01";
	const char *ts3 = "0a";
	const char *ts4 = "0A";
	const char *ts5 = "10";
	const char *ts6 = "ff";
	const char *ts7 = "FF";

	bool res = true;

	res &= test(ts1, 0);
	res &= test(ts2, 1);
	res &= test(ts3, 10);
	res &= test(ts4, 10);
	res &= test(ts5, 16);
	res &= test(ts6, 255);
	res &= test(ts7, 255);

	return (res?0:1);
}
