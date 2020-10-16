#!/usr/bin/python3
import logging
import msgpack
from ..SLMessages import SLMessageSink

def process_arguments():
    import argparse
    options = argparse.ArgumentParser(description="Read messages from SELKIE Logger or compatible devices and parse details to screen", epilog="Created as part of the SELKIE project")
    options.add_argument("file",  help="Input file name")
    options.add_argument("-v, --verbose", dest="verbose", action="store_true", default=False, help="Include timing and source/channel information messages")
    return options.parse_args()

def processMessages(up, out, allM): 
    for msg in up:
        msg = out.Process(msg, output="string", allMessages=allM)
        if msg:
            print(msg)

def SLDump():
    logging.basicConfig(level=logging.INFO)
    args = process_arguments()
    inFile = open(args.file, "rb")
    unpacker = msgpack.Unpacker(inFile, unicode_errors='ignore')
    out = SLMessageSink(msglogger=logging.getLogger("Messages"))
    processMessages(unpacker, out, args.verbose)
    inFile.close()

if __name__ == "__main__":
    SLDump()
