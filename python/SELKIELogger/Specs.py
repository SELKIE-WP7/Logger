## @file


class ChannelSpec:
    """! ChannelSpec: Specify a data channel with a formatted string"""

    def __init__(self, s):
        """!
        Split string into source, channel, and optionally array index
        components. Values are validated to the extent possible without device
        and source specific information
        @param s String in format 'source:channel[:index]'
        """
        parts = s.split(":")
        l = len(parts)
        if l < 2 or l > 3:
            raise ValueError("Invalid channel specification")

        ## Selected source ID
        self.source = int(parts[0], base=0)

        ## Selected channel ID
        self.channel = int(parts[1], base=0)
        if l == 3:
            ## Selected array index (optional)
            self.index = int(parts[2], base=0)
        else:
            self.index = None

        if self.source < 0 or self.source > 127:
            raise ValueError("Invalid channel specification (bad source)")

        if self.channel < 0 or self.channel > 127:
            raise ValueError("Invalid channel specification (bad channel)")

        if not self.index is None and self.index < 0:
            raise ValueError("Invalid channel specification (bad index)")

    def __str__(self):
        """! @returns Correctly formatted string"""
        if self.index is None:
            return f"0x{self.source:02x}:0x{self.channel:02x}"
        else:
            return f"0x{self.source:02x}:0x{self.channel:02x}:0x{self.index:02x}"

    def __repr__(self):
        """! @returns Parseable representation"""
        return f"ChannelSpec('{self.__str__():s}')"


class LocatorSpec:
    """!
    LocatorSpec: String representation of location monitoring settings
    """

    def __init__(self, s):
        """!
        Split string into components defining location warning settings.
        Values are validated to the extent possible without device
        and source specific information
        String is a set of colon separated values in this order:
         - Latitude ChannelSpec()
         - Longitude ChannelSpec()
         - Reference Latitude
         - Reference Longitude
         - Warning threshold distance (m)
         - [Optional] Name
        @param s String in required format.
        """
        parts = s.split(",")
        if len(parts) < 5 or len(parts) > 6:
            raise ValueError("Invalid locator specification")

        ## ChannelSpec() identifying source of latitude data
        self.latChan = ChannelSpec(parts[0])

        ## ChannelSpec() identifying source of longitude data
        self.lonChan = ChannelSpec(parts[1])

        ## Reference latitude (decimal degrees)
        self.refLat = float(parts[2])

        ## Reference longitude (decimal degrees)
        self.refLon = float(parts[3])

        ## Warning Distance (m)
        self.threshold = float(parts[4])

        if len(parts) == 6:
            ## Name for this locator (used in reporting)
            self.name = parts[5]
        else:
            self.name = f"{self.latChan}/{self.lonChan}"

    def __str__(self):
        """! @returns Correctly formatted string"""
        return f"{self.latChan},{self.lonChan},{self.refLat},{self.refLon},{self.threshold},{self.name}"

    def __repr__(self):
        """! @returns Parseable representation"""
        return f"LocatorSpec('{self.__str__():s}')"
