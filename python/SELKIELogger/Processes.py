import pandas as pd

from os import path

import logging
from .SLFiles import DatFile, VarFile
from .SLMessages import IDs


def convertDataFile(varfile, datfile, output, fileFormat, timesource, resample, dropna):
    log = logging.getLogger("SELKIELogger.convertDataFile")

    log.info(f"Using '{varfile}' as channel map file")
    vf = VarFile(varfile)
    smap = vf.getSourceMap()

    log.info("Channel map created")
    log.info(f"Using {smap[timesource]} as master clock source")

    if fileFormat not in ["csv", "csv.gz", "xlsx", "parquet", "mat"]:
        log.error(f"Invalid output format: {fileFormat}!")
        raise Exception(f"Invalid output format requested: {fileFormat}")

    log.info("Beginning message processing")
    df = DatFile(datfile, pcs=timesource)
    df.addSourceMap(smap)
    data = df.asDataFrame(dropna=dropna, resample=resample)
    log.info("Message processing completed")

    log.info("Writing data out to file")
    if output is None:
        if resample:
            cf = path.splitext(datfile)[0] + "-" + resample + "." + fileFormat
        else:
            cf = path.splitext(datfile)[0] + "." + fileFormat
    else:
        cf = output

    if fileFormat == "csv":
        data.to_csv(cf)
    elif fileFormat == "csv.gz":
        data.to_csv(cf, compression="gzip")
    elif fileFormat == "xlsx":
        data.to_excel(cf, freeze_panes=(1, 1))
    elif fileFormat == "parquet":
        data.reset_index().to_parquet(cf, index=False)
    elif fileFormat == "mat":
        from scipy import io

        data.reset_index(inplace=True)

        def matlabFieldName(s):
            # Alphanumeric or underscores, starts with a letter
            import re

            s = re.sub("\W", "_", s)
            if not s[0].isalpha():
                s = "L_" + s
            return s

        data = data.T.dropna(how="all").T
        io.savemat(
            cf,
            {matlabFieldName(x): data[x].to_numpy() for x in data.columns},
            oned_as="column",
        )

    log.info(f"Data output to {cf}")


def extractFromFile(datfile, output, source, channel, raw=True):
    log = logging.getLogger("SELKIELogger.extractFromFile")

    log.info("Beginning message processing")
    df = DatFile(datfile)

    if output is None:
        if channel != IDs.SLCHAN_RAW:
            cf = path.splitext(datfile)[0] + f".s{source:02x}.c{channel:02x}.dat"
        else:
            cf = path.splitext(datfile)[0] + f".s{source:02x}.raw.dat"
    else:
        cf = output
    log.info(f"Writing data out to {cf}")

    with open(cf, "wb") as outFile:
        for m in df.messages(source=source, channel=channel):
            if raw:
                outFile.write(m.Data)
            else:
                outFile.write(m.pack())

    log.info(f"Data output to {cf}")
