import msgpack
import logging
from warnings import warn

class SLMessage:
    """Represent messages stored by the Logger program and generated by devices
    with compatible firmware.

    Messages are 4 element MessagePacked arrays, with the first element a
    constant 0x55, second and third elements identifying the source and message
    IDs and the final element containing the data for that message.
    """
    __slots__ = ["SourceID", "ChannelID", "Data"]

    def __init__(self, sourceID, channelID, data):
        try:
            assert (0 <= sourceID  and sourceID < 128)
            assert (0 <= channelID and channelID < 128)
        except:
            print(f"Source: {sourceID}, Channel: {channelID}, Data: {data}")
            raise

        self.SourceID = sourceID
        self.ChannelID = channelID
        self.Data = data

    @classmethod
    def unpack(cl, data):
        if isinstance(data, bytes):
            data = msgpack.unpackb(data)
        if not isinstance(data, list) or len(data) != 4:
            raise ValueError("Bad message")
        assert (data[0] == 0x55)
        return cl(data[1], data[2], data[3])

    def pack(self):
        return msgpack.packb([0x55, self.SourceID, self.ChannelID, self.Data])

    def __repr__(self):
        return self.pack()

    def __str__(self):
        return f"{self.SourceID:02x}:{self.ChannelID:02x}\t{str(self.Data)}"

class SLMessageSource:
    """Software source of messages"""
    def __init__(self, sourceID, name="PythonDL", dataChannels=1, dataChannelNames=None):
        """Any valid source must have a source ID, name and a list of named data channels"""
        self.SourceID = int(sourceID)
        self.Name = str(name)
        self.ChannelMap = ["Name", "Channels", "Timestamp"]

        if self.SourceID > 127 or self.SourceID <= 0:
            raise ValueError("Invalid Source ID")

        if dataChannelNames is None:
            dataChannelNames = [f"Data{x+1}" for x in range(dataChannels)]

        if len(dataChannelNames) != dataChannels:
            raise ValueError(f"Inconsistent number of channels specified: Expected {dataChannels}, got {len(dataChannels)}")

        if len(dataChannelNames) > 0:
            self.ChannelMap.extend(dataChannelNames)

    def IDMessage(self):
        return SLMessage(self.SourceID, 0, self.Name)

    def ChannelsMessage(self):
        """Send data source name"""
        return SLMessage(self.SourceID, 1, self.ChannelMap)

    def InfoMessage(self, message):
        """Send INFO level log message"""
        return SLMessage(self.SourceID, 125, str(message))

    def WarningMessage(self, message):
        """Send WARNING level log message"""
        return SLMessage(self.SourceID, 126, str(message))

    def ErrorMessage(self, message):
        """Send ERROR level log message"""
        return SLMessage(self.SourceID, 127, str(message))

    def TimestampMessage(self):
        """Sources should provide a timestamp periodically to allow messages
        generated at a particular time to be grouped. Not implemented here"""
        raise NotImplemented

    def DataMessage(self, channelID, data):
        """Send data from this source"""
        assert channelID < len(self.ChannelMap)
        return SLMessage(self.SourceID, channelID, data)


class SLMessageSink:
    """Parse incoming messages. Includes processing incoming source and channel
    names to pretty up the output."""
    def __init__(self, msglogger=None):
        self.Sources = dict()

        if msglogger is None:
            warn("No message logger specified")
            self._msglog = logging.getLogger(f"{__name__}.msg")
        self._log = logging.getLogger(__name__)
        self._msglog = logging.getLogger(f"{__name__}.msg")

    def SourceName(self, source):
        """Update the map of known source names"""
        if source in self.Sources:
            return self.Sources[source]['name']
        else:
            return f"[{source:02x}]"

    def ChannelName(self, source, channel):
        """Update the map of known channel names"""
        if source in self.Sources:
            if channel < (len(self.Sources[source]['channels'])):
                return self.Sources[source]['channels'][channel]
            elif channel == 125:
                return "Information"
            elif channel == 126:
                return "Warning"
            elif channel == 127:
                return "Error"
            else:
                return f"[{channel:02x}]"

    def FormatMessage(self, msg):
        """Pretty print a message"""
        return f"{self.SourceName(msg.SourceID)}:{self.ChannelName(msg.SourceID, msg.ChannelID)}:{msg.Data}"

    def Process(self, message, output="dict"):
        """ Process an incoming message

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

        if not message.SourceID in self.Sources:
            self.Sources[message.SourceID] = {'name': f'[{message.SourceID:02x}]', 'channels': ["Name", "Channels", "Timestamp"], 'lastTimestamp': 0}

        if message.ChannelID == 0:
            self._log.debug(f"New name for {self.SourceName(message.SourceID)}: {message.Data}")
            self.Sources[message.SourceID]['name'] = message.Data
        elif message.ChannelID == 1:
            self._log.debug(f"New channels for {self.SourceName(message.SourceID)}: {message.Data}")
            self.Sources[message.SourceID]['channels'] = message.Data
        elif message.ChannelID == 2:
            self._log.debug(f"New update time for {self.SourceName(message.SourceID)}: {message.Data}")
            self.Sources[message.SourceID]['lastTimestamp'] = message.Data
        elif message.ChannelID == 125:
            self._msglog.info(self.FormatMessage(message))
        elif message.ChannelID == 126:
            self._msglog.warning(self.FormatMessage(message))
        elif message.ChannelID == 127:
            self._msglog.error(self.FormatMessage(message))
        else:
            if output == "dict":
                return {
                        'sourceID': message.SourceID, 'sourceName': self.SourceName(message.SourceID),
                        'channelID': message.ChannelID, 'channelName': self.ChannelName(message.SourceID, message.ChannelID),
                        'data': message.Data
                        }
            elif output == "string":
                return self.FormatMessage(message)
            elif output == "raw":
                return message
            else:
                return
