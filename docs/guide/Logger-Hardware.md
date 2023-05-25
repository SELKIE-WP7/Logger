## Hardware and Installation {#LoggerHardware}

The data logging software has been developed to run on Linux systems, and can be run on typical desktop or laptop devices, or on smaller embedded systems such as the Raspberry Pi family of devices.
Of the latter, most testing has been carried out using Raspberry Pi 4 boards, with a number of different peripherals.

Examples of specific hardware data sources is given later in this guide under [supported devices and sources](@ref SupportedSources), but includes GPS receivers, networked devices, and a number of analog interface options.

## General Setup

### Prerequisites

This guide assumes that you have installed and configured Linux on the machine that will be used as a logger.
For Raspberry Pi devices, see the official [getting started](https://www.raspberrypi.com/documentation/computers/getting-started.html) guide.
The use of the "Lite" images is recommended.

Other items you may wish to consider:
- Provision of additional storage
- Configure a dedicated user account for the logging software
  - This will be `logger` in example configurations
- Remote access

After initial configuration is complete, the logging software needs to be installed from packages or from source. Packaged installation is easier, where packages are available. Compiling software from source is more suitable for developing new features, but would also be required if no pre-made packages are available.

### Option A: Install from packages
Pre-built packages can be downloaded from [GitHub](https://github.com/SELKIE-WP7/Logger/release) and installed using the normal package management tools, for example on Debian based systems (including Raspberry Pi OS) use `sudo dpkg -i <package name>`.

After installation, the core tools will be available to run from the command line. 

\todo Desktop shortcuts for UI tools
\todo Package split description

### Option B: Compile from source
To compile the code from source, you will need the following libraries and tools installed:
- git
- cmake
- gcc
- make
- pkgconfig
- libi2c
- msgpack
- mosquitto
- pthreads
- zlib

To build the documentation, you will additionally need:
- pandoc
- doxygen
- graphviz

Download the software from [GitHub](https://github.com/SELKIE-WP7/Logger) or [SwanSim](https://git.swansim.org/cee-turbine/SELKIELogger)

~~~
[user@local ~]$      git remote add origin https://github.com/SELKIE-WP7/Logger.git
[user@local ~]$      cd Logger
[user@local Logger]$ mkdir build
[user@local build]$  cd build
[user@local build]$  cmake ..
[user@local build]$  make
~~~

After building the software, the tools can be run directly from each directory or a package can be generated for local installation using CPack:

~~~
[user@local build]$ cpack -GDEB # For Debian based devices
[user@local build]$ cpack -GRPM # For Fedora/Red Hat based devices
~~~

### Next Steps
After installation, connect data sources to the computer and create a configuration file.

The software can then be run manually as:
~~~
Logger <config file name>
~~~

An example systemd unit file is included with the source code, which can be used to allow the logging software to run automatically, which may be useful for longer term and autonomous deployments.

## Further reading
* Up: [Logger user guide](@ref Logger)
* Prev: [Concepts](@ref LoggerConcepts)
* Next: [Configuration](@ref LoggerConfig)
