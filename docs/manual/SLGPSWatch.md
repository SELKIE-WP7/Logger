# SLGPSWatch {#SLGPSWatch}

Watch a logger state file for GPS positions and warn if positions move outside a configured perimeter.

### Locators
Each position to be watched needs to be specified on the command line to specify the logger channels to use as Latitude and Longitude, the reference point and warning distance.

The format of this locator is as a comma separated series of parameters:
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

### Command line options
```
usage: SLGPSWatch [-h] [-v] [-t N] [-P] STATEFILE locator [locator ...]
```

- `STATEFILE`
  - State file name
- locator
  - One or more locator strings - see full description above
- -v or --verbose
  - Increase output verbosity
- -t or --interval
  - Poll state file every N seconds
- -P or --disable-pushover
  - Disable pushover notifications (print warnings to console only)
