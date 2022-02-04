import logging
import msgpack
import os
import pandas as pd
import numpy as np

from .SLMessages import IDs, SLMessage, SLMessageSink
log = logging.getLogger(__name__)
class VarFile:
    """! Represent a channel mapping (.var) file, caching information as necessary """
    def __init__(self, filename):
        self._fn = filename
        self._sm = None

    def getSourceMap(self, force=False):
        if self._sm and not force:
            log.debug("Returning cached SourceMap")
            return self._sm

        file = open(self._fn, "rb")
        unpacker = msgpack.Unpacker(file, unicode_errors='ignore')
        out = SLMessageSink(msglogger=log.getChild("VarFile"))

        # Seed the map with default names for the data source and converter (us)
        out.Process(SLMessage(0,0, "Logger").pack())
        out.Process(SLMessage(1,0, "SLPython").pack())
        for msg in unpacker:
            log.debug(msg)
            msg = out.Process(msg, output="raw")
        self._sm = out.SourceMap()
        return self._sm

    def printSourceMap(self, fancy=True):
        try:
            from rich.console import Console
            from rich.table import Table
        except ImportError:
            fancy = False

        if not fancy:
            print("Source          \tChannels")
            for src in self._sm:
                print(f"0x{src:02x} - {self._sm.GetSourceName(src):16s}{list(self._sm[src])}")
            return

        # We can be fancy!
        t = Table(show_header=True, header_style="bold")
        t.add_column("Source", style="dim", width=6, justify="center")
        t.add_column("Source Name")
        t.add_column("Channel Names")
        for src in self._sm:
            chanID = 0
            t.add_row(f"0x{src:02x}", self._sm.GetSourceName(src), str(list(self._sm[src])))
        Console().print(t)

class DatFile:
    def __init__(self, filename, pcs=IDs.SLSOURCE_TIMER):
        self._fn = filename
        self._fields = None
        self._pcs = pcs
        self._sm = None
        self._records = None

    def addSourceMap(self, sm):
        if self._sm:
            log.warning("Overriding existing source map")
        self._sm = sm

    @staticmethod
    def tryParse(value):
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
        if self._fields and not force:
            log.debug("Returning cached converters")
            return self._fields

        simpleSources  = [x for x in range(IDs.SLSOURCE_I2C, IDs.SLSOURCE_I2C + 0x10)]
        simpleSources += [x for x in range(IDs.SLSOURCE_MP, IDs.SLSOURCE_MP + 0x10)]
        simpleSources += [x for x in range(IDs.SLSOURCE_ADC, IDs.SLSOURCE_ADC + 0x10)]

        fields = {}
        for src in self._sm:
            fields[src] = {}
            if src == self._pcs:
                if includeTS:
                    fields[src][0x02] = [[f"Timestamp:0x{self._pcs:02x}"], [lambda x: x.Data]]
                cid = 0
                for chan in list(self._sm[src]):
                    if cid > IDs.SLCHAN_TSTAMP and chan != "":
                        fields[src][cid] = [[f"{chan}:0x{src:02x}"], [lambda x: x.Data]]
                    cid += 1
            elif src in range(IDs.SLSOURCE_GPS, IDs.SLSOURCE_GPS + 0x10):
                cid = 0
                for chan in list(self._sm[src]):
                    if cid == IDs.SLCHAN_TSTAMP:
                        fields[src][cid] = [[f"Timestamp:0x{src:02x}"], [lambda x: x.Data]]
                        cid += 1
                    elif cid == 4:
                        # Position
                        fields[src][cid] = [
                                [f"Longitude:0x{src:02x}", f"Latitude:0x{src:02x}", f"Height:0c{src:02x}", f"HAcc:0x{src:02x}", f"VAcc:0x{src:02x}"],
                                [lambda x: x.Data[0], lambda x: x.Data[1], lambda x: x.Data[2], lambda x: x.Data[4], lambda x: x.Data[5]]
                                ]
                        cid += 1
                    elif cid == 5:
                        # Velocity
                        fields[src][cid] = [
                                [f"Velocity_N:0x{src:02x}", f"Velocity_E:0x{src:02x}", f"Velocity_D:0c{src:02x}", f"SpeecAcc:0x{src:02x}", f"Heading:0x{src:02x}", f"HeadAcc:0x{src:02x}"],
                                [lambda x: x.Data[0], lambda x: x.Data[1], lambda x: x.Data[2], lambda x: x.Data[5], lambda x: x.Data[4], lambda x: x.Data[6]]
                                ]
                        cid += 1
                    elif cid == 6:
                        # Date/Time
                        fields[src][cid] = [
                                [f"Date:0x{src:02x}", f"Time:0x{src:02x}", f"DTAcc:0x{src:02x}"],
                                [lambda x: f"{x.Data[0]:04.0f}-{x.Data[1]:02.0f}-{x.Data[2]:02.0f}", lambda x: f"{x.Data[3]:02.0f}:{x.Data[4]:02.0f}:{x.Data[5]:02.0f}.{x.Data[6]:06.0f}", lambda x: f"{x.Data[7]:09.0f}"]
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
                        fields[src][cid] = [[f"Timestamp:0x{src:02x}"], [lambda x: x.Data]]
                        cid += 1
                    else:
                        fields[src][cid] =[[f"{chan}:0x{src:02x}"], [lambda x: self.tryParse(x.Data)]]
                        cid += 1
            elif src in simpleSources:
                cid = 0
                for chan in list(self._sm[src]):
                    if cid in [IDs.SLCHAN_NAME, IDs.SLCHAN_MAP]:#, IDs.SLCHAN_RAW]:
                        cid += 1
                        continue
                    elif chan == "":
                        cid += 1
                        continue
                    elif cid == IDs.SLCHAN_TSTAMP:
                        fields[src][cid] = [[f"Timestamp:0x{src:02x}"], [lambda x: x.Data]]
                        cid += 1
                    else:
                        fields[src][cid] = [[f"{chan}:0x{src:02x}"], [lambda x: x.Data]]
                        cid += 1
            else:
                if src >= 0x02:
                    log.info(f"No conversion routine known for source 0x{src:02x} ({self._sm[src]})")
        self._fields = fields
        return self._fields

    def buildRecord(self, msgStack):
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

    def processMessages(self, includeTS=False, force=False, chunkSize=100000):
        if self._records and not force:
            return self._records

        self._records = []
        datFile = open(self._fn, "rb")
        unpacker = msgpack.Unpacker(datFile, unicode_errors='ignore')
        sink = SLMessageSink(msglogger=log.getChild("Data"))

        sink.Process(SLMessage(0,0, "Logger").pack())
        sink.Process(SLMessage(1,0, "SLPython").pack())

        fields = self.prepConverters(includeTS=includeTS, force=force)
        log.debug(fields)

        stack = []
        currentTime = 0
        nextTime = 0
        count = 0
        for msg in unpacker:
            msg = sink.Process(msg, output="raw", allMessages=True)
            # log.debug(msg)
            if msg is None:
                continue

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
        log.debug(f"Out of data - {len(stack)} messages abandoned beyond last timestanp")

    def yieldDataFrame(self, dropna=False, resample=None):
        count = 0
        for chunk in self.processMessages():
            ndf = pd.DataFrame(data=[x[1] for x in chunk], index=[x[0] for x in chunk])
            count += len(ndf)
            if resample:
                ndf.index = pd.to_timedelta(ndf.index.values, unit='ms')
                ndf = ndf.resample(resample).mean()

                # Double 'astype' here to ensure we get an integer representing milliseconds back
                ndf.index = [x.astype('m8[ms]').astype(int) for x in ndf.index.values]

            for x in ndf.columns:
                if pd.api.types.is_numeric_dtype(ndf[x].dtype):
                    ndf[x] = ndf[x].astype(pd.SparseDtype(ndf[x].dtype, np.nan))
            if dropna:
                ndf.dropna(how='all', inplace=True)
            ndf.index.name="Timestamp"
            yield ndf

    def asDataFrame(self, dropna=False, resample=None):
        df = None
        for ndf in self.yieldDataFrame(dropna, resample):
            count = len(ndf)
            if df is None:
                df = ndf
            else:
                df = pd.concat([df, ndf], copy=False)
                del ndf
            log.debug(f"{count} steps processed ({pd.to_timedelta((df.index.max() - df.index.min()), unit='ms')})")

        df.index.name='Timestamp'
        return df

class StateFile:
    """! Represent a channel mapping (.var) file, caching information as necessary """
    def __init__(self, filename):
        self._fn = filename
        self._sm = None
        self._ts = 0
        self._stats = None

    def parse(self):
        with open(self._fn) as sf:
            self._ts = int(sf.readline())
            cols = ["Source", "Channel", "Count", "Last"]
            self._stats = pd.read_csv(sf, header=None, names = cols, index_col=["Source","Channel"], converters = {x: lambda z: int(z, base=0) for x in cols})
        self._stats["SecondsAgo"] = (self._stats["Last"] - self._ts) / 1000
        self._stats["DateTime"] = self._stats["Last"].apply(self.to_clocktime).apply(lambda x: x.strftime("%Y-%m-%d %H:%M:%S"))
        return self._stats

    def timestamp(self):
        return self._ts

    def to_clocktime(self, timestamp):
        if self._ts is None:
            return None
        mtime = os.stat(self._fn).st_mtime
        delta = (mtime - self._ts/1000)
        return pd.to_datetime(timestamp/1000 + delta, unit='s')
