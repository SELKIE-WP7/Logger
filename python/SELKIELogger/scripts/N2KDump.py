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

from SELKIELogger.scripts import log
from SELKIELogger.N2KMessages import ACTN2KMessage, ConvertN2KMessage


def process_arguments():
    import argparse

    options = argparse.ArgumentParser(
        description="Read N2K messages from an ACT gateway or compatible devices and parse details to screen",
        epilog="Created as part of the SELKIE project",
    )
    options.add_argument("file", help="Input file name")
    options.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        action="store_true",
        default=False,
        help="Include all messages (otherwise only messages with a known conversion are displayed)",
    )
    return options.parse_args()


def N2KDump():
    args = process_arguments()
    with open(args.file, "rb") as inFile:
        i = 0
        for x in ACTN2KMessage.yieldFromFile(inFile):
            if x:
                i += 1
                m = ConvertN2KMessage(x)
                if m:
                    print(f"{x.timestamp / 1E3:.03f} - {m}")
                elif args.verbose:
                    # If message wasn't converted, only print if verbose
                    print(f"{x.timestamp / 1E3:.03f} - {x}")

    print(f"{i} messages read from file")


if __name__ == "__main__":
    N2KDump()
