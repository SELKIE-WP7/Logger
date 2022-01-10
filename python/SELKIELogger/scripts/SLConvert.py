#!/usr/bin/python3
import msgpack
import pandas as pd

from os import path
from . import log
from ..SLMessages import SLMessage, SLMessageSink, IDs
from ..SLFiles import DatFile, VarFile

def process_arguments():
    import argparse
    options = argparse.ArgumentParser(description="Read messages from SELKIE Logger or compatible output file and convert to other data formats", epilog="Created as part of the SELKIE project")
    options.add_argument("file", metavar="DATFILE",  help="Main data file name")
    options.add_argument("-v", "--verbose", action="count", default=0, help="Increase output verbosity")
    options.add_argument("-c", "--varfile", default=None, help="Path to channel mapping file (.var file). Default: Auto detect or fall back to details in data file (slow)")
    options.add_argument("-d", "--dropna", default=False, action="store_const", const=True, help="Drop empty records during processing")
    options.add_argument("-o", "--output", metavar="OUTFILE", default=None, help="Output file name")
    options.add_argument("-T", "--timesource", metavar="ID", default=IDs.SLSOURCE_TIMER, help="Data source to use as master clock")
    options.add_argument("-t", "--format", default="csv", choices=["csv", "xlsx", "parquet", "mat"], help="Output file format")
    options.add_argument("-z", "--compress", default=False, action="store_const", const=True, help="Enable compression of CSV output")
    return options.parse_args()

def SLConvert():
    args = process_arguments()
    if args.verbose > 1:
        log.setLevel(log.DEBUG)
    elif args.verbose > 0:
        log.setLevel(log.INFO)

    log.debug(f"Log level set to {log.getLevelName(log.getEffectiveLevel())}")
    log.info(f"Using '{args.file}' as main data file")

    if args.compress:
        if not args.format=="csv":
            log.warning("Explicit compression only supported for CSV output")
        else:
            args.format="csv.gz"

    if args.varfile is None:
        log.warning("No channel map file provided")
        vf = path.splitext(args.file)[0] + '.var'
        if path.exists(vf):
            args.varfile = vf
            log.warning(f"Will attempt to use '{vf}' as channel map file")
        else:
            args.varfile = args.file
            log.warning("No channel map (.var) file found - will fall back to information embedded in main data file. This can be very slow!")

    log.info(f"Using '{args.varfile}' as channel map file")
    vf = VarFile(args.varfile)
    smap = vf.getSourceMap()
    if args.verbose > 1:
        vf.printSourceMap(smap)

    log.info("Channel map created")
    log.info(f"Using {smap[args.timesource]} as master clock source")

    log.info("Beginning message processing")
    df = DatFile(args.file, pcs=args.timesource)
    df.addSourceMap(smap)
    data = df.asDataFrame(dropna=args.dropna)
    log.info("Message processing completed")

    log.info("Writing data out to file")
    if args.output is None:
        cf = path.splitext(args.file)[0] + '.' + args.format

    else:
        cf = args.output
    if args.format == "csv":
        data.to_csv(cf)
    elif args.format == "csv.gz":
        data.to_csv(cf, compression="gzip")
    elif args.format == "xlsx":
        data.to_excel(cf, freeze_panes=(1,1))
    elif args.format == "parquet":
        data.reset_index().to_parquet(cf, index=False)
    elif args.format == "mat":
        from scipy import io
        data.reset_index(inplace=True)
        def matlabFieldName(s):
            # Alphanumeric or underscores, starts with a letter
            import re
            s = re.sub("\W", "_", s)
            if not s[0].isalpha():
                s = "L_" + s
            return s
        data = data.T.dropna(how='all').T
        io.savemat(cf, {matlabFieldName(x): data[x].to_numpy() for x in data.columns}, oned_as="column")
    else:
        log.error(f"Invalid output format: {args.format}!")
        raise Exception(f"Invalid output format requested: {args.format}")

    log.info(f"Data output to {cf}")


if __name__ == "__main__":
    SLConvert()
