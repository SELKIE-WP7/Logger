#!/usr/bin/env python3
from os import path

from SELKIELogger.scripts import log
from SELKIELogger.Processes import N2KToTimeseries


def processArguments():
    import argparse

    options = argparse.ArgumentParser(
        description="Read N2K messages from an ACT gateway or compatible devices and write timeseries in chosen data format",
        epilog="Created as part of the SELKIE project",
    )
    options.add_argument("file", metavar="N2KFILE", help="Data file name")
    options.add_argument(
        "-v", "--verbose", action="count", default=0, help="Increase output verbosity"
    )
    options.add_argument(
        "-d",
        "--dropna",
        default=False,
        action="store_const",
        const=True,
        help="Drop empty columns during processing",
    )
    options.add_argument(
        "-o", "--output", metavar="OUTFILE", default=None, help="Output file name"
    )
    options.add_argument(
        "-t",
        "--format",
        default="csv",
        choices=["csv", "xlsx", "parquet", "mat"],
        help="Output file format",
    )
    options.add_argument(
        "-z",
        "--compress",
        default=False,
        action="store_const",
        const=True,
        help="Enable compression of CSV output",
    )
    options.add_argument(
        "-e",
        "--epoch",
        default=False,
        action="store_const",
        const=True,
        help="Convert running timestamps to date/time values using Epoch field from primary clock source",
    )
    ### Also need to add PGN selection support
    return options.parse_args()


def N2KConvert():
    args = processArguments()
    if args.verbose > 1:
        log.setLevel(log.DEBUG)
    elif args.verbose > 0:
        log.setLevel(log.INFO)

    log.debug(f"Log level set to {log.getLevelName(log.getEffectiveLevel())}")
    log.info(f"Using '{args.file}' as main data file")

    if args.compress:
        if not args.format == "csv":
            log.warning("Explicit compression only supported for CSV output")
        else:
            args.format = "csv.gz"

    return N2KToTimeseries(
        args.file, args.output, args.format, args.dropna, epoch=args.epoch
    )


if __name__ == "__main__":
    N2KConvert()
