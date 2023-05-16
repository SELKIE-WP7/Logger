# Core Options {#LoggerConfigCore}
[TOC]

All of the configuration for the software is contained within an INI style text file, consisting of `key = value` pairs and sections defined by a tag within square brackets (i.e.  `[tag]`)

The core options are specified at the top of the configuration file, outside of any section.
Each source of data is configured by defining a new uniquely named section, and this is described in more detail in [source definitions](@ref LoggerConfigSources)

## Information output
<!-- These aren't python, but it gives suitable colouring -->
~~~{.py}
# Valid values 0,1,2 or 3
verbose = 0
~~~

The amount of information output while the software is running can be controlled by choosing a level here.
A setting of 0 (zero) here will output very limited status information, while a setting of 3 will output debugging information.
Warnings and errors are always shown.

This setting can be adjusted at runtime using the command line flags `-v` - which will increase the setting by 1 - or `-q` - which will decrease it by 1.

## Timestamp Frequency

~~~{.py}
# Markers per second
# Minimum: 1
frequency = 10
~~~

Control the rate at which internal timestamps are generated.
The frequency is specified as the number of timestamps to be output per second.

The maximum value here is limited by hardware capabilities, but an excessively high value will inflate file sizes and may limit the ability of hardware to read in genuine data from other sources.

## Output file options

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

More information about the different output files is described on the [file formats](@ref LoggerFiles) page

## State file options

~~~{.py}
# Enable / disable use of a state file
savestate = True
# Path to state file (if enabled)
statefile = "/media/data/Logger/current.state"
~~~

If the `savestate` option is enabled, a summary of the logged data is periodically written to the file named in `statefile`.
This gives a snapshot of data received by the logging software and when the last value on each channel was received.

As this represents a snapshot, this file does not get suffixed with the date and serial number and is not rotated at midnight - regardless of the `rotate` setting.

More information about the state file is described on the [file formats](@ref LoggerFiles) page


## Further reading
* Up: [Logger configuration](@ref LoggerConfig)
* Next: [Logger source definitions](@ref LoggerConfigSources)
