#!/usr/bin/env python3
import msgpack
from SELKIELogger.scripts import log
from SELKIELogger.SLMessages import SLMessageSink


def process_arguments():
    import argparse

    options = argparse.ArgumentParser(
        description="Read messages from SELKIE Logger data file and classify unknown/raw UBX and NMEA messages found",
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
    return options.parse_args()


class ProcessedRecords:
    __slots__ = ["classes", "processed", "SourceMap"]

    def __init__(self):
        SourceTypes = ["GPS", "NMEA", "MP"]
        self.classes = {x: {} for x in SourceTypes}
        self.processed = {x: 0 for x in SourceTypes}
        self.SourceMap = None


def classify_messages(file):
    unpacker = msgpack.Unpacker(file, unicode_errors="ignore")
    out = SLMessageSink(msglogger=log)

    tots = ProcessedRecords()

    # Dict of Dicts gpsSeen[class][msg] = count and similar for NMEA
    for msg in unpacker:
        msg = out.Process(msg, output="raw")
        if msg is None:
            continue
        tots.processed["MP"] += 1
        if msg.SourceID in tots.classes["MP"]:
            if msg.ChannelID in tots.classes["MP"][msg.SourceID]:
                tots.classes["MP"][msg.SourceID][msg.ChannelID] += 1
            else:
                tots.classes["MP"][msg.SourceID][msg.ChannelID] = 1
        else:
            tots.classes["MP"][msg.SourceID] = {}
            tots.classes["MP"][msg.SourceID][msg.ChannelID] = 1

        if msg.SourceID >= 0x10 and msg.SourceID < 0x20:
            # Assume GPS
            if msg.ChannelID != 0x03:
                # We're only looking at raw messages on Ch 3
                if not msg.ChannelID in [0, 1, 2]:
                    # But we will count messages that have been converted to other types
                    tots.processed["GPS"] = tots.processed["GPS"] + 1
                continue
            header = int.from_bytes(msg.Data[0:2], byteorder="big", signed=False)
            ubxClass = msg.Data[2]
            ubxMsg = msg.Data[3]

            if ubxClass in tots.classes["GPS"]:
                if ubxMsg in tots.classes["GPS"][ubxClass]:
                    tots.classes["GPS"][ubxClass][ubxMsg] = (
                        tots.classes["GPS"][ubxClass][ubxMsg] + 1
                    )
                else:
                    tots.classes["GPS"][ubxClass][ubxMsg] = 1
            else:
                tots.classes["GPS"][ubxClass] = {}
                tots.classes["GPS"][ubxClass][ubxMsg] = 1

        elif msg.SourceID >= 0x30 and msg.SourceID < 0x40:
            # Assume NMEA
            if msg.ChannelID != 0x03:
                if not msg.ChannelID in [0, 1, 2]:
                    tots.processed["NMEA"] = tots.processed["NMEA"] + 1
                continue
            nmeaHead = msg.Data[0:10].decode("ascii", "replace")
            if nmeaHead[1] == "P":
                talker = nmeaHead[1:5]
                message = nmeaHead[5:8]
            else:
                talker = nmeaHead[1:3]
                message = nmeaHead[3:6]

            if talker in tots.classes["NMEA"]:
                if message in tots.classes["NMEA"][talker]:
                    tots.classes["NMEA"][talker][message] = (
                        tots.classes["NMEA"][talker][message] + 1
                    )
                else:
                    tots.classes["NMEA"][talker][message] = 1
            else:
                tots.classes["NMEA"][talker] = {}
                tots.classes["NMEA"][talker][message] = 1

    tots.SourceMap = out.SourceMap()
    return tots


def print_classifications(res):
    print("======= Messages Seen =======")
    print(f"Total processed:\t{res.processed['MP']:d}")
    print(f"By Source and Type:")
    for ms in res.classes["MP"]:
        for mt in res.classes["MP"][ms]:
            print(f"\t0x{ms:02x}\t0x{mt:02x}\t{res.classes['MP'][ms][mt]:d}")

    print("\n\n===== GPS Messages Seen =====")
    print(f"Converted:\t\t{res.processed['GPS']:d}")
    print(f"Not Converted:")
    for uC in res.classes["GPS"]:
        for uM in res.classes["GPS"][uC]:
            print(f"\t0x{uC:02x}\t0x{uM:02x}\t{res.classes['GPS'][uC][uM]:d}")
    print("\n\n===== NMEA Messages Seen ====")
    print(f"Converted:\t\t{res.processed['NMEA']:d}")
    print(f"Not Converted:")
    for t in res.classes["NMEA"]:
        for m in res.classes["NMEA"][t]:
            print(f"\t{t:3s}\t{m:3s}\t{res.classes['NMEA'][t][m]:d}")


def SLClassify():
    args = process_arguments()
    inFile = open(args.file, "rb")
    res = classify_messages(inFile)
    inFile.close()
    print_classifications(res)

    if args.verbose:
        print("\n\n==== Sources and channels ===")
        for x in res.SourceMap:
            print(f"\t0x{x:02x}: {res.SourceMap[x].name}")

            ix = 0
            for y in res.SourceMap[x]:
                print(f"\t\t0x{ix:02x}: {y}")
                ix += 1


if __name__ == "__main__":
    SLClassify()
