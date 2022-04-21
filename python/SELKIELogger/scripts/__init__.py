import logging

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


# Put general logging configuration in one place
def setupLogging():
    """!
    Configure custom logging instance, using the functions from the rich
    package if it is available and falling back to standard definitions
    otherwise.
    @returns Custom logger object, with some module items added for convenience.
    """
    log = logging.getLogger("")
    try:
        from rich.logging import RichHandler

        _rh = RichHandler(enable_link_path=False)
        log.addHandler(_rh)

    except ImportError:
        # 'rich' package not available, so fall back to standard config
        _lh = logging.StreamHandler()
        # Set the handler to lowest (standard) level
        # Actual output will be filtered based on the level set for log (WARNING by default)
        _lh.setLevel(logging.DEBUG)
        _lf = logging.Formatter("[%(asctime)s]\t%(levelname)s\t%(message)s")
        _lh.setFormatter(_lf)
        log.addHandler(_lh)

    # Expose some functions and constants from the logging module
    log.DEBUG = logging.DEBUG
    log.INFO = logging.INFO
    log.WARNING = logging.WARNING
    log.ERROR = logging.ERROR
    log.CRITICAL = logging.CRITICAL
    log.getLevelName = logging.getLevelName
    return log


## Expose a configured logger as "log"
log = setupLogging()
