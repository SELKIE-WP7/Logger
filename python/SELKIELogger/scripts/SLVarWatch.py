#!/usr/bin/env python3

# Copyright (C) 2023 Swansea University
#
# This file is part of the SELKIELogger suite of tools.
#
# SELKIELogger is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this SELKIELogger product.
# If not, see <http://www.gnu.org/licenses/>.

## @file

import pandas as pd

from numpy import sin, cos, sqrt, arcsin, power, deg2rad, isnan

from os import path
from time import monotonic, sleep

from SELKIELogger.scripts import log
from SELKIELogger.Specs import ChannelSpec, LocatorSpec, LimitSpec
from SELKIELogger.SLFiles import StateFile
from SELKIELogger.PushoverClient import PushoverClient


def process_arguments():
    """!
    Handle command line arguments
    @returns Namespace from parse_args()
    """
    import argparse

    options = argparse.ArgumentParser(
        description="Read a SELKIE Logger state file and check variables against thresholds",
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
        "variable",
        nargs="+",
        help="Specify channel and min/max values",
    )
    return options.parse_args()


def generateWarningMessage(states):
    """!
    Iterates over `states` - an array of limit,state,value tuples - and
    generates a suitable warning message as a string.

    @param states Array of tuples (variable, alarm, value)
    @returns Warning/Information message as string
    """
    message = "At least one monitored channel has a value outside configured limits:\n"
    for s in states:
        if isnan(s[2]):
            message += f"{s[0].name} has missing or invalid value ({s[2]:.3f})\n"
    message += "\n"

    # Report flagged markers first
    for s in states:
        if s[1]:
            message += f"{s[0].name}: {s[2]:.3f} - Outside limits [{s[0].lowLim:.3f}:{s[0].highLim:.3f}]\n"
    message += "\n"

    # Now for the remainder:
    for s in states:
        if not s[1] and not isnan(s[2]):
            message += f"{s[0].name}: {s[2]:.3f} - within limits [{s[0].lowLim:.3f}:{s[0].highLim:.3f}]\n"
    return message


def SLVarWatch():
    """!
    SLVarWatch: Script entry point.
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
        for l in args.variable:
            var = LimitSpec(l)
            flagged = var.check(ds)
            value = var.getValue(ds)

            log.info(
                f"{var.name}:\t {value:.3f} \t--\t {var.lowLim:.3f},{var.highLim:.3f}"
            )
            if flagged:
                if value > var.highLim:
                    log.warning(
                        f"{var.name} value {value} is above upper limit ({var.highLim})"
                    )

                if value < var.lowLim:
                    log.warning(
                        f"{var.name} value {value} is below lower limit ({var.lowLim})"
                    )

                if isnan(value):
                    log.warning(f"{var.name} value is currently unknown")

            states.append((var, flagged, value))
            if flagged:
                anyFlagged = True

        if (lastFlagged != anyFlagged) and not args.disable_pushover:
            now = monotonic()
            if anyFlagged == False:
                PushoverClient().sendMessage(
                    "All monitored channels are within configured limits",
                    "[OK] Channel Monitoring",
                )
                lastFlagged = False
                lastFlaggedTime = now
            else:
                message = generateWarningMessage(states)
                PushoverClient().sendMessage(message, "[WARNING] Channel Monitoring")
                lastFlagged = True
                lastFlaggedTime = now

        sleep(args.interval)
