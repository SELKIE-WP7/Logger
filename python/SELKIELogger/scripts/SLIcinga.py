from time import time
from sys import exit

from SELKIELogger.Specs import ChannelSpec
from SELKIELogger.SLFiles import StateFile


def process_arguments():
    import argparse

    options = argparse.ArgumentParser(
        description="Read a SELKIE Logger state file and output Icinga compatible status",
        epilog="Created as part of the SELKIE project",
    )
    options.add_argument("file", metavar="STATEFILE", help="State file name")
    options.add_argument(
        "-s",
        "--source",
        default=[],
        action="append",
        help="Monitor sources for message activity",
        type=lambda x: int(x, base=0),
    )
    options.add_argument(
        "-c",
        "--channel",
        default=[],
        action="append",
        help="Monitor specific channel for activity",
        type=ChannelSpec,
    )
    options.add_argument(
        "-t", "--timeout", default=1800, help="Inactivity alarm threshold", type=int
    )
    options.add_argument(
        "-w", "--warning", default=900, help="Inactivity warning threshold", type=int
    )

    return options.parse_args()


def SLIcinga():
    args = process_arguments()
    sf = StateFile(args.file)
    stats = sf.parse()

    exitStatus = 0

    def updateWarnings(x, y):
        if x > args.timeout:
            return 2
        elif x > args.warning:
            return 1
        elif x < 0:
            return -1
        return y

    # File Age (Convert from source relative stamp to posix time)
    lastMessage = sf.to_clocktime(sf.timestamp())
    fileAge = time() - lastMessage.timestamp()
    print(
        f"Most recent data: {lastMessage.strftime('%Y-%m-%d %H:%M:%S')}|age={int(fileAge)};{args.warning};{args.timeout};0;86400"
    )
    exitStatus = updateWarnings(fileAge, exitStatus)

    # Sources
    for s in args.source:
        lm = sf.last_source_message(s)
        try:
            lmd = int(10 * (time() - lm.timestamp())) / 10.0
            lms = lm.strftime("%Y-%m-%d %H:%M:%S")
        except:
            lmd = -1
            lms = "Not Found"
        name = sf._vf[s] if s in sf._vf else str(s)
        print(
            f"Last {name} message: {lms}|last{s}={lmd};{args.warning};{args.timeout};0;86400"
        )
        exitStatus = updateWarnings(lmd, exitStatus)

    # Channels
    for c in args.channel:
        lm = sf.last_channel_message(c.source, c.channel)
        try:
            lmd = int(10 * (time() - lm.timestamp())) / 10.0
            lms = lm.strftime("%Y-%m-%d %H:%M:%S")
        except:
            lmd = -1
            lms = "Not Found"

        if c.source in sf._vf:
            source = sf._vf[c.source]
            channel = sf._vf[c.source][c.channel]
        else:
            source = f"0x{c.source:02x}"
            chan = f"0x{c.channel:02x}"
        print(
            f"Last {source}/{channel} message: {lms}|last{c.source}_{c.channel}={lmd};{args.warning};{args.timeout};0;86400"
        )
        exitStatus = updateWarnings(lmd, exitStatus)

    exit(exitStatus)


if __name__ == "__main__":
    SLIcinga()
