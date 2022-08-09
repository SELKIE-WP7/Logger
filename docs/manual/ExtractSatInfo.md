# ExtractSatInfo {#ExtractSatInfo}

## NAME
ExtractSatInfo - Extract Satellite information from SELKIE Logger data files

## SYNOPSIS
**ExtractSatInfo** [**-v**] [**-q**] [**-f**] [**-Z**]  [**-o** *OUTFILE*] *DATFILE*

## DESCRIPTION
Scans through a recorded data file for NAV-SAT messages from u-Blox GPS receivers and extracts satellite information.

Satellite information is written in compressed CSV format by default, compression can be disabled using the `-Z` command line option.

For each satellite, the signal to noise ratio, signal quality, elevation, azimuth and psudorange residual will be output.
Each line is indexed by GPS time of week, device source number, GNSS system and Satellite ID (i.e. GPS SV Number).
If a satellite is being used for navigation calculations, this is flagged in the final column.

## OPTIONS
**-v**
:  Increase output verbosity

**-q**
:  Decrease output verbosity

**-o** *OUTFILE*
:  Output file name

**-Z**
:  Disable output compression

## SEE ALSO
SLClassify(1)
