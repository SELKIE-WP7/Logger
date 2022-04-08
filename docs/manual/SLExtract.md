# SLExtract {#SLExtract}

## NAME
SLExtract - SELKIE Logger data extraction tool

## SYNOPSIS
**SLExtract** -h

**SLExtract**  [**-v**] [**-r**] **-s** *SOURCE* **-c** *CHANNEL* [**-o** *OUTFILE*] *DATFILE*

## DESCRIPTION
**SLExtract** allows specific data channels to be extracted from a data file. The main use case for this is to allow raw data embedded within a data file to be analysed using other software - typically software produced by the device manufacturer.

The channel to be extracted must be specified by numerical source and channel ID.

Data can optionally be output without any headers specifying source or channel information. This is useful if extracting raw data (usually on channel 3) for analysis in other software.

## OPTIONS
**-v**, **--verbose**
:  Increase output verbosity

**-r**, **--raw**
:  Enable raw output

**-s** *ID*, **--source** *ID*
:  Source ID to be extracted

**-c** *ID*, **--channel** *ID*
:  Channel ID to be extracted

**-o** *OUTFILE*, **--output** *OUTFILE*
:  Path to output file.

Source and channel IDs can be specified as decimal numbers or in hexadecimal using the prefix 0x - e.g. `-s 98` or `-s 0x62`
