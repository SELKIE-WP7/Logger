#!/usr/bin/python3
import msgpack
from . import log
from ..SLMessages import SLMessageSink

def process_arguments():
    import argparse
    options = argparse.ArgumentParser(description="Read messages from SELKIE Logger or compatible devices and parse details to screen", epilog="Created as part of the SELKIE project")
    options.add_argument("file",  help="Input file name")
    options.add_argument("-v", "--verbose", dest="verbose", action="store_true", default=False, help="Include timing and source/channel information messages")
    options.add_argument("-s", "--source-map", dest="map", action="store_true", default=False, help="Dump source map")
    return options.parse_args()

def processMessages(up, out, allM): 
    for msg in up:
        msg = out.Process(msg, output="string", allMessages=allM)
        if msg:
            print(msg)

def SLDump():
    args = process_arguments()
    inFile = open(args.file, "rb")
    unpacker = msgpack.Unpacker(inFile, unicode_errors='ignore')
    out = SLMessageSink(msglogger=log)
    processMessages(unpacker, out, args.verbose)
    inFile.close()

    if args.map:
        sm = out.SourceMap()
        print("Source          \tChannels")
        for src in sm:
            print(f"0x{src:02x} - {sm.GetSourceName(src):16s}{list(sm[src])}")

if __name__ == "__main__":
    SLDump()
