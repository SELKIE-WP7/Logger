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

import sys
import logging
import traceback

import tkinter as tk
from tkinter import ttk


def exceptionHandler(exc_type, exc_value, exc_traceback, thread=None):
    if issubclass(exc_type, KeyboardInterrupt):
        logging.info("Exiting on keyboard interrupt")
        sys.exit()
        return

    if thread:
        logging.critical(
            f"Unhandled exception in {thread}: {exc_type.__name__} - {exc_value}"
        )
    else:
        logging.critical(f"Unhandled exception: {exc_type.__name__} - {exc_value}")
    if exc_traceback:
        for tbline in traceback.format_tb(exc_traceback):
            logging.critical(tbline)


class TextHandler(logging.Handler):
    # This class allows you to log to a Tkinter Text or ScrolledText widget
    # Adapted from Moshe Kaplan: https://gist.github.com/moshekaplan/c425f861de7bbf28ef06

    def __init__(self, text):
        # run the regular Handler __init__
        logging.Handler.__init__(self)
        # Store a reference to the Text it will log to
        self.text = text
        _lf = logging.Formatter("[%(asctime)s]\t%(levelname)s\t%(message)s")
        self.setFormatter(_lf)

    def emit(self, record):
        msg = self.format(record)

        def append():
            self.text.configure(state="normal")
            self.text.insert(tk.END, msg + "\n")
            self.text.configure(state="disabled")
            # Autoscroll to the bottom
            self.text.yview(tk.END)

        # This is necessary because we can't modify the Text from other threads
        self.text.after(0, append)


def Button(parent, command, config=None, **kwargs):
    """Wrapper around tk.Button()

    Applies a default configuration and binds commands to both the default
    "command" option and additionally to the return/enter key to match
    expected behaviour.

    Default configuration can be overriden by providing a dictionary of
    options that will be expanded and passed to .configure()
    """
    button_defconfig = {"width": 8}
    # b = tk.Button(parent, command=command, **kwargs)
    b = ttk.Button(parent, command=command)
    b.bind("<Return>", command)
    b.configure(**button_defconfig)
    if config:
        b.configure(**config)
    return b


def logFuncExceptions(func, *args, **kwargs):
    res = None
    try:
        res = func(*args, **kwargs)
    except Exception as e:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        logging.critical(f"Exception: {exc_type.__name__} - {exc_value}")
        if exc_traceback:
            for tbline in traceback.format_tb(exc_traceback):
                logging.debug(tbline)
    return res


def main(gui):
    root = tk.Tk()
    root.geometry("800x600")
    try:
        ttk.Style().theme_use("alt")
    except:
        pass

    sys.excepthook = exceptionHandler
    try:
        from threading import excepthook

        excepthook = exceptionHandler  # For threads
    except ImportError:
        logging.warning(
            "Threaded exception handler not available - some errors may only appear on the console"
        )

    app = gui(root)
    root.protocol("WM_DELETE_WINDOW", app.exit)

    logging.info("GUI startup")
    root.mainloop()
