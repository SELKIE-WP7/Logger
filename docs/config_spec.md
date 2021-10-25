Configuration File Options
--------------------------

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

- Bus path
- Sensor definitions:
  - Sensor Address
  - Sensor Type
  - Base address

NMEA Source Options
===================

- Source Name (name)
- Serial port (port)
- Baud rate (baud)

Datawell Source Options
=======================
- Serial port, *or*
- Network address and port

