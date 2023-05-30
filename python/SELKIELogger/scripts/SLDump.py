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

import msgpack
from SELKIELogger.scripts import log
from SELKIELogger.SLMessages import SLMessageSink


def process_arguments():
    import argparse

    options = argparse.ArgumentParser(
        description="Read messages from SELKIE Logger or compatible devices and parse details to screen",
        epilog="Created as part of the SELKIE project",
    )
    options.add_argument("file", help="Input file name")
    options.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        action="store_true",
        default=False,
        help="Include timing and source/channel information messages",
    )
    options.add_argument(
        "-s",
        "--source-map",
        dest="map",
        action="store_true",
        default=False,
        help="Dump source map",
    )
    return options.parse_args()


def processMessages(up, out, allM):
    for msg in up:
        msg = out.Process(msg, output="string", allMessages=allM)
        if msg:
            print(msg)


def SLDump():
    args = process_arguments()
    inFile = open(args.file, "rb")
    unpacker = msgpack.Unpacker(inFile, unicode_errors="ignore")
    out = SLMessageSink(msglogger=log)
    processMessages(unpacker, out, args.verbose)
    inFile.close()

    if args.map:
        sm = out.SourceMap()
        print("Source          \tChannels")
        for src in sm:
            print(f"0x{src:02x} - {sm.GetSourceName(src):16s}{list(sm[src])}")


if __name__ == "__main__":
    SLDump()
