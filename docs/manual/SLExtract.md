# SLExtract {#SLExtract}

SLExtract allows specific data channels to be extracted from a data file. The main use case for this is to allow raw data embedded within a data file to be analysed using other software - typically software produced by the device manufacturer.

The channel to be extracted must be specified by numerical source and channel ID.

Data can optionally be output without any headers specifying source or channel information. This is useful if extracting raw data (usually on channel 3) for analysis in other software.

### Command line options
```bash
usage: SLExtract [-h] [-v] [-r] -s SOURCE -c CHANNEL [-o OUTFILE] DATFILE
```

Extract specific channel data from a SELKIE Logger data file

- `DATFILE`
  - Main data file name
- -v, --verbose
  - Increase output verbosity
- -r or --raw
  - Enable raw output
- -s or --source
  - Source ID to be extracted
- -c or -- channel
  - Channel ID to be extracted
- -o or --output
  - Path to output file. 

Source and channel IDs can be specified as decimal numbers or in hexadecimal using the prefix 0x - e.g. `-s 98` or `-s 0x62`
