# Copyright (C) 2023 Swansea University
#
# This file is part of the SELKIELogger suite of tools.
#
# SELKIELogger is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# SELKIELogger is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this SELKIELogger product.
# If not, see <http://www.gnu.org/licenses/>.

## @file

from numpy import sin, cos, sqrt, arcsin, power, deg2rad, isnan

import logging

log = logging.getLogger(__name__)


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

    def getValue(self, state):
        """!
        Extract value from `state` corresponding to this Spec.
        @param state Dataframe containing StateFile data
        @returns floating point value, or NaN if not found
        """
        try:
            val = state.loc[(self.source, self.channel)].Value
            if self.index is None:
                return float(val)
            else:
                return float(val.split("/")[self.index - 1])
        except:
            return float("NaN")

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
        String is a set of comma separated values in this order:
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

    @staticmethod
    def haversine(lat1, lon1, lat2, lon2):
        """!
        Haversine distance formula, from Wikipedia
        @param lat1 Latitude 1 (decimal degrees)
        @param lon1 Longitude 1 (decimal degrees)
        @param lat2 Latitude 2 (decimal degrees)
        @param lon2 Latitude 2 (decimal degrees)
        @returns Distance from 1 -> 2 in metres
        """
        hdLat = (deg2rad(lat2) - deg2rad(lat1)) / 2
        hdLon = (deg2rad(lon2) - deg2rad(lon1)) / 2

        # 2 * r, for r = WGS84 semi-major axis
        return (
            2
            * 6378137
            * arcsin(
                sqrt(
                    power(sin(hdLat), 2) + cos(lat1) * cos(lat2) * power(sin(hdLon), 2)
                )
            )
        )

    def check(self, s):
        """!
        Check whether locator is within warning threshold or not, based on the
        data in `s`

        In returned tuple, `alert` is True if position is unknown or further from
        the reference position than the warning threshold distance. The distance
        from the reference point in metres is returned along with the current
        position in decimal degrees.

        @param s State dataframe
        @returns Tuple of form (alert, distance, (lat, lon))
        """
        curLat = self.latChan.getValue(s)
        curLon = self.lonChan.getValue(s)

        if isnan(curLat) or isnan(curLon):
            log.warning(f"{self.name} has invalid coordinates")
            return (True, float("nan"), (curLat, curLon))

        d = self.haversine(curLat, curLon, self.refLat, self.refLon)
        if d > self.threshold:
            return (True, d, (curLat, curLon))
        return (False, d, (curLat, curLon))

    def __str__(self):
        """! @returns Correctly formatted string"""
        return f"{self.latChan},{self.lonChan},{self.refLat},{self.refLon},{self.threshold},{self.name}"

    def __repr__(self):
        """! @returns Parseable representation"""
        return f"LocatorSpec('{self.__str__():s}')"


class LimitSpec:
    """!
    LimitSpec: String representation of channel limit specification
    """

    def __init__(self, s):
        """!
        Split string into components defining channel and warning limits.
        Values are validated to the extent possible without device
        and source specific information
        String is a set of comma separated values in this order:
         - ChannelSpec()
         - Low value threshold
         - High value threshold
         - [Optional] Name
        @param s String in required format.
        """
        parts = s.split(",")
        if len(parts) < 3 or len(parts) > 4:
            raise ValueError(
                f"Invalid limit specification ({parts}: {len(parts)} parts)"
            )

        ## ChannelSpec() identifying source of latitude data
        self.channel = ChannelSpec(parts[0])

        ## Lower limit
        self.lowLim = float(parts[1])

        ## Upper limit
        self.highLim = float(parts[2])

        if len(parts) == 4:
            ## Name for this locator (used in reporting)
            self.name = parts[3]
        else:
            self.name = f"{self.channel}"

    def check(self, s):
        """!
        Check whether channel is within warning threshold or not, based on the
        data in `s`

        Returns true if value is unknown or outside limits

        @param s State dataframe
        @returns true/false
        """
        value = self.channel.getValue(s)

        if isnan(value):
            log.warning(f"{self.name} has invalid value")
            return True

        return (value < self.lowLim) or (value > self.highLim)

    def getValue(self, s):
        """! @returns Channel getValue result"""
        return self.channel.getValue(s)

    def __str__(self):
        """! @returns Correctly formatted string"""
        return f"{self.channel},{self.lowLim},{self.highLim},{self.name}"

    def __repr__(self):
        """! @returns Parseable representation"""
        return f"LocatorSpec('{self.__str__():s}')"
