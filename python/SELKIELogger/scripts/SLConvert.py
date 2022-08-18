#!/usr/bin/env python3
from os import path

from SELKIELogger.scripts import log
from SELKIELogger.SLMessages import IDs
from SELKIELogger.Processes import convertDataFile


def processArguments():
    import argparse

    options = argparse.ArgumentParser(
        description="Read messages from SELKIE Logger or compatible output file and convert to other data formats",
        epilog="Created as part of the SELKIE project",
    )
    options.add_argument("file", metavar="DATFILE", help="Main data file name")
    options.add_argument(
        "-v", "--verbose", action="count", default=0, help="Increase output verbosity"
    )
    options.add_argument(
        "-c",
        "--varfile",
        default=None,
        help="Path to channel mapping file (.var file). Default: Auto detect or fall back to details in data file (slow)",
    )
    options.add_argument(
        "-d",
        "--dropna",
        default=False,
        action="store_const",
        const=True,
        help="Drop empty records during processing",
    )
    options.add_argument(
        "-r", "--resample", default=None, help="Resample data before writing to file"
    )
    options.add_argument(
        "-o", "--output", metavar="OUTFILE", default=None, help="Output file name"
    )
    options.add_argument(
        "-T",
        "--timesource",
        metavar="ID",
        default=IDs.SLSOURCE_TIMER,
        help="Data source to use as master clock",
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
    return options.parse_args()


def SLConvert():
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

    if args.varfile is None:
        log.warning("No channel map file provided")
        vf = path.splitext(args.file)[0] + ".var"
        if path.exists(vf):
            args.varfile = vf
            log.warning(f"Will attempt to use '{vf}' as channel map file")
        else:
            args.varfile = args.file
            log.warning(
                "No channel map (.var) file found - will fall back to information embedded in main data file. This can be very slow!"
            )

    return convertDataFile(
        args.varfile,
        args.file,
        args.output,
        args.format,
        args.timesource,
        args.resample,
        args.dropna,
        args.epoch,
    )


if __name__ == "__main__":
    SLConvert()
