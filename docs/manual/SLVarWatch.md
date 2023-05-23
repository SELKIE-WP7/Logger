# SLVarWatch {#SLVarWatch}

## NAME
SLVarWatch - Watch state file and warn if variables exceed configured limits.

## SYNOPSIS
**SLVarWatch**  **-h**

**SLVarWatch** [**-v**] [**-t** *N*] [**-P**] *STATEFILE* *variable* [*variable* ...]

## DESCRIPTION
Taking a logger state file, this utility will poll the file for changes in the value of specific channels and issue a warning if the value of any of the specified channels exceeds configured limits.

Each channel to be watched needs to be specified on the command line to specify the limits.

The format of this locator is as a comma separated series of parameters:

~~~
Channel,LowerLimit,UpperLimit[,Name]
~~~

The Name parameter is optional.

The channel details are specified as a colon separated series of values:

~~~
Source:Channel[:Index]
~~~

The Index parameter is only required for data sources providing an array of values rather than individual channels.

#### Examples
```
0x62:0x08,0.1,1.5,VRef
```
Monitor channel 8 from source 98 (0x62), label it "VRef" and warn if its value drops below 0.1 or exceeds 1.5.

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

*variable*
:  One or more variable definitions  - see full description above.

## SEE ALSO
SLGPSWatch(1)
