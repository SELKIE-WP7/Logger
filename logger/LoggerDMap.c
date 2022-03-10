#include "LoggerDMap.h"

struct dmap {
	char *tag;
	device_callbacks (*dcb)();
	dc_parser dp;
};

struct dmap dmap[] = {
	{"DW", &dw_getCallbacks, &dw_parseConfig},
	{"GPS", &gps_getCallbacks, &gps_parseConfig},
	{"I2C", &i2c_getCallbacks, &i2c_parseConfig},
	{"NMEA", &nmea_getCallbacks, &nmea_parseConfig},
	{"N2K", &n2k_getCallbacks, &n2k_parseConfig},
	{"MP", &mp_getCallbacks, &mp_parseConfig},
	{"MQTT", &mqtt_getCallbacks, &mqtt_parseConfig},
	{"SL", &mp_getCallbacks, &mp_parseConfig},
	{"SERIAL", &rx_getCallbacks, &rx_parseConfig},
	{"NET", &net_getCallbacks, &net_parseConfig},
	{"TCP", &net_getCallbacks, &net_parseConfig},
	{"TIMER", &timer_getCallbacks, &timer_parseConfig},
	{"TICK", &timer_getCallbacks, &timer_parseConfig},
	{NULL, NULL} // End of list sentinel value
};

device_callbacks dmap_getCallbacks(const char *source) {
	size_t ix = 0;
	while (dmap[ix].tag) {
		if (strncasecmp(source, dmap[ix].tag, strlen(dmap[ix].tag)) == 0) {
			return dmap[ix].dcb();
		}
		ix++;
	}
	return (device_callbacks){0};
}

dc_parser dmap_getParser(const char *source) {
	size_t ix = 0;
	while (dmap[ix].tag) {
		if (strncasecmp(source, dmap[ix].tag, strlen(dmap[ix].tag)) == 0) {
			return dmap[ix].dp;
		}
		ix++;
	}
	return NULL;
}
