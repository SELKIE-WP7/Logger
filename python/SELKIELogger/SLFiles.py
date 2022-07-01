import logging
import msgpack
import os
import pandas as pd
import numpy as np

from numbers import Number
from .SLMessages import IDs, SLMessage, SLMessageSink

## @file

## File wide logger instance
log = logging.getLogger(__name__)


class VarFile:
    """! Represent a channel mapping (.var) file, caching information as necessary"""

    def __init__(self, filename):
        """!
        Create VarFile instance. Does not open or parse file.
        @param filename File name and path
        """
        ## File name and path
        self._fn = filename
        ## Source/Channel Map
        self._sm = None

    def getSourceMap(self, force=False):
        """!
        Get source/channel map from file, or return from cache if available.
        @param force Read file again, even if source map exists
        @returns SLSourceMap instance
        """
        if self._sm and not force:
            log.debug("Returning cached SourceMap")
            return self._sm

        file = open(self._fn, "rb")
        unpacker = msgpack.Unpacker(file, unicode_errors="ignore")
        out = SLMessageSink(msglogger=log.getChild("VarFile"))

        # Seed the map with default names for the data source and converter (us)
        out.Process(SLMessage(0, 0, "Logger").pack())
        out.Process(SLMessage(1, 0, "SLPython").pack())
        for msg in unpacker:
            log.log(5, msg)
            msg = out.Process(msg, output="raw")
        self._sm = out.SourceMap()
        return self._sm

    def printSourceMap(self, fancy=True):
        """!
        Print a source/channel map, optionally using the tools provided in the "rich" package to provide a prettier output.

        Falls back to standard print output if `fancy` is False or if the rich package is not installed.

        @param fancy Enable/disable use of rich features.
        @returns None
        """
        try:
            from rich.console import Console
            from rich.table import Table
        except ImportError:
            fancy = False

        if not fancy:
            print("Source          \tChannels")
            for src in self._sm:
                print(
                    f"0x{src:02x} - {self._sm.GetSourceName(src):16s}{list(self._sm[src])}"
                )
            return

        # We can be fancy!
        t = Table(show_header=True, header_style="bold")
        t.add_column("Source", style="dim", width=6, justify="center")
        t.add_column("Source Name")
        t.add_column("Channel Names")
        for src in self._sm:
            chanID = 0
            t.add_row(
                f"0x{src:02x}", self._sm.GetSourceName(src), str(list(self._sm[src]))
            )
        Console().print(t)


class DatFile:
    """!
    Represent a SELKIELogger data file and associated common operations.
    """

    def __init__(self, filename, pcs=IDs.SLSOURCE_TIMER):
        """!
        Create DatFile instance. Does not open or parse file.
        @param filename File name and path
        @param pcs Primary Clock Source (Source ID)
        """

        ## File name and path
        self._fn = filename
        ## Cached conversion functions
        self._fields = None
        ## Primary Clock Source ID
        if isinstance(pcs, Number):
            self._pcs = int(pcs)
        else:
            self._pcs = int(pcs, 0)
        ## Source/Channel Map
        self._sm = None
        ## File data records (once parsed)
        self._records = None

    def addSourceMap(self, sm):
        """!
        Associate an SLChannelMap instance with this file. Used to map source
        and channel IDs to names.
        @param sm SLChannelMap (see VarFile.getSourceMap)
        @returns None
        """
        if self._sm:
            log.warning("Overriding existing source map")
        self._sm = sm

    @staticmethod
    def tryParse(value):
        """!
        Attempt to convert value to float. If unsuccessful, parse as JSON and attempt to return a float from either a) the sole value within the object or b) the data associated with a key named "value".
        @param value Input value
        @returns Floating point value or np.nan on failure
        """
        try:
            x = float(value)
            return x
        except ValueError:
            pass

        try:
            import json

            x = json.loads(value, parse_int=float, parse_constant=float)
            if len(x) == 1:
                return float(x.popitem()[1])
            elif "value" in x:
                return float(x["value"])
        except:
            return np.nan

    def prepConverters(self, force=False, includeTS=False):
        """!
        Generate and cache functions to convert each channel into defined
        fields and corresponding field names suitable for creating a DataFrame
        later.

        Names and functions are returned as a collection keyed by source and
        channel. As each message may produce multiple message, names and
        functions are each returned as lists containing a minimum of one entry.

        ```.py
        fields = x.prepConverters()
        names, funcs = fields[source][channel]
        out = {}
        out[names[0]] = funcs[0](input)
        ```
        Where input is an input SLMessage

        @param force Regenerate fields rather than using cached values
        @param includeTS Retain timestamp field as data column as well as index
        @returns Collection of conversion functions and field names
        """
        if self._fields and not force:
            log.debug("Returning cached converters")
            return self._fields

        simpleSources = [x for x in range(IDs.SLSOURCE_I2C, IDs.SLSOURCE_I2C + 0x10)]
        simpleSources += [x for x in range(IDs.SLSOURCE_MP, IDs.SLSOURCE_MP + 0x10)]
        simpleSources += [x for x in range(IDs.SLSOURCE_ADC, IDs.SLSOURCE_ADC + 0x10)]
        simpleSources += [x for x in range(IDs.SLSOURCE_EXT, IDs.SLSOURCE_EXT + 0x07)]

        fields = {}
        for src in self._sm:
            fields[src] = {}
            if src == self._pcs:
                if includeTS:
                    fields[src][0x02] = [
                        [f"Timestamp:0x{self._pcs:02x}"],
                        [lambda x: x.Data],
                    ]
                cid = 0
                for chan in list(self._sm[src]):
                    if cid > IDs.SLCHAN_TSTAMP and chan != "":
                        fields[src][cid] = [[f"{chan}:0x{src:02x}"], [lambda x: x.Data]]
                    cid += 1
            elif src in range(IDs.SLSOURCE_GPS, IDs.SLSOURCE_GPS + 0x10):
                cid = 0
                for chan in list(self._sm[src]):
                    if cid == IDs.SLCHAN_TSTAMP:
                        fields[src][cid] = [
                            [f"Timestamp:0x{src:02x}"],
                            [lambda x: x.Data],
                        ]
                        cid += 1
                    elif cid in [IDs.SLCHAN_NAME, IDs.SLCHAN_MAP, IDs.SLCHAN_RAW]:
                        # Don't include in frame outputs
                        cid += 1
                        continue
                    elif cid == 4:
                        # Position
                        fields[src][cid] = [
                            [
                                f"Longitude:0x{src:02x}",
                                f"Latitude:0x{src:02x}",
                                f"Height:0x{src:02x}",
                                f"HAcc:0x{src:02x}",
                                f"VAcc:0x{src:02x}",
                            ],
                            [
                                lambda x: x.Data[0],
                                lambda x: x.Data[1],
                                lambda x: x.Data[2],
                                lambda x: x.Data[4],
                                lambda x: x.Data[5],
                            ],
                        ]
                        cid += 1
                    elif cid == 5:
                        # Velocity
                        fields[src][cid] = [
                            [
                                f"Velocity_N:0x{src:02x}",
                                f"Velocity_E:0x{src:02x}",
                                f"Velocity_D:0x{src:02x}",
                                f"SpeecAcc:0x{src:02x}",
                                f"Heading:0x{src:02x}",
                                f"HeadAcc:0x{src:02x}",
                            ],
                            [
                                lambda x: x.Data[0],
                                lambda x: x.Data[1],
                                lambda x: x.Data[2],
                                lambda x: x.Data[5],
                                lambda x: x.Data[4],
                                lambda x: x.Data[6],
                            ],
                        ]
                        cid += 1
                    elif cid == 6:
                        # Date/Time
                        fields[src][cid] = [
                            [
                                f"Date:0x{src:02x}",
                                f"Time:0x{src:02x}",
                                f"DTAcc:0x{src:02x}",
                            ],
                            [
                                lambda x: f"{x.Data[0]:04.0f}-{x.Data[1]:02.0f}-{x.Data[2]:02.0f}",
                                lambda x: f"{x.Data[3]:02.0f}:{x.Data[4]:02.0f}:{x.Data[5]:02.0f}.{x.Data[6]:06.0f}",
                                lambda x: f"{x.Data[7]:09.0f}",
                            ],
                        ]
                        cid += 1
                    elif self._sm[src][cid] == "":
                        cid += 1
                        continue
                    else:
                        fields[src][cid] = [[f"{chan}:0x{src:02x}"], [lambda x: x.Data]]
                        cid += 1
            elif src in range(IDs.SLSOURCE_MQTT, IDs.SLSOURCE_MQTT + 0x07):
                cid = 0
                for chan in list(self._sm[src]):
                    if cid in [IDs.SLCHAN_NAME, IDs.SLCHAN_MAP]:
                        cid += 1
                        continue
                    elif chan == "":
                        cid += 1
                        continue
                    elif cid == IDs.SLCHAN_TSTAMP:
                        fields[src][cid] = [
                            [f"Timestamp:0x{src:02x}"],
                            [lambda x: x.Data],
                        ]
                        cid += 1
                    else:
                        fields[src][cid] = [
                            [f"{chan}:0x{src:02x}"],
                            [lambda x: self.tryParse(x.Data)],
                        ]
                        cid += 1
            elif src in simpleSources:
                cid = 0
                for chan in list(self._sm[src]):
                    if cid in [IDs.SLCHAN_NAME, IDs.SLCHAN_MAP]:  # , IDs.SLCHAN_RAW]:
                        cid += 1
                        continue
                    elif chan == "" or chan == "-":
                        cid += 1
                        continue
                    elif chan == "Raw Data" or (
                        cid == IDs.SLCHAN_RAW and chan.lower().startswith("raw")
                    ):
                        cid += 1
                        continue
                    elif cid == IDs.SLCHAN_TSTAMP:
                        fields[src][cid] = [
                            [f"Timestamp:0x{src:02x}"],
                            [lambda x: x.Data],
                        ]
                        cid += 1
                    else:
                        fields[src][cid] = [[f"{chan}:0x{src:02x}"], [lambda x: x.Data]]
                        cid += 1
            else:
                if src >= 0x02:
                    log.info(
                        f"No conversion routine known for source 0x{src:02x} ({self._sm[src]})"
                    )
        self._fields = fields
        self._columnList = []
        for _, channels in self._fields.items():
            for _, c in channels.items():
                self._columnList.extend(c[0])

        return self._fields

    def buildRecord(self, msgStack):
        """!
        Convert a group (stack) of messages into a single record, keyed by field names.

        Field values default to None, and in the event that multiple values for
        a single field are received, the last value received will be stored.

        @param msgStack Group of messages received for a specific interval
        @returns Record/dictionary of values keyed by field name
        """
        record = {}
        for sid, channels in self._fields.items():
            for cid, converter in channels.items():
                for s in converter[0]:
                    record[s] = None

        for m in msgStack:
            try:
                converter = self._fields[m.SourceID][m.ChannelID]
            except KeyError:
                continue
            ls = converter[0]
            dat = [d(m) for d in converter[1]]
            for x in range(len(ls)):
                record[ls[x]] = dat[x]
        return record

    def messages(self, source=None, channel=None):
        """!
        Process data file and yield messages, optionally restricted to those
        matching a specific source and/or channel ID.

        * x.messages() - Yields all messages
        * x.messages(source=0x10) - Yields all messages from source 0x10 (GPS0)
        * x.messages(channel=0x03) - Yields all channel 3 (raw) messages from any source
        * x.messages(0x10, 0x03) - Yield all channel 3 messages from source 0x10

        @param source Optional: Source ID to match
        @param channel Optional: Channel ID to match
        @returns Yields messages in file order
        """
        datFile = open(self._fn, "rb")
        unpacker = msgpack.Unpacker(datFile, unicode_errors="ignore")
        sink = SLMessageSink(msglogger=log.getChild("Data"))

        sink.Process(SLMessage(0, 0, "Logger").pack())
        sink.Process(SLMessage(1, 0, "SLPython").pack())

        for msg in unpacker:
            msg = sink.Process(msg, output="raw", allMessages=True)
            if msg is None:
                continue

            if source and msg.SourceID != source:
                continue

            if channel and msg.ChannelID != channel:
                continue

            yield msg

        datFile.close()

    def processMessages(self, includeTS=False, force=False, chunkSize=100000):
        """!
        Process messages and return. Will yield data in chunks.
        @param includeTS Passed to prepConverters()
        @param force Passed to prepConverters()
        @param chunkSize Yield records after this many timestamps
        @returns List of tuples containing timestamp and dictionary of records
        """
        if self._records:
            log.error("Some records already cached - discarding")
            del self._records

        fields = self.prepConverters(includeTS=includeTS, force=force)
        log.debug(fields)
        log.debug(f"Primary clock source: 0x{self._pcs:02x} [{self._sm[self._pcs]}]")

        self._records = []
        stack = []
        currentTime = 0
        nextTime = 0
        count = 0
        for msg in self.messages():

            count += 1

            if msg.SourceID == self._pcs and msg.ChannelID == IDs.SLCHAN_TSTAMP:
                nextTime = msg.Data
            ##          log.debug(f"New timestep: {nextTime} (from {currentTime}, delta {nextTime - currentTime})")
            ##         // If we accumulate too many messages (bad clock choice?), force a record to be output
            ##         if (currMsg >= (ctsLimit - 1) && (timestep == nextstep)) {
            ##             log_warning(&state, "Too many messages processed during %d. Forcing output.", timestep);
            ##             nextstep++;
            ##         }
            if nextTime != currentTime:
                tsdf = self.buildRecord(stack)
                stack.clear()
                currentTime = nextTime
                self._records.extend([(currentTime, tsdf)])
                numTS = len(self._records)
                if (numTS % chunkSize) == 0:
                    yield self._records
                    self._records.clear()
                    tsdf.clear()
                continue

            stack.extend([msg])

        ## Out of messages, so yield remaining processes timesteps
        yield self._records

        # Out of messages and no more timestamps available
        log.debug(
            f"Out of data - {len(stack)} messages abandoned beyond last timestanp"
        )

    def yieldDataFrame(self, dropna=False, resample=None):
        """!
        Process file and yield results as dataframes that can be merged later.

        Optionally drop empty records and perform naive averaging over a given resample interval.
        @param dropna Drop rows consisting entirely of NaN/None vales
        @param resample Resampling interval
        @returns pandas.DataFrame representing a chunk of file data
        """
        count = 0
        for chunk in self.processMessages():
            ndf = pd.DataFrame(data=[x[1] for x in chunk], index=[x[0] for x in chunk])
            count += len(ndf)
            if resample:
                ndf.index = pd.to_timedelta(ndf.index.values, unit="ms")
                ndf = ndf.resample(resample).mean()

                # Double 'astype' here to ensure we get an integer representing milliseconds back
                ndf.index = [x.astype("m8[ms]").astype(int) for x in ndf.index.values]

            for x in ndf.columns:
                if pd.api.types.is_numeric_dtype(ndf[x].dtype):
                    ndf[x] = ndf[x].astype(pd.SparseDtype(ndf[x].dtype, np.nan))

            ndf = ndf.reindex(columns=self._columnList, copy=False)

            if dropna:
                ndf.dropna(how="all", inplace=True)
            ndf.index.name = "Timestamp"
            yield ndf

    def asDataFrame(self, dropna=False, resample=None):
        """!
        Wrapper around yieldDataFrame.
        Processes all records and merges them into a single frame.
        @param dropna Drop empty records. @see yieldDataFrame()
        @param resample Resampling interval. @see yieldDataFrame()
        @returns pandas.DataFrame containing file data
        """
        df = None
        for ndf in self.yieldDataFrame(dropna, resample):
            count = len(ndf)
            if df is None:
                df = ndf
            else:
                df = pd.concat([df, ndf], copy=False)
                del ndf
            log.info(
                f"{count} steps processed ({pd.to_timedelta((df.index.max() - df.index.min()), unit='ms')})"
            )

        df.index.name = "Timestamp"
        return df


class StateFile:
    """! Represent a logger state file, caching information as necessary"""

    def __init__(self, filename):
        """!
        Create new object. File is not opened or parsed until requested.
        @param filename Path to state file to be read
        """
        ## File name (and path, if required) for this instance
        self._fn = filename

        ## SourceMap
        self._sm = None
        ## Last known timestamp
        self._ts = None
        ## Channel mapping file / VarFile associated with this state file
        self._vf = None
        ## Source/Channel statistics
        self._stats = None

    def parse(self):
        """!
        Read file and extract data
        @returns Channel statistics (also stored in _stats)
        """
        with open(self._fn) as sf:
            self._ts = int(sf.readline())
            self._vf = VarFile(sf.readline().strip()).getSourceMap()
            cols = ["Source", "Channel", "Count", "Time", "Value"]
            self._stats = pd.read_csv(
                sf,
                header=None,
                names=cols,
                index_col=["Source", "Channel"],
                quotechar="'",
                converters={x: lambda z: int(z, base=0) for x in cols if x != "Value"},
            )
        self._stats["SecondsAgo"] = (self._stats["Time"] - self._ts) / 1000
        self._stats["DateTime"] = (
            self._stats["Time"]
            .apply(self.to_clocktime)
            .apply(lambda x: x.strftime("%Y-%m-%d %H:%M:%S"))
        )
        return self._stats

    def sources(self):
        """!
        Retrieve list of sources referenced in this state file, parsing file if necessary
        @returns Sorted set of source IDs
        """
        if self._stats is None:
            if self.parse() is None:
                return None

        return sorted(set(self._stats.index.get_level_values(0)))

    def channels(self, source):
        """!
        Extract all channels referenced in this state file for a specific source ID.

        Will parse the state file if required.

        @param source Source ID to extract
        @returns Sorted set of channel IDs
        """
        if self._stats is None:
            if self.parse() is None:
                return None
        return sorted(
            set(self._stats[(source, 0x00):(source, 0xFF)].index.get_level_values(1))
        )

    def last_source_message(self, source):
        """!
        Find most recent message received from a given source and convert to a
        clocktime value.

        Will parse the state file if required.

        @param source Source ID to check
        @returns clocktime, or None on error.
        """
        if self._stats is None:
            if self.parse() is None:
                return None

        try:
            times = self._stats.loc[(source, 0x00):(source, 0xFF)].Time
            return self.to_clocktime(times.max())
        except (KeyError, TypeError):
            return None

    def last_channel_message(self, source, channel):
        """!
        Find most recent message received from a channel (specified by source
        and channel IDs) and convert to a clocktime value.

        Will parse the state file if required.

        @param source Source ID to be checked
        @param channel Channel ID to be checked
        @returns clocktime, or None on error.
        """
        if self._stats is None:
            self.parse()
        try:
            return self.to_clocktime(self._stats.loc[(source, channel)].Time)
        except KeyError:
            return None

    def timestamp(self):
        """!
        Return latest timestamp from state file, parsing file if required.
        @returns Latest logger timestamp value (ms)
        """
        if self._ts is None:
            self.parse()
        return self._ts

    def to_clocktime(self, timestamp):
        """!
        Convert logger timestamp to real date/time

        Assumes that the most recent message was received at the file's
        modification time and uses this as an offset.

        @param timestamp Value to be converted
        @returns Pandas DateTime object
        """
        if timestamp is None:
            return None
        if self._ts is None:
            self.parse()
        mtime = os.stat(self._fn).st_mtime
        delta = mtime - self._ts / 1000
        return pd.to_datetime(timestamp / 1000 + delta, unit="s")
