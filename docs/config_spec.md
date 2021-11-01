Configuration File Options
--------------------------
The key to use in the ini file is specified in brackets after the description.

Global Options
==============

- Console verbosity (verbose)

- Timestamp / marker frequency (frequency)

- Monitor file path/prefix (prefix)
- Monitor file rotation (rotate)

- State file path/prefix (statefile)
- State file enabled (savestate)


Generic Options
===============

These should be supported by all sources.

- Tag (from section name)
- Source ID (sourcenum)
- Source Name (name)

An exception is data from MP sources, as these will provide the source number
and name in the provided data.

Tags are mandatory and are taken from the section name used in the
configuration file. If no source name is specified, the tag will be used
instead.

GPS Source Options
==================

- Serial port (port)
- Initial baud rate (initialbaud)
- General baud rate (baud)
- Unfiltered Output (dumpall)

MP Source Options
=================

- Serial port (port)
- Baud rate (baud)

*N.B. that sourcenum and name are ignored here*

I2C Source Options
==================

- Bus path (bus)
- Sample rate (frequency)
  - This is set per bus, not per sensor

Sensor definitions need to define sensor type, I2C address and the (base) message ID.
Sensor type is provided by the configuration option name (which may be present more than once), with the I2C address and base message ID provided in hex, separated by a colon.
If the base message ID is missing, a default will be substituted. Mixing automatic and manual allocation of base message ID may lead to conflicts.

- INA219 Voltage and current monitor (ina219)
  - e.g. ina219 = 0x42:0x05
- ADS1015 ADC (ads1015)

NMEA Source Options
===================

- Source Name (name)
- Serial port (port)
- Baud rate (baud)

Datawell Source Options
=======================
- Serial port, *or*
- Network address and port

