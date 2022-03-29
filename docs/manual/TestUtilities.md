# General Test Utilities {#TestUtils}

## Pimoroni Automation HAT Support
These utilities are designed to test functions related to the [Pimoroni Automation HAT](https://shop.pimoroni.com/products/automation-hat) and probably don't have other uses.
### AutomationHatLEDTest {#AutomationHatLEDTest}
Uses the functions defined in [SELKIELoggerI2C](@ref SELKIELoggerI2C) to test support for the SN3218 LED driver used on the automation hat.
Each LED should light in sequence for approximately 1 second.

See [SN3218 control functions](@ref SN3218Control) and [SN3218 Registers](@ref SN3218REG) for details of the library support.

### AutomationHatRead {#AutomationHatRead}
Performs a single read of each ADS1014 ADC channel (0-3) and two delta channels (1-3 and 2-3). This should also work for any other HAT or interface board that places an ADS1015 onto the I2C bus using the default address.

See [ADS1015](@ref ads1015) for the corresponding library documentation.

## Other interface tests
### PowerHatRead {#PowerHatRead}
Designed to test the [Current/Power monitor from Waveshare](https://www.waveshare.com/wiki/Current/Power_Monitor_HAT), this program reads each of the 4 IN219 chips present on that device.
For each of the 4 inputs the voltage across the shunt resistor, bus voltage, current, and power reported by the chip will be printed.

See [INA219](@ref ina219) for library documentation.

### MQTTTest {#MQTTTest}
Tests the support for MQTT subscriptions, optionally including the Victron style keepalive commands. This tool will connect to a specified host and port and subscribe to one or more topics.

```
Usage: MQTTTest [-h host] [-p port] [-k sysid] topic [topic ...]
```

- -h MQTT Broker Host name
- -p MQTT Broker port
- -k sysid Enable Victron compatible keepalive messages/requests for given system ID
- -v Dump all messages

If not specified, will attempt to connect to a broker running locally on port 1883.

## Data format testing
### DumpMessages {#DumpMessages}
Prints a simple representation of each message contained in a data file.

Does not try to read friendly names from a channel mapping file - message source and channel IDs are output as hexadecimal numbers.
