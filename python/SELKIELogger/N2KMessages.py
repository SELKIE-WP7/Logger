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

from dataclasses import dataclass, field
from enum import IntEnum
from struct import pack, unpack

import logging
import time

N2K_TO_DEGREES = 0.0057295779513082332


class ACTChar(IntEnum):
    BEM = 0xA0
    N2K = 0x93
    EOT = 0x03
    SOT = 0x02
    ESC = 0x10


_PGNRegistry = {}


def pgnclass(number):
    """! Decorator: Registers a class to parse the specified PGN

    Each class is required to have a static fromMessage method that parses an
    ACTN2KMessage and returns a new instance of the class, or returns None on
    failure.
    """

    def pgnconverter(cl):
        if number in _PGNRegistry:
            raise KeyError(f"Converter already registered for PGN {number}")
        _PGNRegistry[number] = cl.fromMessage
        # Each PGN class must be a dataclass, so apply that here to save typing
        # it above every single definition below
        return dataclass(cl)

    return pgnconverter


def ConvertN2KMessage(msg):
    """! Convert ACTN2KMessage instance (or return None)

    Uses classes registered with @pgnclass to perform conversion
    """

    if msg.PGN in _PGNRegistry:
        return _PGNRegistry[msg.PGN](msg)
    return None


def SupportedPGNs():
    return [k for k in _PGNRegistry]


def Timeseries(msgsource, pgns=None):
    """! Convert stream of N2K messages into a timeseries

    msgsource is expected to be a generator yielding ACTN2KMessage instances
    Will return a pandas DataFrame indexed by message timestamp (which are
    assumed to be trusted)
    """

    if pgns is None:
        exclude = [60928]
        pgns = [x for x in SupportedPGNs() if not x in exclude]

    from pandas import to_datetime, DataFrame as df

    skipped = False
    messages = {}
    cgroup = 0
    ctimestamp = None
    for msg in msgsource:
        # Break after two consecutive failures to read messages
        if msg is None:
            if skipped:
                break
            skipped = True
            continue
        skipped = False
        if msg.PGN not in pgns:
            continue
        cm = ConvertN2KMessage(msg)
        if cm:
            row = {k: v for k, v in zip(cm.fieldnames(), cm.fieldvalues())}
            row["_N2KTS"] = msg.timestamp
            if msg.timestamp == ctimestamp:
                messages[cgroup].update(row)
            else:
                cgroup += 1
                ctimestamp = msg.timestamp
                messages[cgroup] = row
    dat = df.from_dict(messages, orient="index").sort_index()
    del messages
    timecols = [x for x in dat.columns if x.endswith("Timestamp")]
    for t in timecols:
        dat[t] = to_datetime(dat[t], unit="s")
    return dat


@dataclass
class ACTN2KMessage:
    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    length: int = 0
    PGN: int = 0
    datalen: int = 0
    data: bytearray = field(default_factory=bytearray)
    checksum: int = 0

    @staticmethod
    def findStart(data):
        try:
            return data.index(bytearray([ACTChar.ESC, ACTChar.SOT, ACTChar.N2K]))
        except ValueError:
            return None

    def findEnd(data):
        try:
            return data.index(bytearray([ACTChar.ESC, ACTChar.EOT]))
        except ValueError:
            return None

    @staticmethod
    def unpack(data):
        spos = ACTN2KMessage.findStart(data)
        if spos is None:
            logging.debug("No message found")
            return (None, 0)

        if (len(data) - spos) < 18:
            # Insufficient data available
            logging.debug("Buffer/data too small for any message")
            return (None, 0)

        data = data[spos:]
        if len(data) < data[3]:
            logging.debug("Buffer/data too small for claimed message size")
            # Insufficient data available
            return (None, 0)

        msg = ACTN2KMessage()
        msg.length = data[3]
        msg.priority = data[4]
        msg.PGN = data[5] + (data[6] * (2**8)) + (data[7] * (2**16))
        msg.dst = data[8]
        msg.src = data[9]
        msg.timestamp = (
            data[10]
            + (data[11] * (2**8))
            + (data[12] * (2**16))
            + (data[13] * (2**24))
        )
        msg.datalen = data[14]

        datapos = 15
        remaining = msg.datalen
        while (len(data) - datapos) > 3 and remaining > 0:
            c = data[datapos]
            datapos += 1
            remaining -= 1
            if c == ACTChar.ESC:
                nc = data[datapos]
                datapos += 1
                if nc == ACTChar.ESC:
                    msg.data.append(ACTChar.ESC)
                    continue
                elif nc == ACTChar.EOT:
                    logging.debug("Message terminated early")
                    return (None, spos + datapos)
                elif nc == ACTChar.SOT:
                    logging.debug("Unexpected message restart")
                    return (None, spos + datapos - 2)
                else:
                    logging.debug(
                        f"Invalid escape sequence found in input data (ESC + 0x{nc:02x})"
                    )
                    return (None, spos + datapos)
            else:
                msg.data.append(c)

        if remaining:
            logging.debug("Failed to read whole message?")

        msg.checksum = data[datapos]
        datapos += 1

        if not (ACTN2KMessage.findEnd(data) == datapos):
            logging.debug("End of message not in expected location")
        else:
            # Skip end of message bytes
            datapos += 2

        return (msg, spos + datapos)

    @staticmethod
    def yieldFromFile(file):
        x = file.read(1024)
        msg = ACTN2KMessage.unpack(x)
        x = x[msg[1] :]
        while x:
            yield msg[0]
            msg = ACTN2KMessage.unpack(x)
            x = x[msg[1] :]
            if len(x) < 100 or msg[1] == 0:
                d = file.read(1024 - len(x))
                if d:
                    x = x + d
                else:
                    break
        # Out of data, so process until unable to parse further
        while msg[1] > 0:
            yield msg[0]
            msg = ACTN2KMessage.unpack(x)
            x = x[msg[1] :]

        return msg[0]

    def calcChecksum(self):
        csum = ACTChar.N2K
        csum += self.length
        csum += self.priority
        for b in pack("<I", self.PGN)[0:3]:
            csum += b
        csum += self.dst
        csum += self.src
        for b in pack("<I", self.timestamp):
            csum += b
        csum += self.datalen
        for b in self.data:
            csum += b
        return (256 - csum) % 256

    def getUInt8(self, offset):
        return unpack("<B", self.data[offset : offset + 1])[0]

    def getInt8(self, offset):
        return unpack("<b", self.data[offset : offset + 1])[0]

    def getUInt16(self, offset):
        return unpack("<H", self.data[offset : offset + 2])[0]

    def getInt16(self, offset):
        return unpack("<h", self.data[offset : offset + 2])[0]

    def getUInt32(self, offset):
        return unpack("<L", self.data[offset : offset + 4])[0]

    def getInt32(self, offset):
        return unpack("<l", self.data[offset : offset + 4])[0]

    def getUInt64(self, offset):
        return unpack("<Q", self.data[offset : offset + 8])[0]

    def getInt64(self, offset):
        return unpack("<q", self.data[offset : offset + 8])[0]

    def getFloat(self, offset, size, signed=True):
        if size not in [8, 16, 32, 64]:
            return ValueError("Unsupported size requested")
        value = 0
        m = "get"
        if signed:
            m += "Int"
        else:
            m += "UInt"
        m += str(size)
        value = getattr(self, m)(offset)
        if (
            ((not signed) and value == ((2**size) - 1))
            or (signed and (value == -(2 ** (size - 1))))
            or (signed and (value == (2 ** (size))))
        ):
            return float("NaN")

        return float(value)


class N2KBearingRef(IntEnum):
    TRUE = 0
    MAGNETIC = 1
    INVALID = 2
    UNKNOWN = 3


class N2KWindDirs(IntEnum):
    NORTHRELATIVE = 0
    MAGNETIC = 1
    APPARENT = 2
    BOATRELATIVE = 3
    INVALID4 = 4
    INVALID5 = 5
    INVALID6 = 6
    UNKNOWN = 7


class N2KTemperatureSource(IntEnum):
    SEAWATER = 0
    EXTERNAL = 1
    INTERNAL = 2
    ENGINEROOM = 3
    CABIN = 4
    UNKNOWN = 5


class N2KHumiditySource(IntEnum):
    INTERNAL = 0
    EXTERNAL = 1
    UNKNOWN = 2
    UNAVAILABLE = 3


@pgnclass(60928)
class N2KPGN_60928:
    """N2K Address Claim"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    id: int = 0
    mfr: int = 0
    inst: int = 0
    fn: int = 0
    cls: int = 0
    cfg: bool = False
    ind: int = 0
    sys: int = 0

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 60928
        pgn = N2KPGN_60928()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.id = msg.getUInt32(0) & 0x1FFFFF
        pgn.mfr = msg.getUInt16(2) >> 5
        pgn.inst = msg.getUInt8(4)
        pgn.fn = msg.getUInt8(5)
        pgn.cls = msg.getUInt8(6) & 0xFE

        si = msg.getUInt8(7)
        pgn.cfg = (si & 0x80) == 0x80
        pgn.ind = (si & 0x70) >> 4
        pgn.sys = si & 0x0F
        return pgn

    @staticmethod
    def fieldnames():
        return [
            "ISO-ID",
            "Manufacturer",
            "Instance",
            "Function",
            "Class",
            "System",
            "Industry",
            "Configurable",
        ]

    def fieldvalues(s):
        return [s.id, s.mfr, s.inst, s.fn, s.cls, s.sys, s.ind, s.cfg]


@pgnclass(127250)
class N2KPGN_127250:
    """Heading"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    seq: int = 0
    heading: float = float("NaN")
    deviation: float = float("NaN")
    variation: float = float("NaN")
    reference: N2KBearingRef = N2KBearingRef.UNKNOWN

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 127250
        pgn = N2KPGN_127250()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.seq = msg.getUInt8(0)
        pgn.heading = msg.getFloat(1, 16, False) * N2K_TO_DEGREES
        pgn.deviation = msg.getFloat(3, 16, True) * N2K_TO_DEGREES
        pgn.variation = msg.getFloat(5, 16, True) * N2K_TO_DEGREES
        pgn.reference = N2KBearingRef(msg.getUInt8(7) & 0x03)
        return pgn

    @staticmethod
    def fieldnames():
        return ["Heading", "Deviation", "Variation", "Reference"]

    def fieldvalues(s):
        return [s.heading, s.deviation, s.variation, s.reference]


@pgnclass(127251)
class N2KPGN_127251:
    """Rate of turn"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    seq: int = 0
    rate: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 127251
        pgn = N2KPGN_127251()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.seq = msg.getUInt8(0)
        pgn.rate = msg.getFloat(1, 32, True) / 3200 * N2K_TO_DEGREES
        return pgn

    @staticmethod
    def fieldnames():
        return ["RateOfTurn"]

    def fieldvalues(s):
        return [s.rate]


@pgnclass(127257)
class N2KPGN_127257:
    """Orientation"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    seq: int = 0
    yaw: float = float("NaN")
    pitch: float = float("NaN")
    roll: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 127257
        pgn = N2KPGN_127257()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.seq = msg.getUInt8(0)
        pgn.yaw = msg.getFloat(1, 16, True) * N2K_TO_DEGREES
        pgn.pitch = msg.getFloat(3, 16, True) * N2K_TO_DEGREES
        pgn.roll = msg.getFloat(5, 16, True) * N2K_TO_DEGREES
        return pgn

    @staticmethod
    def fieldnames():
        return ["Pitch", "Roll", "Yaw"]

    def fieldvalues(s):
        return [s.pitch, s.roll, s.yaw]


@pgnclass(128267)
class N2KPGN_128267:
    """Water depth"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    seq: int = 0
    depth: float = float("NaN")
    offset: float = float("NaN")
    range: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 128267
        pgn = N2KPGN_128267()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.seq = msg.getUInt8(0)
        pgn.depth = msg.getFloat(1, 32) * 0.01
        pgn.offset = msg.getFloat(5, 16) * 0.01
        pgn.range = msg.getFloat(7, 8) * 10.0
        return pgn

    @staticmethod
    def fieldnames():
        return ["Depth", "DepthOffset", "DepthRange"]

    def fieldvalues(s):
        return [s.depth, s.offset, s.range]


@pgnclass(129025)
class N2KPGN_129025:
    """GPS/GNSS Position"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    latitude: float = float("NaN")
    longitude: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 129025
        pgn = N2KPGN_129025()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.latitude = msg.getFloat(0, 32) * 1e-7
        pgn.longitude = msg.getFloat(4, 32) * 1e-7
        return pgn

    @staticmethod
    def fieldnames():
        return ["Fast-Latitude", "Fast-Longitude"]

    def fieldvalues(s):
        return [s.latitude, s.longitude]


@pgnclass(129026)
class N2KPGN_129026:
    """Speed and Course"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    seq: int = 0
    magnetic: N2KBearingRef = N2KBearingRef.UNKNOWN
    course: float = float("NaN")
    speed: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 129026
        pgn = N2KPGN_129026()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.seq = msg.getUInt8(0)
        pgn.magnetic = N2KBearingRef(msg.getUInt8(1) & 0x03)
        pgn.course = msg.getFloat(2, 16, False) * N2K_TO_DEGREES
        pgn.speed = msg.getFloat(4, 16, True) * 0.01
        return pgn

    @staticmethod
    def fieldnames():
        return ["Fast-CourseOverGround", "Fast-SpeedOverGround", "Fast-COGSOGReference"]

    def fieldvalues(s):
        return [s.course, s.speed, s.magnetic]


@pgnclass(129029)
class N2KPGN_129029:
    """Detailed GPS/GNSS Information"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    seq: int = 0
    epochDays: int = 0
    seconds: float = float("NaN")
    latitude: float = float("NaN")
    longitude: float = float("NaN")
    altitude: float = float("NaN")
    type: int = 0
    method: int = 0
    integ: int = 0
    numsv: int = 0
    hdop: float = float("NaN")
    pdop: float = float("NaN")
    geoidsep: float = float("NaN")
    rs: int = 0
    rst: int = 0
    rsid: int = 0
    dgnssa: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 129029
        pgn = N2KPGN_129029()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.seq = msg.getUInt8(0)
        pgn.epochDays = msg.getUInt16(1)
        pgn.seconds = msg.getUInt32(3) * 0.0001
        pgn.latitude = msg.getFloat(7, 64) * 1e-16
        pgn.longitude = msg.getFloat(15, 64) * 1e-16
        pgn.alt = msg.getFloat(23, 64) * 1e-6

        tm = msg.getUInt8(31)
        pgn.type = tm & 0x0F
        pgn.method = (tm & 0xF0) >> 4

        pgn.integ = (msg.getUInt8(32) & 0xC0) >> 12

        pgn.numsv = msg.getUInt8(33)
        pgn.hdop = msg.getFloat(34, 16) * 0.01
        pgn.pdop = msg.getFloat(36, 16) * 0.01
        pgn.geoidsep = msg.getFloat(38, 16) * 0.01

        pgn.rs = msg.getUInt8(40)
        rsd = msg.getUInt16(41)
        pgn.rst = rsd & 0x000F
        pgn.rsid = (rsd & 0xFFF0) >> 4

        if msg.datalen >= 45:
            pgn.dgnssa = msg.getFloat(43, 16) * 0.01
        return pgn

    def asTimestamp(self):
        """Return an approximate epoch date/time"""
        return 86400 * self.epochDays + self.seconds

    @staticmethod
    def fieldnames():
        return [
            "Latitude",
            "Longitude",
            "Altitude",
            "GNSS-System",
            "GNSS-Type",
            "HDOP",
            "PDOP",
            "GNSS-SVs",
            "GNSS-Timestamp",
        ]

    def fieldvalues(s):
        return [
            s.latitude,
            s.longitude,
            s.alt,
            s.type,
            s.method,
            s.hdop,
            s.pdop,
            s.numsv,
            s.asTimestamp(),
        ]


@pgnclass(129033)
class N2KPGN_129033:
    """Date/Time information"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    epochDays: int = 0
    seconds: float = float("NaN")
    utcMins: int = 0

    def asTimestamp(self):
        """Return an approximate epoch date/time"""
        return 86400 * self.epochDays + self.seconds

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 129033
        pgn = N2KPGN_129033()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.epochDays = msg.getUInt16(0)
        pgn.seconds = msg.getUInt32(2) * 0.0001
        pgn.utcMins = msg.getInt16(6)
        if abs(pgn.utcMins) > 1440:
            pgn.utcMins = 0
        return pgn

    @staticmethod
    def fieldnames():
        return ["Timestamp"]

    def fieldvalues(s):
        return [s.asTimestamp()]


@pgnclass(130306)
class N2KPGN_130306:
    """Wind speed / direction"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    seq: int = 0
    ref: N2KWindDirs = N2KWindDirs.UNKNOWN
    speed: float = float("NaN")
    angle: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 130306
        pgn = N2KPGN_130306()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp
        pgn.seq = msg.getUInt8(0)
        pgn.speed = msg.getFloat(1, 16, True) * 0.01
        pgn.angle = msg.getFloat(3, 16, False) * N2K_TO_DEGREES
        pgn.ref = N2KWindDirs(msg.getUInt8(5) & 0x07)

        return pgn

    @staticmethod
    def fieldnames():
        return ["WindSpeed", "WindDirection", "WindReference"]

    def fieldvalues(s):
        return [s.speed, s.angle, s.ref]


@pgnclass(130311)
class N2KPGN_130311:
    """Environmental Data"""

    priority: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
    seq: int = 0
    tid: N2KTemperatureSource = N2KTemperatureSource.UNKNOWN
    hid: N2KHumiditySource = N2KHumiditySource.UNKNOWN
    temperature: float = float("NaN")
    humidity: float = float("NaN")
    pressure: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 130311
        pgn = N2KPGN_130311()
        pgn.priority = msg.priority
        pgn.dst = msg.dst
        pgn.src = msg.src
        pgn.timestamp = msg.timestamp

        pgn.seq = msg.getUInt8(0)

        ids = msg.getUInt8(1)
        pgn.tid = ids & 0x3F
        pgn.hid = (ids & 0xC0) >> 6

        pgn.temperature = msg.getFloat(2, 16, False) * 0.01 - 273.15
        pgn.humidity = msg.getFloat(4, 16, True) * 0.004
        pgn.press = msg.getFloat(6, 16, False)

        return pgn

    def fieldnames(s):
        return [
            f"Temperature-0x{s.src:02x}",
            f"Humidity-0x{s.src:02x}",
            f"Pressure-0x{s.src:02x}",
        ]

    def fieldvalues(s):
        return [s.temperature, s.humidity, s.pressure]
