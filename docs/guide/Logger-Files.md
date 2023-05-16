# Files {#LoggerFiles}
[TOC]
When running the logger, four main output files are (or can be) generated. Three of these share the same prefix but with different file extensions.
These are the main data file (ending with `.dat`), a channel mapping file (ending with `.var`) that contains information about the sources and channels recorded, and an event log file (ending with `.log`).
The fourth file is a summary of the system state, written to the file name set as `stateFile` in the configuration file.

As an example, if the configuration file contains:
~~~{.py}
stateFile = "data/example.state"
saveState = True
prefix = "data/Log-"
rotate = True
~~~

Then the output files would be:
~~~{.py}
data/Log-2023030200.dat
data/Log-2023030200.var
data/Log-2023030200.log
data/example.state
~~~

## Main data file {#dat}
## Channel mapping / variable information file {#var}
## Text log file {#log}
## State file {#state}


## Further reading
* Up: [Logger user guide](@ref Logger)
* Prev: [Configuration](@ref LoggerConfig)
* Next: [Technical details](@ref LoggerTechDetails)
