#include "GPSTypes.h"

// clang-format off
//! Human readable message names by class and ID
const ubx_message_name ubx_message_names[] = {
	{UBXACK, 0x00, "UBX-ACK-NAK"},
	{UBXACK, 0x01, "UBX-ACK-ACK"},
	{UBXCFG, 0x00, "UBX-CFG-PRT"},
	{UBXCFG, 0x01, "UBX-CFG-MSG"},
	{UBXCFG, 0x08, "UBX-CFG-RATE"},
	{UBXCFG, 0x11, "UBX-CFG-RXM"},
	{UBXCFG, 0x3E, "UBX-CFG-GNSS"},
	{UBXINF, 0x00, "UBX-INF-ERROR"},
	{UBXINF, 0x01, "UBX-INF-WARNING"},
	{UBXINF, 0x02, "UBX-INF-NOTICE"},
	{UBXINF, 0x03, "UBX-INF-TEST"},
	{UBXINF, 0x04, "UBX-INF-DEBUG"},
	{UBXMON, 0x28, "UBX-MON-GNSS"},
	{UBXNAV, 0x03, "UBX-NAV-STATUS"},
	{UBXNAV, 0x07, "UBX-NAV-PVT"},
	{UBXNAV, 0x21, "UBX-NAV-TIMEUTC"},
	{UBXNAV, 0x35, "UBX-NAV-SAT"},
};
