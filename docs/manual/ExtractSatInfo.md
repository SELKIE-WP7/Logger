# ExtractSatInfo {#ExtractSatInfo}

Scans through a recorded data file for NAV-SAT messages from u-Blox GPS receivers and extracts satellite information.

Satellite information is written in compressed CSV format by default, compression can be disabled using the `-Z` command line option.

For each satellite, the signal to noise ratio, signal quality, elevation, azimuth and psudorange residual will be output.
Each line is indexed by GPS time of week, device source number, GNSS system and Satellite ID (i.e. GPS SV Number).
If a satellite is being used for navigation calculations, this is flagged in the final column.

### Command line options
```sh
usage: ExtractSatInfo [-v] [-q] [-f] [-z|-Z]  [-o outfile] datfile
```

- `DATFILE`
  - Input data file name
- -v, --verbose
  - Increase output verbosity
- -q, --verbose
  - Decrease output verbosity
- -o OUTFILE
  - Output file name
- -z
  - Enable output compression
- -Z
  - Disable output compression
