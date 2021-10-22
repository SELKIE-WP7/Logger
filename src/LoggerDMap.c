#include "LoggerDMap.h"

struct dmap {
	char *tag;
	device_callbacks (*dcb)();
};

struct dmap dmap[8] = {
	{"GPS", &gps_getCallbacks},
	{"I2C", &i2c_getCallbacks},
	{"NMEA", &nmea_getCallbacks},
	{"MP", &mp_getCallbacks},
	{"SL", &mp_getCallbacks},
	{"TIMER", &timer_getCallbacks},
	{"TICK", &timer_getCallbacks},
	{NULL, NULL} // End of list sentinel value
};


device_callbacks dmap_getCallbacks(const char * source) {
	size_t ix = 0;
	while (dmap[ix].tag) {
		if (strncasecmp(source, dmap[ix].tag, strlen(dmap[ix].tag)) == 0) {
			return dmap[ix].dcb();
		}
		ix++;
	}
	return (device_callbacks){0};
}
