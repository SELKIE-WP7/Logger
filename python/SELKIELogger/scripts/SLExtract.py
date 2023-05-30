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

from os import path

from SELKIELogger.scripts import log
from SELKIELogger.SLMessages import IDs
from SELKIELogger.Processes import extractFromFile


def processArguments():
    import argparse

    options = argparse.ArgumentParser(
        description="Extract specific channel data from a SELKIE Logger data file",
        epilog="Created as part of the SELKIE project",
    )
    options.add_argument("file", metavar="DATFILE", help="Main data file name")
    options.add_argument(
        "-v", "--verbose", action="count", default=0, help="Increase output verbosity"
    )
    options.add_argument(
        "-r",
        "--raw",
        default=False,
        action="store_const",
        const=True,
        help="Resample data before writing to filea",
    )
    options.add_argument(
        "-s", "--source", required=True, default=None, help="Source ID to extract"
    )
    options.add_argument(
        "-c", "--channel", required=True, default=None, help="Channel ID to extract"
    )
    options.add_argument(
        "-o", "--output", metavar="OUTFILE", default=None, help="Output file name"
    )
    return options.parse_args()


def SLExtract():
    args = processArguments()
    if args.verbose > 1:
        log.setLevel(log.DEBUG)
    elif args.verbose > 0:
        log.setLevel(log.INFO)

    log.debug(f"Log level set to {log.getLevelName(log.getEffectiveLevel())}")
    log.info(f"Using '{args.file}' as data file")

    try:
        args.source = int(args.source, base=0)
    except Exception:
        log.error(f"Invalid source value supplied: {args.source}")
        return False

    try:
        args.channel = int(args.channel, base=0)
    except Exception:
        log.error(f"Invalid channel value supplied: {args.channel}")
        return False

    return extractFromFile(
        args.file,
        args.output,
        args.source,
        args.channel,
        args.raw,
    )


if __name__ == "__main__":
    SLExtract()
