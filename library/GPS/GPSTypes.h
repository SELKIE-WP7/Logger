#ifndef SELKIELoggerGPS_Types
#define SELKIELoggerGPS_Types

/*!
 * @file GPSTypes.h Data types and definitions for use with UBX protocol messages
 * @ingroup SELKIELoggerGPS
 */

#include <stdint.h>
#include <stdbool.h>

/*!
 * @addtogroup SELKIELoggerGPS
 * @{
 */


/*! @brief UBX Serial synchronisation byte 1
 *
 * 0xB5 as defined in UBlox documentation.
 *
 * Represents the UTF-8 µ glyph
 */
#define UBX_SYNC_BYTE1 0xB5

/*! @brief UBX Serial synchronisation byte 2
 *
 * Defined as 0x62 in UBlox documentation.
 * Represents a b in ASCII or UTF-8.
 */
#define UBX_SYNC_BYTE2 0x62

//! UBX Message class ID bytes
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

/*!
 * @brief Internal representation of a UBX message
 *
 * All messages must start with two sync bytes: 0xB5 0x62
 *
 * Short messages are stored in the data array, larger messages will be stored
 * at the location pointed to by the extdata member.
 *
 * If extdata is set, it must be explicitly freed when message is disposed of.
 */
typedef struct ubx_message {
	uint8_t sync1; //!< Should always be 0xB5
	uint8_t sync2; //!< Should always be 0x62
	uint8_t msgClass; //!< A value from ubx_class
	uint8_t msgID; //!< Message ID byte
	uint16_t length; //!< Message length
	uint8_t data[256]; //!< Data if length <= 256
	uint8_t csumA; //!< Checksum part A
	uint8_t csumB; //!< Checksum part B
	uint8_t *extdata; //!< If length > 256, this points to allocated data array of correct length
} ubx_message;

//! UBX Message descriptions
typedef struct ubx_message_name {
	uint8_t msgClass; //!< ubx_class value
	uint8_t msgID; //!< Message ID byte
	char name[30]; //!< Human readable description/name
} ubx_message_name;

//! Represent decoded NAV-PVT message
typedef struct ubx_nav_pvt {
	uint32_t tow; //!< GPS Time of Week
	uint16_t year; //!< Calendar year
	uint8_t month; //!< Calendar month
	uint8_t day; //!< Calendar day
	uint8_t hour; //!< Hour (UTC)
	uint8_t minute; //!< Minute (UTC)
	uint8_t second; //!< Second (UTC)
	bool validDate; //!< Date data valid
	bool validTime; //!< Time data valid
	bool validMagDec; //!< Magnetic declination data valid
	uint32_t accuracy; //!< Estimated time accuracy (ns)
	int32_t nanosecond; //!< +/- nanosecond (UTC)
	uint8_t fixType; //!< Navigation Fix type
	uint8_t fixFlags; //!< Navigation status flags
	uint8_t fixFlags2; //!< Expanded navigation status flags
	uint8_t numSV; //!< Number of satellites used for current solution
	float longitude; //!< WGS84 Longitude
	float latitude; //!< WGS84 Latitude
	int32_t height; //!< WGS84 Height
	int32_t ASL; //!< Height above mean sea level (?datum)
	uint32_t horizAcc; //!< Horizontal accuracy estimate (mm)
	uint32_t vertAcc; //!< Vertical accuracy estimate
	int32_t northV; //!< Velocity (North, mm/s)
	int32_t eastV; //!< Velocity (East, mm/s)
	int32_t downV; //!< Velocity (Down, mm/s)
	int32_t groundSpeed; //!< Ground Speed (mm/s)
	float heading; //!< Motion heading
	int32_t speedAcc; //!< Speed/velocity accuracy estimate
	int32_t headingAcc; //!< Heading accuracy estimate
	uint16_t pDOP; //!< Position dilution
	uint8_t pvtFlags; //!< More flags
	float vehicleHeading; //!< Vehicle orientation
	float magneticDeclination; //!< Local magnetic field declination
	float magDecAcc; //!< Estimated accuracty of magnetic field declination
} ubx_nav_pvt;
//! @}
#endif
