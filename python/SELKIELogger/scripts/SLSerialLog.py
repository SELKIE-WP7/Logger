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
