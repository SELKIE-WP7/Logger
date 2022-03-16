#!/usr/bin/python3
import inotify.adapters as ia
import pandas as pd

from numpy import sin, cos, sqrt, arcsin, power, deg2rad

from os import path
from time import monotonic, sleep

from . import log
from ..SLFiles import StateFile
from ..PushoverClient import PushoverClient


class ChannelSpec:
    def __init__(self, s):
        parts = s.split(":")
        l = len(parts)
        if l < 2 or l > 3:
            raise ValueError("Invalid channel specification")
        self.source = int(parts[0], base=0)
        self.channel = int(parts[1], base=0)
        if l == 3:
            self.index = int(parts[2], base=0)
        else:
            self.index = None

        if self.source < 0 or self.source > 127:
            raise ValueError("Invalid channel specification (bad source)")

        if self.channel < 0 or self.channel > 127:
            raise ValueError("Invalid channel specification (bad channel)")

        if not self.index is None and self.index < 0:
            raise ValueError("Invalid channel specification (bad index)")

    def __str__(self):
        if self.index is None:
            return f"0x{self.source:02x}:0x{self.channel:02x}"
        else:
            return f"0x{self.source:02x}:0x{self.channel:02x}:0x{self.index:02x}"


class LocatorSpec:
    def __init__(self, s):
        # Spec: latChan,lonChan,refLat,refLon,distance
        # ChannelSpec: source:type[:index]
        parts = s.split(",")
        if len(parts) < 5 or len(parts) > 6:
            raise ValueError("Invalid locator specification")

        self.latChan = ChannelSpec(parts[0])
        self.lonChan = ChannelSpec(parts[1])
        self.refLat = float(parts[2])
        self.refLon = float(parts[3])
        self.threshold = float(parts[4])

        if len(parts) == 6:
            self.name = parts[5]
        else:
            self.name = f"{self.latChan},{self.lonChan}"

    def __str__(self):
        return f"{self.latChan},{self.lonChan},{self.refLat},{self.refLon},{self.threshold}"


def process_arguments():
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
    val = state.loc[(locator.source, locator.channel)].Value
    if locator.index is None:
        return float(val)
    else:
        return float(val.split(",")[locator.index - 1])


def haversine(lat1, lon1, lat2, lon2):
    # 2 * r, for r = WGS84 semi-major axis
    hdLat = (deg2rad(lat2) - deg2rad(lat1)) / 2
    hdLon = (deg2rad(lon2) - deg2rad(lon1)) / 2

    return (
        2
        * 6378137
        * arcsin(
            sqrt(power(sin(hdLat), 2) + cos(lat1) * cos(lat2) * power(sin(hdLon), 2))
        )
    )


def checkLocator(s, l):
    curLat = getLocatorValue(s, l.latChan)
    curLon = getLocatorValue(s, l.lonChan)
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
    message = "At least one GPS input has moved beyond configured limits:\n"
    # Report flagged markers first
    for s in states:
        if s[1]:
            message += (
                f"{s[0].name}: Current coordinates ({s[3][0]:.5f}, {s[3][1]:.5f}), "
            )
            message += f"{s[2] - s[0].threshold:.0f}m beyond {s[0].threshold:.0f}m limit from reference point.\n"
    message += "\n"

    # Now for the remainder:
    for s in states:
        if not s[1]:
            message += (
                f"{s[0].name}: Current coordinates ({s[3][0]:.5f}, {s[3][1]:.5f}), "
            )
            message += f"within {s[0].threshold:.0f}m of reference point.\n"
    return message


def SLGPSWatch():
    args = process_arguments()
    if args.verbose > 1:
        log.setLevel(log.DEBUG)
    elif args.verbose > 0:
        log.setLevel(log.INFO)

    log.debug(f"Log level set to {log.getLevelName(log.getEffectiveLevel())}")
    log.info(f"Using '{args.file}' as state file")

    lastFlagged = False
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