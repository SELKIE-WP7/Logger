# Logger {#Logger}

# SELKIE Logger
This is the core of the data logging system, responsible for reading data from the various sources and recording it to disk.

[TOC]

## Sources, Channels, and Messages
Information is recorded from one or more **sources**, with each source generating data for one or more **channels**.
Each source represents an input connected to the computer running the logging software, and can either be a link directly to a specific device (e.g. a serial or USB connection to a GPS module) or a group of devices (e.g. a serial to NMEA bridge). A more complete list of sources is available later, under [supported devices and sources](@ref SupportedSources).

Data from each configured source is acquired in parallel, with incoming data split into **messages**.
Each message may represent a single input value or map onto an equivalent message in the source's own data format.
There are some special purpose channels which are reserved for specific information.
These allow each source to report the name to be used for that source along with the names to be used for each channel reported by that source.
Each source can also report its own timestamps to provide grouping information for the messages it generates.

## Timestamping

In order to provide timing information when reconstructing the logged data, markers are added to the logged data by generating a message containing a timestamp at regular intervals.
Messages received between timestamps are not individually timed, so messages recorded in the interval between timestamps are grouped together during analysis and conversion to other formats.

External data sources can also provide timestamps to associate with messages from that source, allowing messages to be batched together for improved timing accuracy.
This is not currently implemented in the data conversion software.

## Configuration

All of the configuration for the software is contained within an INI style text file.
Configuration options within the file can be split into two categories: Core options (which control the behaviour of the software itself) and source definitions (which define and configure the sources to be used).

Comments can be marked using the hash (`#`) character

### Core Options
#### Information output
<!-- These aren't python, but it gives suitable colouring -->
~~~{.py}
verbose = 0
# Valid values 0,1,2 or 3
~~~

The amount of information output while the software is running can be controlled by choosing a level here.
A setting of 0 (zero) here will output very limited status information, while a setting of 3 will output debugging information.
Warnings and errors are always shown.

This setting can be adjusted at runtime using the command line flags `-v` - which will increase the setting by 1 - or `-q` - which will decrease it by 1.

#### Timestamp Frequency

~~~{.py}
frequency = 10
# Markers per second
# Minimum: 1
~~~

Control the rate at which internal timestamps are generated.
The frequency is specified as the number of timestamps to be output per second.

The maximum value here is limited by hardware capabilities, but an excessively high value will inflate file sizes and may limit the ability of hardware to read in genuine data from other sources.

#### Output file options

~~~{.py}
# Path to data file - date and serial number to be appended
prefix = "/media/data/Logger/Log-"
# Enable rotation of output files at midnight
rotate = True
~~~

These two options control the output of recorded data.
The `prefix` option defines the path and initial prefix to be used for the main output files.
This value must end with a trailing slash (`/`) or the start of a valid file name.

The output files will then be created by adding the date and an incrementing serial number represented as two hexadecimal digits:
~~~{.py}
# Example file names
"<prefix>YYYYMMDDXX.dat" # Main data file
"<prefix>YYYYMMDDXX.var" # Channel mapping / variable information file
"<prefix>YYYYMMDDXX.log" # Text log file
~~~

If the `rotate` option is enabled, a new set of files will be created at midnight.
If the option is disabled then the files created when the software is started will be used until the software is terminated.

More information about the different output files is described below under [Files](@ref LoggerFiles)

#### State file options

~~~{.py}
# Enable / disable use of a state file
savestate = True
# Path to state file (if enabled)
statefile = "/media/data/Logger/current.state"
~~~

If the `savestate` option is enabled, a summary of the logged data is written to the file named in `statefile`.
This file does not get suffixed with the date and serial number and is not rotated at midnight, regardless of the `rotate` setting.

See the description in the [Files](@ref LoggerFiles) section for more information.

### Source Definitions
~~~{.py}
[Bus01]
name = Marine
type = n2k
sourcenum = 0x61
~~~

Each source is configured in its own section, starting with a section tag (`[Bus01]` in the example above) and followed by the source specific configuration values.
The name given in the tag is used when printing log messages to identify the source responsible for them and will be used as the source name unless overridden by a `name` option.

#### Generic configuration options
Common to almost all supported sources are the options `name`, `type`, and `sourcenum`. Of these, `type` is mandatory.

The `type` options identifies which of the supported source types are being configured by this block. Blocks without a valid `type` specified will generate an error.
See [supported sources](@ref SupportedSources) for details of valid type options.

The `name` option controls the reported source name used in the channel information file, which is then available for subsequent processing and data conversion software.
If not provided, it defaults to the section tag. The name is arbitrary information and is not used by the logging software itself.

The `sourcenum` option controls the ID number used to identify messages from this source. There can be only one source active with a given number for a particular configuration.
If no `sourcenum` option is present, a suitable value will be configured automatically.


### Example Configuration

~~~{.py}
stateFile = "testData/logger.state"
saveState = True
prefix = "testData/Log-"
verbose = 3
frequency = 10

[GPS01]
baud = 19200
sourcenum = 5
type = GPS
# N.B. Can't assume this will always be the same device
# Consider using udev rules to give devices permanent names
port = "/dev/ttyUSB0"
~~~

## Files {#LoggerFiles}
### Main data file
### Channel mapping / variable information file
### Text log file
### State file

## Supported Sources and Devices {#SupportedSources}
