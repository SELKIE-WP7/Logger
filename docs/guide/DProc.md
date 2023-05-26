# Data Processing Guide {#DataProcGuide}

## File conversion

In general, the first step to process recorded data will be to convert the recorded file into a more standard format, generally consisting of a table of values where each row represents a single timestamp and each output value becomes a column.
In most circumstances, you will use both the data file (ending with .dat) and the channel mapping file (ending in .var) to convert the data.

The channel mapping file accelerates the conversion process by storing copies of channel mapping messages received from each source, which allows the conversion tools to determine what fields are present in the recorded data.
If the channel mapping file is unavailable then this information can be obtained directly from the data file, but as this requires reading the entire data file twice it will be a much slower process.

The recorded data files are a stream of data messages received from the individual sources, broken into chunks with timestamp messages at regular intervals.
This is described in more detail in the [file formats](@ref LoggerFiles) section.
The channel mapping information (either from the .var file or extracted from the data file) is used to create a set of output fields that will form the columns of the output data.
Some channels contain data that will generate multiple fields, while other channels map directly to a single field.

The recorded data is then scanned, and the value for each field stored until a new timestamp is encountered.
At this point, a new row is output with the timestamp as its index and the last received value output for each field.
If no value was received for a particular field since the previous timestamp, a suitable value is output to indicate this - either an empty string for text based fields or an invalid / NaN value for numerical fields.

This conversion can be done using [SLConvert](@ref SLConvert) or the graphical interface [SLConvertGUI](@ref SLConvertGUI).
A simpler tool exists that can convert to comma separated value (CSV) format only, which is documented at [dat2csv](@ref dat2csv). This tool is more limited, but can be used in environments where Python is unavailable.

The graphical interface [SLConvertGUI](@ref SLConvertGUI) takes a step-by-step approach to converting data file and is the recommended starting point.


### Supported file formats
The main conversion tools can generate files in the following formats:
- Comma separated values (CSV) - the simplest but often largest output format
- GZip compressed CSV
- Excel compatible spreadsheet in the newer XML format (XLSX)
- Matlab compatible workspace (MAT) file
- Parquet

Each of the formats represents the data in the same way - a series of fields and corresponding timestamps.

In theory, data can be converted to any format supported by the underlying Pandas library if required.
See the [pandas documentation](https://pandas.pydata.org/docs/user_guide/io.html) for possible options.

The SciPy library is used to convert data to a Matlab compatible format.
See the documentation for [scipy.io.savemat](https://docs.scipy.org/doc/scipy/reference/generated/scipy.io.savemat.html#scipy.io.savemat) for compatibility information (at time of writing, MATLAB 7.2+).

### Resampling / Averaging

One of the options provided in [SLConvert](@ref SLConvert) and [SLConvertGUI](@ref SLConvertGUI) allows data to be resampled and averaged before output.
This is useful for two reasons:
- Data can be aligned to a common timing
- Output file size can be reduced

The data is reduced by grouping into intervals of the desired length (0.1s, 1 second, 1 minute etc.) and a simple mean calculated for each field with any invalid (NaN) data discarded.
If a field only contains one value within that interval then the field remains unchanged (although the fields will all be aligned to the interval chosen).
No other checks or conversions are carried out, which can lead to misleading values for fields representing angular quantities or similar.

## Data Extraction

One of the features of the logging software is the ability to embed arbitrary data within the recorded files for later extraction.
This is used to allow messages that aren't directly understood or usable to be examined later, possibly using existing tools for that data format.
As an example this can be used to store the raw data coming from a Datawell buoy for later extraction and analysis with their software, which is more suited to this task than blindly converting data as part of the timeseries outputs described above.

The tools for this process are [SLExtract](@ref SLExtract) and the corresponding graphical interface [SLExtractGUI](@ref SLExtractGUI).

In general, you will want to extract channel 3 (the default raw data channel) of a particular source and use the raw output mode. This ensures that no Logger formatting is added to the output file.
As this data is arbitrary, the extraction tool does not know what file extension to add to the output and will default to a generic ".dat" extension.
Replace this as required or rename the resulting output file to match what your existing software may be expecting.


