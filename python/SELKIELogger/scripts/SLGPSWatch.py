#!/usr/bin/env python3

## @file

import pandas as pd

from numpy import sin, cos, sqrt, arcsin, power, deg2rad, isnan

from os import path
from time import monotonic, sleep

from SELKIELogger.scripts import log, ChannelSpec
from SELKIELogger.SLFiles import StateFile
from SELKIELogger.PushoverClient import PushoverClient


class LocatorSpec:
    """!
    LocatorSpec: String representation of location monitoring settings
    """

    def __init__(self, s):
        """!
        Split string into components defining location warning settings.
        Values are validated to the extent possible without device
        and source specific information
        String is a set of colon separated values in this order:
         - Latitude ChannelSpec()
         - Longitude ChannelSpec()
         - Reference Latitude
         - Reference Longitude
         - Warning threshold distance (m)
         - [Optional] Name
        @param s String in required format.
        """
        parts = s.split(",")
        if len(parts) < 5 or len(parts) > 6:
            raise ValueError("Invalid locator specification")

        ## ChannelSpec() identifying source of latitude data
        self.latChan = ChannelSpec(parts[0])

        ## ChannelSpec() identifying source of longitude data
        self.lonChan = ChannelSpec(parts[1])

        ## Reference latitude (decimal degrees)
        self.refLat = float(parts[2])

        ## Reference longitude (decimal degrees)
        self.refLon = float(parts[3])

        ## Warning Distance (m)
        self.threshold = float(parts[4])

        if len(parts) == 6:
            ## Name for this locator (used in reporting)
            self.name = parts[5]
        else:
            self.name = f"{self.latChan}/{self.lonChan}"

    def __str__(self):
        """! @returns Correctly formatted string"""
        return f"{self.latChan},{self.lonChan},{self.refLat},{self.refLon},{self.threshold},{self.name}"

    def __repr__(self):
        """! @returns Parseable representation"""
        return f"LocatorSpec('{self.__str__():s}')"


def process_arguments():
    """!
    Handle command line arguments
    @returns Namespace from parse_args()
    """
    import argparse

    options = argparse.ArgumentParser(
        description="Read a SELKIE Logger state file and check GPS position",
        epilog="Created as part of the SELKIE project",
    )
    options.add_argument("file", metavar="STATEFILE", help="State file name")

    options.add_argument(
        "-v", "--verbose", action="count", default=0, help="Increase output verbosity"
    )

    options.add_argument(
        "-t", "--interval", default=60, metavar="N", help="Poll every N seconds"
    )

    options.add_argument(
        "-P",
        "--disable-pushover",
        default=False,
        action="store_true",
        help="Disable pushover notifications",
    )

    options.add_argument(
        "locator",
        nargs="+",
        help="Specify lat/lon channels, reference point and distance threshold",
    )
    return options.parse_args()


def getLocatorValue(state, locator):
    """!
    Extract value from `state` corresponding to the ChannelSpec() in `locator`
    @param state Dataframe containing StateFile data
    @param locator ChannelSpec() to look up
    @returns floating point value, or NaN if not found
    """
    try:
        val = state.loc[(locator.source, locator.channel)].Value
        if locator.index is None:
            return float(val)
        else:
            return float(val.split("/")[locator.index - 1])
    except:
        return float("NaN")


def haversine(lat1, lon1, lat2, lon2):
    """!
    Haversine distance formula, from Wikipedia
    @param lat1 Latitude 1 (decimal degrees)
    @param lon1 Longitude 1 (decimal degrees)
    @param lat2 Latitude 2 (decimal degrees)
    @param lon2 Latitude 2 (decimal degrees)
    @returns Distance from 1 -> 2 in metres
    """
    hdLat = (deg2rad(lat2) - deg2rad(lat1)) / 2
    hdLon = (deg2rad(lon2) - deg2rad(lon1)) / 2

    # 2 * r, for r = WGS84 semi-major axis
    return (
        2
        * 6378137
        * arcsin(
            sqrt(power(sin(hdLat), 2) + cos(lat1) * cos(lat2) * power(sin(hdLon), 2))
        )
    )


def checkLocator(s, l):
    """!
    Check whether locator given in `l` is within warning threshold or not,
    based on the data in `s`

    In returned tuple, `alert` is True if position is unknown or further from
    the reference position than the warning threshold distance. The distance
    from the reference point in metres is returned along with the current
    position in decimal degrees.

    @param s State dataframe
    @param l LocatorSpec()
    @returns Tuple of form (alert, distance, (lat, lon))
    """
    curLat = getLocatorValue(s, l.latChan)
    curLon = getLocatorValue(s, l.lonChan)
    if isnan(curLat) or isnan(curLon):
        log.warning(f"{l.name} has invalid coordinates")
        return (True, float("nan"), (curLat, curLon))

    d = haversine(curLat, curLon, l.refLat, l.refLon)
    log.info(
        f"{l.name}:\t {curLat:.5f},{curLon:.5f} \t--\t {l.refLat:.5f},{l.refLon:.5f} \t--\t {d:.2f}"
    )
    if d > l.threshold:
        log.warning(f"{l.name} is {d - l.threshold:.0f}m outside threshold")
        log.info(
            f"{l.name} is {d:.1f}m from reference point, with a {l.threshold:.1f}m radius set"
        )
        return (True, d, (curLat, curLon))
    return (False, d, (curLat, curLon))


def generateWarningMessage(states):
    """!
    Iterates over `states` - an array of tuples from checkLocator - and
    generates a suitable warning message as a string.

    @param states Array of tuples from checkLocation()
    @returns Warning/Information message as string
    """
    message = "At least one GPS input has moved beyond configured limits:\n"
    for s in states:
        if isnan(s[2]):
            message += f"{s[0].name} has missing or invalid coordinates ({s[3][0]:.5f}, {s[3][1]:.5f})\n"
    message += "\n"

    # Report flagged markers first
    for s in states:
        if s[1] and not isnan(s[2]):
            message += (
                f"{s[0].name}: Current coordinates ({s[3][0]:.5f}, {s[3][1]:.5f}), "
            )
            message += f"{s[2] - s[0].threshold:.0f}m beyond {s[0].threshold:.0f}m limit from reference point.\n"
    message += "\n"

    # Now for the remainder:
    for s in states:
        if not s[1] and not isnan(s[2]):
            message += (
                f"{s[0].name}: Current coordinates ({s[3][0]:.5f}, {s[3][1]:.5f}), "
            )
            message += f"within {s[0].threshold:.0f}m of reference point.\n"
    return message


def SLGPSWatch():
    """!
    SLGPSWatch: Script entry point.
    @returns Exit status (0 on success)
    """
    args = process_arguments()
    if args.verbose > 1:
        log.setLevel(log.DEBUG)
    elif args.verbose > 0:
        log.setLevel(log.INFO)

    log.debug(f"Log level set to {log.getLevelName(log.getEffectiveLevel())}")
    log.info(f"Using '{args.file}' as state file")

    lastFlagged = None
    lastFlaggedTime = monotonic()
    while True:
        sf = StateFile(args.file)
        ds = sf.parse()

        states = []
        anyFlagged = False
        for l in args.locator:
            locator = LocatorSpec(l)
            flagged, distance, coords = checkLocator(ds, locator)
            states.append((locator, flagged, distance, coords))
            if flagged:
                anyFlagged = True

        if (lastFlagged != anyFlagged) and not args.disable_pushover:
            now = monotonic()
            if anyFlagged == False:
                PushoverClient().sendMessage(
                    "All monitored GPS inputs are within configured boundaries",
                    "[OK] GPS Monitoring",
                )
                lastFlagged = False
                lastFlaggedTime = now
            else:
                message = generateWarningMessage(states)
                PushoverClient().sendMessage(message, "[WARNING] GPS Monitoring")
                lastFlagged = True
                lastFlaggedTime = now

        sleep(args.interval)
