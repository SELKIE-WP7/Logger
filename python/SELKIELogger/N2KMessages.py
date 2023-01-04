from dataclasses import dataclass, field
from enum import IntEnum
from struct import pack, unpack

import logging

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


@dataclass
class ACTN2KMessage:
    length: int = 0
    priority: int = 0
    PGN: int = 0
    dst: int = 0
    src: int = 0
    timestamp: int = 0
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
            remaining -= 1
            if c == ACTChar.ESC:
                nc = data[datapos + 1]
                if nc == ACTChar.ESC:
                    msg.data.append(ACTChar.ESC)
                    datapos += 1
                    continue
                elif nc == ACTChar.EOT:
                    logging.debug("Message terminated early")
                    return (None, spos + datapos)
                elif nc == ACTChar.SOT:
                    logging.debug("Unexpected message restart")
                    return (None, spos + datapos - 2)
                else:
                    logging.debug(
                        f"Invalid escape sequence found in input data (ESC + 0x{nc:02x}"
                    )
                    return (None, spos + datapos)
            msg.data.append(c)
            datapos += 1

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
                    x = x[msg[1] :] + d
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


@pgnclass(60928)
class N2KPGN_60928:
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


@pgnclass(127250)
class N2KPGN_127250:
    seq: int = 0
    heading: float = float("NaN")
    deviation: float = float("NaN")
    variation: float = float("NaN")
    reference: int = -1

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 127250
        pgn = N2KPGN_127250()
        pgn.seq = msg.getUInt8(0)
        pgn.heading = msg.getFloat(1, 16, False) * N2K_TO_DEGREES
        pgn.deviation = msg.getFloat(3, 16, True) * N2K_TO_DEGREES
        pgn.variation = msg.getFloat(5, 16, True) * N2K_TO_DEGREES
        pgn.reference = msg.getUInt8(7) & 0x03
        return pgn


@pgnclass(127251)
class N2KPGN_127251:
    seq: int = 0
    rate: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 127251
        pgn = N2KPGN_127251()
        pgn.seq = msg.getUInt8(0)
        pgn.rate = msg.getFloat(1, 32, True) / 3200 * N2K_TO_DEGREES
        return pgn


@pgnclass(127257)
class N2KPGN_127257:
    seq: int = 0
    yaw: float = float("NaN")
    pitch: float = float("NaN")
    roll: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 127257
        pgn = N2KPGN_127257()
        pgn.seq = msg.getUInt8(0)
        pgn.yaw = msg.getFloat(1, 16, True) * N2K_TO_DEGREES
        pgn.pitch = msg.getFloat(3, 16, True) * N2K_TO_DEGREES
        pgn.roll = msg.getFloat(5, 16, True) * N2K_TO_DEGREES
        return pgn


@pgnclass(128267)
class N2KPGN_128267:
    seq: int = 0
    depth: float = float("NaN")
    offset: float = float("NaN")
    range: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 128267
        pgn = N2KPGN_128267()
        pgn.seq = msg.getUInt8(0)
        pgn.depth = msg.getFloat(1, 32) * 0.01
        pgn.offset = msg.getFloat(5, 16) * 0.01
        pgn.range = msg.getFloat(7, 8) * 10.0
        return pgn


@pgnclass(129025)
class N2KPGN_129025:
    latitude: float = float("NaN")
    longitude: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 129025
        pgn = N2KPGN_129025()
        pgn.latitude = msg.getFloat(0, 32) * 1e-7
        pgn.longitude = msg.getFloat(4, 32) * 1e-7
        return pgn


@pgnclass(129026)
class N2KPGN_129026:
    seq: int = 0
    magnetic: int = 0
    course: float = float("NaN")
    speed: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 129026
        pgn = N2KPGN_129026()
        pgn.seq = msg.getUInt8(0)
        pgn.magnetic = msg.getUInt8(1) & 0x03
        pgn.course = msg.getFloat(2, 16, False) * N2K_TO_DEGREES
        pgn.speed = msg.getFloat(4, 16, True) * 0.01
        return pgn


@pgnclass(129029)
class N2KPGN_129029:
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
    geos: float = float("NaN")
    rs: int = 0
    rst: int = 0
    rsid: int = 0
    dgnssa: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 129029
        pgn = N2KPGN_129029()
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
        pgn.geos = msg.getFloat(38, 16) * 0.01

        pgn.rs = msg.getUInt8(40)
        rsd = msg.getUInt16(41)
        pgn.rst = rsd & 0x000F
        pgn.rsid = (rsd & 0xFFF0) >> 4

        if msg.datalen >= 45:
            pgn.dgnssa = msg.getFloat(43, 16) * 0.01
        return pgn


@pgnclass(129033)
class N2KPGN_129033:
    epochDays: int = 0
    seconds: float = float("NaN")
    utcMins: int = 0

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 129033
        pgn = N2KPGN_129033()
        pgn.epochDays = msg.getUInt16(0)
        pgn.seconds = msg.getUInt32(2) * 0.0001
        pgn.utcMins = msg.getInt16(6)
        if abs(pgn.utcMins) > 1440:
            return None
        return pgn


@pgnclass(130306)
class N2KPGN_130306:
    seq: int = 0
    ref: int = 0
    speed: float = float("NaN")
    angle: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 130306
        pgn = N2KPGN_130306()
        pgn.seq = msg.getUInt8(0)
        pgn.speed = msg.getFloat(1, 16, True) * 0.01
        pgn.angle = msg.getFloat(3, 16, False) * N2K_TO_DEGREES
        pgn.ref = msg.getUInt8(5) & 0x07

        return pgn


@pgnclass(130311)
class N2KPGN_130311:
    seq: int = 0
    tid: int = 0
    hid: int = 0
    temperature: float = float("NaN")
    humidity: float = float("NaN")
    pressure: float = float("NaN")

    @staticmethod
    def fromMessage(msg):
        assert msg.PGN == 130311
        pgn = N2KPGN_130311()
        pgn.seq = msg.getUInt8(0)

        ids = msg.getUInt8(1)
        pgn.tid = ids & 0x3F
        pgn.hid = (ids & 0xC0) >> 6

        pgn.temperature = msg.getFloat(2, 16, False) * 0.01 - 273.15
        pgn.humidity = msg.getFloat(4, 16, True) * 0.004
        pgn.press = msg.getFloat(6, 16, False)

        return pgn
