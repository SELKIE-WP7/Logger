# Core Concepts {#LoggerConcepts}
[TOC]

## Overview

The logging software processes information from a series of different **sources**. These represent devices or groups of devices providing information that needs to be recorded.
Each source represents an input connected to the computer running the logging software, and can either be a link directly to a specific device (e.g. a serial or USB connection to a GPS module) or a group of devices (e.g. a serial to NMEA bridge).

A more complete list of sources is available under [supported devices and sources](@ref SupportedSources).

Each data source provides a number of different **channels** - some of these are defined by the logging software and allow sources and channels to provide configuration information or feedback, while the remainder are source specific.

Data is received from each source as **messages** containing the source and channel ID number as well as one or more values to be stored.
Groups of these messages are separated by **timestamps**, establishing the relative ordering of events and the precision to which the timing of events can be reconstructed later.

For most sources each channel will map to a particular value generated or measured by that source, such as a particular voltage input or the acceleration in a specific direction.
Some sources (such as GPS devices) provide groups of values as a single channel, guaranteeing that those values are always received (and timestamped) together but requiring specific support in data processing software.
The details of how this timestamping process works are discussed further below.

\image latex MessageSequence.eps "Example of a received sequence of messages showing source, channel, and data."
\image html MessageSequence.svg "Example of a received sequence of messages showing source, channel, and data."

### Messages

The SELKIE logging software defines a specific message format that forms the basis of the recorded file format. Each message consists of a prefix value, source and channel ID numbers and is followed by the message data. When stored to disk or transmitted between devices, these messages are encoded using the [msgpack](https://msgpack.org) format.

Some hardware has been developed during the SELKIE project that emits messages directly in this format, while data from other sources is read and processed by the logging software and individual messages synthesised for storage.

### Timestamping
In order to provide timing information when reconstructing the logged data, markers are added to the logged data by generating a message containing a timestamp at regular intervals.
Messages received between timestamps are not individually timed, so messages recorded in the interval between timestamps are grouped together during analysis and conversion to other formats.
This means that the timestamp frequency (set in the [core options](@ref LoggerConfigCore) section of the configuration) defines the temporal resolution of the output data.
Increasing the timestamp frequency will allow more granular reconstruction of data after recording, but will increase recorded file sizes and leads to significantly larger output files when converting to more generic formats.

As well as the internally generated timestamps, external data sources can also provide timestamps to associate with messages from that source.
This would allow messages from that source be batched together for improved timing accuracy, anchoring a group of messages from that source to the time that they were measured rather than the time received at the logging computer
This feature is not currently implemented in the data conversion software, but could be added in the future and applied to existing recorded files.

### Common Channels
There are 128 channels available for each source, with the following common channels defined:

#### Channel 0: Source Name
Each source is expected to send or generate occasional messages on this channel providing the name of the source.
This is displayed in log messages and data conversion tools, but doesn't get used directly.

#### Channel 1: Channel Map
Similar to the source name message, this channel is used to describe the channels provided by this source by providing a list of names. The number of names provided determines the number of channels allocated for the source when processing the data.

To avoid every source having to provide 100+ blank entries, the logging channels are excluded from the map.

#### Channel 2: Source Timestamp
Any source that has its own timing information can generate timestamps to indicate when a group of messages / values were created
The timestamps provided in this channel are expected to be measured in milliseconds, but are otherwise arbitrary.
This allows the delay between messages from the same source to be recorded independent of the overall timestamp frequency.

#### Channel 3: Raw Data
The last generic data channel, Channel 3 is used for arbitrary data that shouldn't be interpreted during data processing.
This can be used for messages that aren't being handled or processed by the data logger for later examination.
Messages using this channel can be easily extracted later and examined using existing tools for the data source in question.

#### Channels 125, 126, and 127: Information, Warning, and Error Messages
The final three reserved messages can be generated at any point by any source and are intended to be displayed either live or when data is later processed.

As a general rule:

- **Error** messages indicate data is unavailable or otherwise unlikely to be usable. This would include situations that lead to termination of logging software or device (possibly followed by a reset/restart).
- **Warning** messages indicate an unexpected state that may or may not indicate a problem with the source or recorded data. Warnings may also be included to record unusual configuration or commands for future reference.
- **Information** messages are included for future reference, but are not expected to require any action to be taken. This would include instrument serial numbers, software versions etc.

## Further reading
* Up: [Logger user guide](@ref Logger)
* Next: [Hardware and Installation](@ref LoggerHardware)
