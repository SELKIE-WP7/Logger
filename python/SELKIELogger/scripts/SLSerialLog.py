import serial

from serial.tools.miniterm import ask_for_port


def log_to_file(port, output, baud=115200, timeout=1):
    s = serial.Serial(port, baud, timeout=timeout)

    with open(output, "wb") as out:
        print("Press CTRL+C to exit")
        try:
            while True:
                data = s.read(s.in_waiting or 1)
                if data:
                    out.write(data)
        except KeyboardInterrupt:
            pass


def SLSerialLog():
    import argparse

    options = argparse.ArgumentParser(
        description="Dump serial data to file",
        epilog="Created as part of the SELKIE project",
    )
    options.add_argument(
        "-o", "--output", metavar="FILE", help="Output file name", default="serial.dat"
    )
    options.add_argument(
        "-p", "--port", metavar="PORT", help="Serial port name", default=None
    )
    options.add_argument(
        "-b", "--baud", metavar="BAUD", help="Serial port data rate", default=115200
    )
    options.add_argument(
        "-t",
        "--timeout",
        metavar="SECONDS",
        help="Serial port read timeout, in seconds",
        default=1,
    )

    args = options.parse_args()

    if args.port is None:
        port = ask_for_port()
    else:
        port = args.port

    return log_to_file(port, args.output, args.baud, args.timeout)


if __name__ == "__main__":
    SLSerialLog()
