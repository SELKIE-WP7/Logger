#!/usr/bin/env python3
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
