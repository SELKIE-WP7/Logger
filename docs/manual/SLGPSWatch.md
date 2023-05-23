# SLGPSWatch {#SLGPSWatch}

## NAME
SLGPSWatch - Watch GPS information in state file and warn if position exceeds limits

## SYNOPSIS
**SLGPSWatch**  **-h**

**SLGPSWatch** [**-v**] [**-t** *N*] [**-P**] *STATEFILE* *locator* [*locator* ...]


## DESCRIPTION
Taking a logger state file, this utility will poll the file for changes and issue a warning if the position defined by a pair of Latitude and Longitude values exceeds a preset distance from a given reference point.

### Locators
Each position to be watched needs to be specified on the command line to specify the logger channels to use as Latitude and Longitude, the reference point and warning distance. These positions are defined on the command line using a set of comma separated series of parameters:
~~~
LatitudeChannel,LongitudeChannel,ReferenceLatitude,ReferenceLonitude,WarningThreshold[,Name]
~~~
The Name parameter is optional.

The latitude and longitude channels are specified as a colon separated series of values to identify the relevant data value:
~~~
Source:Channel[:Index]
~~~
The Index parameter is only required for data sources providing position information as an array of values rather than providing latitude and longitude as individual channels.

#### Examples
```
0x62:0x08,0x62:0x09:1,51.62007,-3.8761,1000,Marker
```
Creates a location called "Marker", using channel 8 from source 98 (0x62) as the Latitude and channel 9 from the same source as the Longitude. The reference point is given as 51.62007N, 3.8761W and will generate warnings if the sensor reports positions more than 1000m from the reference point.

```
0x61:0x04,0x61:0x05,51.89348,-8.49207,800,Buoy
```
This locator string would create a marker called "Buoy", using channels 4 and 5 of source 97 (0x61) as latitude and longitude respectively. Warnings would be generated if the reported position moves more than 800m from the reference point at 51.89348N, 8.49207W.

### Pushover

As well as printing warnings to the console or system log, notifications can also be sent using [Pushover](https://pushover.net) or a service with a compatible API.

Configuration options will be read from `/etc/pushover/pushover-config` and are expected to be in `key=value` form as described for the [pushover-bash](https://github.com/akusei/pushover-bash) script.

## OPTIONS
**-h**, **--help**
:  Show brief help message

**-v**, **--verbose**
:  Output additional messages during processing

**-t** *N*, **--interval** *N*
:  Poll state file every N seconds

**-P**, **--disable-pushover**
:  Disable pushover notifications (print warnings to console only)

*STATEFILE*
:  State file name and path

*locator*
:  One or more location definitions  - see full description above.

## SEE ALSO
SLVarWatch(1)
