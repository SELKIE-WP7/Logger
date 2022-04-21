import logging

## @file

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
