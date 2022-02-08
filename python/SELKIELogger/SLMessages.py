import msgpack
import logging
from warnings import warn


class IDs:
    SLSOURCE_LOCAL = 0x00
    SLSOURCE_CONV = 0x01
    SLSOURCE_TIMER = 0x02
    SLSOURCE_TEST1 = 0x05
    SLSOURCE_TEST2 = 0x06
    SLSOURCE_TEST3 = 0x07
    SLSOURCE_GPS = 0x10
    SLSOURCE_ADC = 0x20
    SLSOURCE_NMEA = 0x30
    SLSOURCE_I2C = 0x40
    SLSOURCE_EXT = 0x60
    SLSOURCE_MQTT = 0x68
    SLSOURCE_MP = 0x70
    SLSOURCE_IMU = SLSOURCE_MP

    SLCHAN_NAME = 0x00
    SLCHAN_MAP = 0x01
    SLCHAN_TSTAMP = 0x02
    SLCHAN_RAW = 0x03
    SLCHAN_LOG_INFO = 0x7
    SLCHAN_LOG_WARN = 0x7
    SLCHAN_LOG_ERR = 0x7


## @brief Python representation of a logged message
class SLMessage:
    """
    Represent messages stored by the Logger program and generated by devices
    with compatible firmware.

    Messages are 4 element MessagePacked arrays, with the first element a
    constant 0x55, second and third elements identifying the source and message
    IDs and the final element containing the data for that message.
    """

    __slots__ = ["SourceID", "ChannelID", "Data"]

    def __init__(self, sourceID, channelID, data):
        """!
        Create message with specified source, channel and data.

        Note that the C library and utilities only support a limited range of
        types within these messages. Check the library documentation for
        details.

        @sa library/base/sources.h
        """
        try:
            assert 0 <= sourceID and sourceID < 128
            assert 0 <= channelID and channelID < 128
        except:
            log.error(
                f"Invalid source/channel values: Source: {sourceID}, Channel: {channelID}, Data: {data}"
            )
            raise ValueError("Invalid source/channel values")

        self.SourceID = sourceID
        self.ChannelID = channelID
        self.Data = data

    @classmethod
    def unpack(cl, data):
        """Unpack raw messagepack bytes or list into SLMessage"""
        if isinstance(data, bytes):
            data = msgpack.unpackb(data)
        if not isinstance(data, list) or len(data) != 4:
            raise ValueError("Bad message")
        assert data[0] == 0x55
        return cl(data[1], data[2], data[3])

    def pack(self):
        """Return packed binary representation of message"""
        return msgpack.packb([0x55, self.SourceID, self.ChannelID, self.Data])

    def __repr__(self):
        """Represent class as packed binary data"""
        return self.pack()

    def __str__(self):
        """Return printable representation of message"""
        return f"{self.SourceID:02x}\t{self.ChannelID:02x}\t{str(self.Data)}"


class SLMessageSource:
    """!
    Software message source

    Provide a framework for creating valid messages from within Python code.
    Although this class enforces channel names and provides support for the
    standard messages (except timestamps), no restriction is placed on the data
    types used in data messages. See separate documentation for the library to
    check compatibility.

    @sa library/base/sources.h
    """

    def __init__(
        self, sourceID, name="PythonDL", dataChannels=1, dataChannelNames=None
    ):
        """Any valid source must have a source ID, name and a list of named data channels"""
        self.SourceID = int(sourceID)
        self.Name = str(name)
        self.ChannelMap = ["Name", "Channels", "Timestamp"]

        if self.SourceID > 127 or self.SourceID <= 0:
            raise ValueError("Invalid Source ID")

        if dataChannelNames is None:
            dataChannelNames = [f"Data{x+1}" for x in range(dataChannels)]

        if len(dataChannelNames) != dataChannels:
            raise ValueError(
                f"Inconsistent number of channels specified: Expected {dataChannels}, got {len(dataChannels)}"
            )

        if len(dataChannelNames) > 0:
            self.ChannelMap.extend(dataChannelNames)

    def IDMessage(self):
        """Return source name message (Channel 0)"""
        return SLMessage(self.SourceID, 0, self.Name)

    def ChannelsMessage(self):
        """Generate channel name map message (Channel 1)"""
        return SLMessage(self.SourceID, 1, self.ChannelMap)

    def InfoMessage(self, message):
        """Return INFO level log message"""
        return SLMessage(self.SourceID, 125, str(message))

    def WarningMessage(self, message):
        """Return WARNING level log message"""
        return SLMessage(self.SourceID, 126, str(message))

    def ErrorMessage(self, message):
        """Return ERROR level log message"""
        return SLMessage(self.SourceID, 127, str(message))

    def TimestampMessage(self):
        """
        Placeholder for timestamp message (Channel 2)

        Sources should provide a timestamp periodically to allow messages
        generated at a particular time to be grouped.

        Not implemented here
        """
        raise NotImplemented

    def DataMessage(self, channelID, data):
        """Return message representing data from this source"""
        assert channelID < len(self.ChannelMap)
        return SLMessage(self.SourceID, channelID, data)


class SLChannelMap:
    __slots__ = ["_s", "_log"]

    class Source:
        __slots__ = ["id", "name", "channels", "lastTimestamp"]

        def __init__(self, number, name=None, channels=None, lastTimestamp=None):
            self.id = number
            if name:
                self.name = f"[{number:02x}]"
            self.name = name
            if channels:
                self.channels = channels
            else:
                self.channels = ["Name", "Channels", "Timestamp"]
            self.lastTimestamp = 0

        def __getitem__(self, ch):
            if ch < len(self.channels):
                return self.channels[ch]
            elif ch == 125:
                return "Information"
            elif ch == 126:
                return "Warning"
            elif ch == 127:
                return "Error"
            else:
                return f"[{ch:02x}]"

        def __iter__(self):
            return iter(self.channels)

        def __str__(self):
            return self.name

    class Channel:
        __slots__ = ["name"]

        def __init__(self, name):
            self.name = name

        def __repr__(self):
            return self.name

    def __init__(self):
        self._s = dict()
        self._log = logging.getLogger(__name__)

    def __getitem__(self, ix):
        return self._s[ix]

    def __iter__(self):
        return iter(self._s)

    def __next__(self):
        return next(self._s)

    def to_dict(self):
        return {x: self._s[x].channels for x in self._s}

    def GetSourceName(self, source):
        """Return formatted name for source ID"""
        if source in self._s:
            return self._s[source].name
        else:
            return f"[{source:02x}]"

    def GetChannelName(self, source, channel):
        """Return formatted channel name"""
        if not isinstance(channel, int):
            self._log.warning(f"Non-integer channel number requested: {channel}")
            return f"[Invalid:{channel}]"
        if self.SourceExists(source):
            if channel < len(self._s[source].channels):
                return self._s[source].channels[channel]
            elif channel == 125:
                return "Information"
            elif channel == 126:
                return "Warning"
            elif channel == 127:
                return "Error"
            else:
                return f"[{channel:02x}]"

    def NewSource(self, source, name=None):
        if self.SourceExists(source):
            self._log.error(
                f"Source 0x{source:02x} already exists (as {self.GetSourceName(source)})"
            )
        self._s[source] = self.Source(source, name)

    def SourceExists(self, source):
        return source in self._s

    def ChannelExists(self, source, channel):
        if channel in [0, 1, 2, 125, 126, 127]:
            return True

        if self.SourceExists(source):
            return channel < len(self._s[source].channels)
        return False

    def SetSourceName(self, source, name):
        if not self.SourceExists(source):
            self.NewSource(source, name)
        else:
            self._s[source].name = name

    def SetChannelNames(self, source, channels):
        if not self.SourceExists(source):
            self.NewSource(source)

        self._s[source].channels = channels

    def UpdateTimestamp(self, source, timestamp):
        if not self.SourceExists(source):
            self.NewSource(source)
        self._s[source].lastTimestamp = int(timestamp)


class SLMessageSink:
    """!
    Parse incoming messages.

    Maintains an internal mapping of channel and source names to provide prettier output.

    Expects data to be provided one message at a time to .Process(), and will
    return data as a dict, printable string or as an SLMessage().
    """

    def __init__(self, msglogger=None):
        """
        Initialise message sink

        Optionally accepts a logging object for any error, warning, or
        information messages encountered in the source data.
        """
        self._sm = SLChannelMap()

        self._log = logging.getLogger(__name__)
        logging.addLevelName(5, "DDebug")
        self._msglog = msglogger

        if self._msglog is None:
            warn("No message logger specified")
            self._msglog = logging.getLogger(f"{__name__}.msg")

    def SourceMap(self):
        return self._sm

    def FormatMessage(self, msg):
        """Pretty print a message"""
        return f"{self._sm.GetSourceName(msg.SourceID)}\t{self._sm.GetChannelName(msg.SourceID, msg.ChannelID)}\t{msg.Data}"

    def Process(self, message, output="dict", allMessages=False):
        """Process an incoming message

        Accepts a SLMessage or bytes that can be unpacked.
        If a valid message is decoded, internal data structures are updated to
        provide channel and source names for message formatting and tracking of
        the last timestamp seen from each source. Any data messages are
        returned as a string or as a dictionary.
        """
        if output not in [None, "string", "dict", "raw"]:
            raise ValueError("Bad output type")

        if not isinstance(message, SLMessage):
            try:
                message = SLMessage.unpack(message)
            except ValueError:
                self._log.info("Bad message encountered, skipping")
                self._log.debug(message)
                return

        if not self._sm.SourceExists(message.SourceID):
            self._sm.NewSource(message.SourceID)

        suppressOutput = False
        if message.ChannelID == 0:
            self._log.log(
                5,
                f"New name for {self._sm.GetSourceName(message.SourceID)}: {message.Data}",
            )
            self._sm.SetSourceName(message.SourceID, message.Data)
            suppressOutput = True
        elif message.ChannelID == 1:
            self._log.log(
                5,
                f"New channels for {self._sm.GetSourceName(message.SourceID)}: {message.Data}",
            )
            self._sm.SetChannelNames(message.SourceID, message.Data)
            suppressOutput = True
        elif message.ChannelID == 2:
            self._log.log(
                5,
                f"New update time for {self._sm.GetSourceName(message.SourceID)}: {message.Data}",
            )
            self._sm.UpdateTimestamp(message.SourceID, message.Data)
            suppressOutput = True
        elif message.ChannelID == 125:
            self._msglog.info(self.FormatMessage(message))
            suppressOutput = True
        elif message.ChannelID == 126:
            self._msglog.warning(self.FormatMessage(message))
            suppressOutput = True
        elif message.ChannelID == 127:
            self._msglog.error(self.FormatMessage(message))
            suppressOutput = True

        if allMessages or (not suppressOutput):
            if output == "dict":
                return {
                    "sourceID": message.SourceID,
                    "sourceName": self._sm.GetSourceName(message.SourceID),
                    "channelID": message.ChannelID,
                    "channelName": self._sm.GetChannelName(
                        message.SourceID, message.ChannelID
                    ),
                    "data": message.Data,
                }
            elif output == "string":
                return self.FormatMessage(message)
            elif output == "raw":
                return message
            else:
                return
