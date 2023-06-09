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

import msgpack
import logging
from warnings import warn

## @file


class IDs:
    """! Mirror ID numbers from base/sources.h"""

    ## Messages generated by the logging software
    SLSOURCE_LOCAL = 0x00
    ## Messages generated by data conversion tools
    SLSOURCE_CONV = 0x01
    ## Local/Software timers
    SLSOURCE_TIMER = 0x02
    ## Test data source ID (1/3)
    SLSOURCE_TEST1 = 0x05
    ## Test data source ID (2/3)
    SLSOURCE_TEST2 = 0x06
    ## Test data source ID (3/3)
    SLSOURCE_TEST3 = 0x07
    ## GPS (or other satellite navigation) sources
    SLSOURCE_GPS = 0x10
    ## Generic analogue inputs
    SLSOURCE_ADC = 0x20
    ## NMEA Bus
    SLSOURCE_NMEA = 0x30
    ## N2K Bus
    SLSOURCE_N2K = 0x38
    ## I2C Bus
    SLSOURCE_I2C = 0x40
    ## Other external data sources
    SLSOURCE_EXT = 0x60
    ## MQTT derived data
    SLSOURCE_MQTT = 0x68
    ## Devices with suitable MessagePack output returning single value channels
    SLSOURCE_MP = 0x70

    ## Name of source device
    SLCHAN_NAME = 0x00
    ## Channel name map (excludes log channels)
    SLCHAN_MAP = 0x01
    ## Source timestamp (milliseconds, arbitrary epoch)
    SLCHAN_TSTAMP = 0x02
    ## Raw device data (Not mandatory)
    SLCHAN_RAW = 0x03
    ## Information messages
    SLCHAN_LOG_INFO = 0x7D
    ## Warning messages
    SLCHAN_LOG_WARN = 0x7E
    ## Error messages
    SLCHAN_LOG_ERR = 0x7F


## @brief Python representation of a logged message
class SLMessage:
    """!
    Represent messages stored by the Logger program and generated by devices
    with compatible firmware.

    Messages are 4 element MessagePacked arrays, with the first element a
    constant 0x55, second and third elements identifying the source and message
    IDs and the final element containing the data for that message.
    """

    ## Explicitly define members
    __slots__ = ["SourceID", "ChannelID", "Data"]

    def __init__(self, sourceID, channelID, data):
        """!
        Create message with specified source, channel and data.

        Note that the C library and utilities only support a limited range of
        types within these messages. Check the library documentation for
        details.

        @param sourceID Message Source
        @param channelID Message Channel
        @param data Message value / data

        @sa library/base/sources.h
        """
        try:
            assert 0 <= sourceID and sourceID < 128
            assert 0 <= channelID and channelID < 128
        except:
            logging.error(
                f"Invalid source/channel values: Source: {sourceID}, Channel: {channelID}, Data: {data}"
            )
            raise ValueError("Invalid source/channel values")

        ## Message Source @see loggerSources
        self.SourceID = sourceID
        ## Message Channel @see loggerChannels
        self.ChannelID = channelID
        ## Message value / embedded data
        self.Data = data

    @classmethod
    def unpack(cl, data):
        """!
        Unpack raw messagepack bytes or list into SLMessage.
        If a list is provided, it's expected to correspond to each member of
        the raw array, i.e. [0x55, sourceID, channelID, data]

        @param cl Class to be created (classmethod)
        @param data Raw bytes or a list of values.
        @returns New message instance
        """
        if isinstance(data, bytes):
            data = msgpack.unpackb(data)
        if not isinstance(data, list) or len(data) != 4:
            raise ValueError("Bad message")
        assert data[0] == 0x55
        return cl(data[1], data[2], data[3])

    def pack(self):
        """!
        Return packed binary representation of message
        @returns Bytes representing message in messagepack form
        """
        return msgpack.packb([0x55, self.SourceID, self.ChannelID, self.Data])

    def __repr__(self):
        """!
        Represent class as packed binary data
        @todo Replace with standards compliant repr
        @returns Message, as bytes
        """
        return self.pack()

    def __str__(self):
        """! @returns printable representation of message"""
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
        """!
        Any valid source must have a source ID, name and a list of named data channels
        @param sourceID Valid data source ID number - @see IDs
        @param name Source Name (Default: PythonDL)
        @param dataChannels Number of data channels to be created (ID 3+)
        @param dataChannelNames Names for data channels (First entry = Channel 3)
        """
        ## ID number for this source - @see IDs
        self.SourceID = int(sourceID)
        ## Source Name
        self.Name = str(name)
        ## Channel Map: Names for all data channels
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
        """! @returns Source name message (Channel 0)"""
        return SLMessage(self.SourceID, 0, self.Name)

    def ChannelsMessage(self):
        """! @returns Channel name map message (Channel 1)"""
        return SLMessage(self.SourceID, 1, self.ChannelMap)

    def InfoMessage(self, message):
        """!
        @returns INFO level log message
        @param message Message text
        """
        return SLMessage(self.SourceID, 125, str(message))

    def WarningMessage(self, message):
        """!
        @returns WARNING level log message
        @param message Message text
        """
        return SLMessage(self.SourceID, 126, str(message))

    def ErrorMessage(self, message):
        """!
        @returns ERROR level log message
        @param message Message text
        """
        return SLMessage(self.SourceID, 127, str(message))

    def TimestampMessage(self):
        """!
        Placeholder for timestamp message (Channel 2)

        Sources should provide a timestamp periodically to allow messages
        generated at a particular time to be grouped.

        Not implemented here
        @returns N/A - Throws NotImplemented exception
        """
        raise NotImplemented

    def DataMessage(self, channelID, data):
        """!
        @returns Message representing data from this source
        @param channelID Channel ID (Must correspond to a map entry)
        @param data Message data
        """
        assert channelID < len(self.ChannelMap)
        return SLMessage(self.SourceID, channelID, data)


class SLChannelMap:
    """!
    Source and channel map data
    """

    ## Explicitly allocate class members
    __slots__ = ["_s", "_log"]

    class Source:
        """! Represent sources being tracked"""

        ## Explicitly allocate class members
        __slots__ = ["id", "name", "channels", "lastTimestamp"]

        def __init__(self, number, name=None, channels=None, lastTimestamp=None):
            """!
            Tracked source must be identified by name
            @param number SourceID
            @param name Source Name
            @param channels Channel Map
            @param lastTimestamp Last source timestamp received
            """
            ## Source ID
            self.id = number
            if name:
                ## Source Name
                self.name = f"[{number:02x}]"
            self.name = str(name)
            if channels:
                ## List of channels
                self.channels = [str(x) for x in channels]
            else:
                ## Initialised to defaults if not provided
                self.channels = ["Name", "Channels", "Timestamp"]

            if lastTimestamp:
                ## Last timestamp received
                self.lastTimestamp = lastTimestamp
            else:
                ## Initialised to zero if not provided
                self.lastTimestamp = 0

        def __getitem__(self, ch):
            """!
            Support subscripted access to channel names
            @param ch Channel ID
            @returns String representing channel name
            """
            if ch < len(self.channels):
                return str(self.channels[ch])
            elif ch == 125:
                return "Information"
            elif ch == 126:
                return "Warning"
            elif ch == 127:
                return "Error"
            else:
                return f"[{ch:02x}]"

        def __iter__(self):
            """!
            Allow iteration over channel list, e.g.
            ```
            for channel in source:
                print(channel)
            ```
            @returns Iterator over channel list
            """
            return iter(self.channels)

        def __str__(self):
            """!
            Represent source by its name
            @returns Source name
            """
            return self.name

    class Channel:
        """! Represents a single channel"""

        ## Explicitly allocate members
        __slots__ = ["name"]

        def __init__(self, name):
            """!
            @param name Channel Name
            """
            ## Channel Name
            self.name = name

        def __repr__(self):
            """!
            @todo Replace with more python compliant function
            @returns Channel name
            """
            return self.name

    # Back to the main SLChannelMap

    def __init__(self):
        """! Initialise blank map"""
        ## Dictionary of sources, keyed by ID
        self._s = dict()
        ## Logger object for later use
        self._log = logging.getLogger(__name__)

    def __getitem__(self, ix):
        """!
        Support subscripted access to sources
        @param ix Source ID
        @returns Source object
        """
        if not isinstance(ix, int):
            ix = int(ix, 0)
        return self._s[ix]

    def __iter__(self):
        """! @returns Iterator over sources"""
        return iter(self._s)

    def __next__(self):
        """! @returns next() source"""
        return next(self._s)

    def to_dict(self):
        """! @returns Dictionary representation of map"""
        return {x: self._s[x].channels for x in self._s}

    def GetSourceName(self, source):
        """!
        @param source Source ID
        @returns Formatted name for source ID
        """
        try:
            if isinstance(source, str):
                source = int(source, base=0)
            else:
                source = int(source)
        except Exception as e:
            self._log.error(str(e))
        if source in self._s:
            return self._s[source].name
        else:
            return f"[0x{source:02x}]"

    def GetChannelName(self, source, channel):
        """!
        @returns Formatted channel name
        @param source Source ID
        @param channel Channel ID
        """
        try:
            if isinstance(source, str):
                channel = int(channel, base=0)
            else:
                channel = int(channel)
        except Exception as e:
            self._log.error(str(e))

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
                return f"[0x{channel:02x}]"

    def NewSource(self, source, name=None):
        """!
        Create or update source
        @param source Source ID
        @param name Source Name
        @returns None
        """
        if self.SourceExists(source):
            self._log.error(
                f"Source 0x{source:02x} already exists (as {self.GetSourceName(source)})"
            )
        self._s[source] = self.Source(source, name)

    def SourceExists(self, source):
        """!
        @param source Source ID
        @returns True if source already known
        """
        return source in self._s

    def ChannelExists(self, source, channel):
        """!
        Special cases default channels that must always exist (SLCHAN_NAME,
        SLCHAN_MAP, SLCHAN_TSTAMP, SLCHAN_LOG_INFO, SLCHAN_LOG_WARN,
        SLCHAN_LOG_ERR), then checks for the existence of others.

        @param source Source ID
        @param channel Channel ID
        @returns True if channel exists in specified source
        """
        if channel in [0, 1, 2, 125, 126, 127]:
            return True

        if self.SourceExists(source):
            return channel < len(self._s[source].channels)
        return False

    def SetSourceName(self, source, name):
        """!
        Update source name, creating source if required.
        @param source SourceID
        @param name SourceName
        @returns None
        """
        if not self.SourceExists(source):
            self.NewSource(source, name)
        else:
            self._s[source].name = name

    def SetChannelNames(self, source, channels):
        """!
        Update channel map for a source, creating if required
        @param source SourceID
        @param channels List of channel names
        @returns None
        """
        if not self.SourceExists(source):
            self.NewSource(source)

        self._s[source].channels = channels

    def UpdateTimestamp(self, source, timestamp):
        """!
        Update last timestamp value for a source, creating source if required.
        @param source Source ID
        @param timestamp Timestamp value
        @returns None
        """
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
        """!
        Initialise message sink

        Optionally accepts a logging object for any error, warning, or
        information messages encountered in the source data.
        @param msglogger Logger object for messages extracted from data
        """
        ## Channel Map to be filled from input data
        self._sm = SLChannelMap()

        ## General logging object
        self._log = logging.getLogger(__name__)
        logging.addLevelName(5, "DDebug")

        ## Logger for messages extracted from data
        self._msglog = msglogger

        if self._msglog is None:
            self._log.warn("No message logger specified")
            self._msglog = logging.getLogger(f"{__name__}.msg")

    def SourceMap(self):
        """! @returns Source/Channel map"""
        return self._sm

    def FormatMessage(self, msg):
        """!
        Pretty print a message
        @param msg Message object
        @returns Formatted Message as string
        """
        return f"{self._sm.GetSourceName(msg.SourceID)}\t{self._sm.GetChannelName(msg.SourceID, msg.ChannelID)}\t{msg.Data}"

    def Process(self, message, output="dict", allMessages=False):
        """!
        Process an incoming message

        Accepts SLMessage object or bytes that can be unpacked into one.

        If a valid message is decoded, internal data structures are updated to
        provide channel and source names for message formatting and tracking of
        the last timestamp seen from each source. Any data messages are
        returned as a string or as a dictionary.

        @param message Message (or bytes) to be decoded
        @param output Output format for each message: String, dict, raw (SLMessage)
        @param allMessages Output messages normally used internally
        @return Message in selected format
        """
        if output not in [None, "string", "dict", "raw"]:
            raise ValueError("Bad output type")

        if not isinstance(message, SLMessage):
            try:
                message = SLMessage.unpack(message)
            except ValueError:
                self._log.warning("Bad message encountered, skipping")
                self._log.debug(repr(message))
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
