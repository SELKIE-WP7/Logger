/*
 *  Copyright (C) 2023 Swansea University
 *
 *  This file is part of the SELKIELogger suite of tools.
 *
 *  SELKIELogger is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this SELKIELogger product.
 *  If not, see <http://www.gnu.org/licenses/>.
*/

#include "LoggerDMap.h"

//! Device map entry
struct dmap {
	char *tag;                     //!< ID tag
	device_callbacks (*dcb)(void); //!< Device getCallbacks function
	dc_parser dp;                  //!< Device parseConfig function
};

/*!
 * Global device map instance
 *
 * Each device type gets at least one entry here.
 * The dmap.tag value will be used in configuration files, so some aliases are provided.
 * The final entry in this list **must** be NULLs
 */
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
	{"LPMS", &lpms_getCallbacks, &lpms_parseConfig},
	{NULL, NULL, NULL} // End of list sentinel value
};

/*!
 * Iterate over the global device map, performing a case insensitive comparison
 * between input string and the dmap.tag values.
 *
 * If a match is found, call the associated device callback function and return
 * the result.
 *
 * @param[in] source Null terminated string to compare against dmap.tag values
 * @returns Return value of device callback function or NULLs if no match found
 */
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

/*!
 * Iterate over the global device map, performing a case insensitive comparison
 * between input string and the dmap.tag values.
 *
 * If a match is found, return a pointer to the associated device configuration parser
 *
 * @param[in] source Null terminated string to compare against dmap.tag values
 * @returns Return pointer to device configuration parser or NULL if no match found
 */
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
