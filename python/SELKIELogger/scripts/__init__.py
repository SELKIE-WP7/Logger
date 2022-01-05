import logging

# Put general logging configuration in one place
def setupLogging():
    log = logging.getLogger(__name__)
    _lh = logging.StreamHandler()
    # Set the handler to lowest (standard) level
    # Actual output will be filtered based on the level set for log (WARNING by default)
    _lh.setLevel(logging.DEBUG)
    _lf = logging.Formatter("%(asctime)s\t%(levelname)s\t%(message)s")
    _lh.setFormatter(_lf)
    log.addHandler(_lh)
    log.DEBUG = logging.DEBUG
    log.INFO = logging.INFO
    log.WARNING = logging.WARNING
    log.ERROR = logging.ERROR
    log.CRITICAL = logging.CRITICAL
    return log

# Expose a configured logger as "log"
log = setupLogging()
