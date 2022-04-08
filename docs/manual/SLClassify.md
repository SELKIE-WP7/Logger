# SLClassify {#SLClassify}
## NAME
SLClassify - Generate summary of a SELKIE Logger data file

## SYNOPSIS
**SLClassify** **-h**

**SLClassify** [**-v**] *DATFILE*

## DESCRIPTION

Read messages from a SELKIE Logger data file and provide summary counts of messages categorised by source and channel IDs.

Any raw messages from GPS or NMEA messages embedded within the file are also categorised and a summary displayed based on UBX or NMEA message ID.

## OPTIONS
**-h**, **--help**
:  Show brief help message

**-v**, **--verbose**
:  Include timing and source/channel information messages during parsing.
