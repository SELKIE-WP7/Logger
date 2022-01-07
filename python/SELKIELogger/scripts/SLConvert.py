#!/usr/bin/python3
import msgpack
import pandas as pd

from os import path
from . import log
from ..SLMessages import SLMessage, SLMessageSink, IDs

def process_arguments():
    import argparse
    options = argparse.ArgumentParser(description="Read messages from SELKIE Logger or compatible output file and convert to other data formats", epilog="Created as part of the SELKIE project")
    options.add_argument("file", metavar="DATFILE",  help="Main data file name")
    options.add_argument("-v", "--verbose", action="count", default=0, help="Increase output verbosity")
    options.add_argument("-c", "--varfile", default=None, help="Path to channel mapping file (.var file). Default: Auto detect or fall back to details in data file (slow)")
    options.add_argument("-o", "--output", metavar="OUTFILE", default=None, help="Output file name")
    return options.parse_args()

def getSourceMap(filename):
    file = open(filename, "rb")
    unpacker = msgpack.Unpacker(file, unicode_errors='ignore')
    out = SLMessageSink(msglogger=log.getChild("ChannelMap"))

    # Seed the map with default names for the data source and converter (us)
    out.Process(SLMessage(0,0, "Logger").pack())
    out.Process(SLMessage(1,0, "SLConvert").pack())
    for msg in unpacker:
        log.debug(msg)
        msg = out.Process(msg, output="raw")
    return (out.SourceMap())

def printSourceMap(smap, fancy=True):
    try:
        from rich.console import Console
        from rich.table import Table
    except ImportError:
        fancy = False

    if not fancy:
        print("Source          \tChannels")
        for src in smap:
            print(f"0x{src:02x} - {smap.GetSourceName(src):16s}{list(smap[src])}")
        return

    # We can be fancy!

    t = Table(show_header=True, header_style="bold")
    t.add_column("Source", style="dim", width=6, justify="center")
    t.add_column("Source Name")
    t.add_column("Channel Names")
    for src in smap:
        chanID = 0
        t.add_row(f"0x{src:02x}", smap.GetSourceName(src), str(list(smap[src])))
    Console().print(t)

def mapToFields(srcMap, primaryClockSource):
    fields = {}
    for src in srcMap:
        fields[src] = {}
        if src == primaryClockSource:
            # fields[src][0x02] = [["Timestamp"], [lambda x: x.Data]]
            cid = 0
            for chan in list(srcMap[src]):
                if cid > IDs.SLCHAN_TSTAMP and chan != "":
                    fields[src][cid] = [[f"{chan}:0x{src:02x}"], [lambda x: x.Data]]
                cid += 1
        elif src >= IDs.SLSOURCE_GPS and src < (IDs.SLSOURCE_GPS + 0x10):
            cid = 0
            for chan in list(srcMap[src]):
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
                            [lambda x: f"{x.Data[0]:04d}-{x.Data[1]:02d}-{x.Data[2]:02d}", lambda x: f"{x.Data[3]:02d}:{x.Data[4]:02d}:{x.Data[5]:02d}.{x.Data[6]:06d}", lambda x: f"{x.Data[7]:09d}"]
                            ]
                    cid += 1
                elif srcMap[src][chan] == "":
                    cid += 1
                    continue
                else:
                    fields[src][cid] = [[f"{chan}:0x{src:02x}"], [lambda x: x.Data]]
                    cid += 1
        elif (src >= IDs.SLSOURCE_I2C and src < (IDs.SLSOURCE_I2C + 0x10)):
            cid = 0
            for chan in list(srcMap[src]):
                if cid in [IDs.SLCHAN_NAME, IDs.SLCHAN_MAP, IDs.SLCHAN_RAW]:
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
        elif (src >= IDs.SLSOURCE_IMU and src < (IDs.SLSOURCE_IMU + 0x10)):
            cid = 0
            for chan in list(srcMap[src]):
                if cid in [IDs.SLCHAN_NAME, IDs.SLCHAN_MAP, IDs.SLCHAN_RAW]:
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
        elif (src >= IDs.SLSOURCE_ADC and src < (IDs.SLSOURCE_ADC + 0x10)):
            cid = 0
            for chan in list(srcMap[src]):
                if cid in [IDs.SLCHAN_NAME, IDs.SLCHAN_MAP, IDs.SLCHAN_RAW]:
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
            log.info(f"No conversion routine known for source {src} ({srcMap[src]})")
    return fields

def recordToDataFrame(fields, primaryClockSource, timeStamp, msgStack):
    record = {}
    for sid, channels in fields.items():
        for cid, converter in channels.items():
            for s in converter[0]:
                record[s] = None

    for m in msgStack:
        try:
            converter = fields[m.SourceID][m.ChannelID]
        except KeyError:
            continue
        ls = converter[0]
        dat = [d(m) for d in converter[1]]
        for x in range(len(ls)):
            record[ls[x]] = dat[x]

    #return pd.DataFrame(record, index=[timeStamp])
    return (timeStamp, record)

def processMessages(datFile, srcMap):
    unpacker = msgpack.Unpacker(datFile, unicode_errors='ignore')
    sink = SLMessageSink(msglogger=log.getChild("Data"))

    sink.Process(SLMessage(0,0, "Logger").pack())
    sink.Process(SLMessage(1,0, "SLConvert").pack())

    primaryClockSource = IDs.SLSOURCE_TIMER

    fields = mapToFields(srcMap, primaryClockSource)
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

        if msg.SourceID == primaryClockSource and msg.ChannelID == IDs.SLCHAN_TSTAMP:
            nextTime = msg.Data
##          log.debug(f"New timestep: {nextTime} (from {currentTime}, delta {nextTime - currentTime})")
##         // If we accumulate too many messages (bad clock choice?), force a record to be output
##         if (currMsg >= (ctsLimit - 1) && (timestep == nextstep)) {
##             log_warning(&state, "Too many messages processed during %d. Forcing output.", timestep);
##             nextstep++;
##         }
        if nextTime != currentTime:
            tsdf = recordToDataFrame(fields, primaryClockSource, nextTime, stack)
            stack.clear()
            currentTime = nextTime
            yield tsdf
            continue

        stack.extend([msg])

    # Out of messages
    log.debug(f"Out of data - {len(stack)} messages abandoned beyond last timestanp")


def SLConvert():
    args = process_arguments()
    if args.verbose > 1:
        log.setLevel(log.DEBUG)
    elif args.verbose > 0:
        log.setLevel(log.INFO)

    log.debug(f"Log level set to {log.getLevelName(log.getEffectiveLevel())}")

    log.info(f"Using '{args.file}' as main data file")

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

    smap = getSourceMap(args.varfile)
    printSourceMap(smap)

    log.info("Channel map created")

    log.info("Beginning message processing")
    df = []
    count = 0
    with open(args.file, "rb") as inFile:
        for ts in processMessages(inFile, smap):
            df.append(ts)
            count += 1
            if (len(df) % 10000) == 0:
                # delta = ((df[1].index.values - df[0].index.values).mean()) * 1E-3
                delta = ((df[1][0] - df[0][0]) * 1E-3)
                log.info(f"{count} records processed ({pd.to_timedelta(count*delta, unit='s')})")
    log.info(f"Message processing completed ({count} records)")

    log.info("Consolidating data")
    index = [x[0] for x in df]
    data = [x[1] for x in df]
    allData = pd.DataFrame(data=data, index=index)
    allData.index.name = "Timestamp"
    log.info("Writing data out to file")
    if args.output is None:
        cf = path.splitext(args.file)[0] + '.csv'
    else:
        cf = args.output
    allData.to_csv(cf)
    log.info(f"Data output to {cf}")


if __name__ == "__main__":
    SLConvert()
