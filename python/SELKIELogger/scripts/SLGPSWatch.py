#!/usr/bin/env python3

## @file

import pandas as pd

from numpy import sin, cos, sqrt, arcsin, power, deg2rad, isnan

from os import path
from time import monotonic, sleep

from SELKIELogger.scripts import log
from SELKIELogger.Specs import ChannelSpec, LocatorSpec
from SELKIELogger.SLFiles import StateFile
from SELKIELogger.PushoverClient import PushoverClient


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
            flagged, distance, coords = locator.check(ds)
            # Locator(ds, locator)
            log.info(
                f"{locator.name}:\t {coords[0]:.5f},{coords[1]:.5f} \t--\t {locator.refLat:.5f},{locator.refLon:.5f} \t--\t {distance:.2f}"
            )
            if distance > locator.threshold:
                log.warning(
                    f"{locator.name} is {distance - locator.threshold:.0f}m outside threshold"
                )
                log.info(
                    f"{locator.name} is {distance:.1f}m from reference point, with a {locator.threshold:.1f}m radius set"
                )

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
