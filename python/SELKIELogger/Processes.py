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

import pandas as pd

from os import path, access, W_OK

import logging
from .SLFiles import DatFile, VarFile
from .SLMessages import IDs


def convertDataFile(
    varfile,
    datfile,
    output,
    fileFormat,
    timesource,
    resample,
    dropna=False,
    epoch=False,
):
    log = logging.getLogger("SELKIELogger.convertDataFile")

    log.info(f"Using '{varfile}' as channel map file")
    vf = VarFile(varfile)
    smap = vf.getSourceMap()

    log.info("Channel map created")
    log.info(f"Using {smap[timesource]} as master clock source")

    if fileFormat not in ["csv", "csv.gz", "xlsx", "parquet", "mat"]:
        log.error(f"Invalid output format: {fileFormat}!")
        raise Exception(f"Invalid output format requested: {fileFormat}")

    if output is None:
        if resample:
            cf = path.splitext(datfile)[0] + "-" + resample + "." + fileFormat
        else:
            cf = path.splitext(datfile)[0] + "." + fileFormat
    else:
        cf = output

    # The output file gets opened later (method depends on output format)
    # The check here is so we can bail early rather than waiting until after the messages are processed
    # Writing to the output could still fail later!
    dcf = path.dirname(cf) or "./"
    if (path.exists(cf) and not access(cf, W_OK)) or not (
        path.exists(dcf) and access(dcf, W_OK)
    ):
        log.error(f"Output path {cf} not writeable")
        return

    log.info("Beginning message processing")
    df = DatFile(datfile, pcs=timesource)
    df.addSourceMap(smap)
    data = df.asDataFrame(dropna=dropna, resample=resample)
    log.info("Message processing completed")

    log.info("Writing data out to file")

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


def N2KToTimeseries(n2kfile, output, fileFormat, dropna=False, pgns=None, epoch=False):
    from . import N2KMessages as N2K

    log = logging.getLogger("SELKIELogger.N2KToTimeseries")

    if fileFormat not in ["csv", "csv.gz", "xlsx", "parquet", "mat"]:
        log.error(f"Invalid output format: {fileFormat}!")
        raise Exception(f"Invalid output format requested: {fileFormat}")

    if output is None:
        cf = path.splitext(n2kfile)[0] + "." + fileFormat
    else:
        cf = output

    # As above, this is an attempt to detect problems early
    # Writing to the output could still fail later!
    dcf = path.dirname(cf) or "./"
    if (path.exists(cf) and not access(cf, W_OK)) or not (
        path.exists(dcf) and access(dcf, W_OK)
    ):
        log.error(f"Output path {cf} not writeable")
        return

    log.info("Beginning message processing")
    with open(n2kfile, "rb") as file:
        data = N2K.Timeseries(N2K.ACTN2KMessage.yieldFromFile(file))

    if dropna:
        dc = data.columns[data.isna().all()]
        log.info(f"Dropping empty columns: [{', '.join(dc)}]")
        data.drop(dc, axis=1, inplace=True)

    if epoch:
        log.info("Mapping timestamps to real times")
        data["Timestamp"] = data["Timestamp"].fillna(method="ffill", inplace=False)
        dg = data.groupby("Timestamp")
        for k, g in dg:
            data.loc[g.index, "Delta"] = (
                g["_N2KTS"] - g["_N2KTS"].head(1).values[0]
            ) / 1000.0
        data.index = data["Timestamp"] + pd.to_timedelta(data["Delta"], unit="s")
        data.drop("Delta", axis=1, inplace=True)
        data.drop("Timestamp", axis=1, inplace=True)

    data.drop("_N2KTS", axis=1, inplace=True)

    log.info("Message processing completed")

    log.info("Writing data out to file")

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
